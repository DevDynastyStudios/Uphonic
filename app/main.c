NAUI_APP("Uphonic")


#define UPH_METRONOME_TIME_RESET 2.0f
#define UPH_METRONOME_MAX_TAPS 8
#define UPH_METRONOME_MIN_INTERVAL (60.0f / 300.0f)
#define UPH_METRONOME_OUTLIER_LOW 0.5f
#define UPH_METRONOME_OUTLIER_HIGH 2.0f

static bool _metronome_active = false;
static float _metronome_last_tap_time;
static float _metronome_avg_interval;
static uint32_t _metronome_count = 0;

Uph_State uph_state = { 0 };

void uph_midi_on_input(const cmidi_event_t* event, void* userdata)
{
	uph_key_bitset_set(uph_midi_editor_data.active_keys, event->note, event->type == CMIDI_NOTE_ON);

	//naui_log(NAUI_LOG_INFO, "[CMIDI] Pressed Note: %i", event->note);
	//cmidi_play_note(uph_state.settings.midi.output, 1, 60, 100);
}

static void _uph_metronome_reset()
{
	_metronome_active = false;
	_metronome_avg_interval = 0.0f;
	_metronome_count = 0;
}

static bool _uph_metronome_tap(float *out_bpm)
{
	const float current_time = naui_time();

	if (!_metronome_active)
	{
		_metronome_active = true;
		_metronome_last_tap_time = current_time;
		_metronome_count = 0;
		_metronome_avg_interval = 0.0f;
		return false;
	}

	const float since_last = current_time - _metronome_last_tap_time;

	if (since_last > UPH_METRONOME_TIME_RESET)
	{
		_metronome_last_tap_time = current_time;
		_metronome_count = 0;
		_metronome_avg_interval = 0.0f;
		return false;
	}

	if (since_last < UPH_METRONOME_MIN_INTERVAL)
		return false;

	if (_metronome_avg_interval > 0.0f)
	{
		const float ratio = since_last / _metronome_avg_interval;
		if (ratio < UPH_METRONOME_OUTLIER_LOW || ratio > UPH_METRONOME_OUTLIER_HIGH)
		{
			_metronome_last_tap_time = current_time;
			return false;
		}
	}

	const uint32_t n = (_metronome_count < UPH_METRONOME_MAX_TAPS) ? (_metronome_count + 1) : UPH_METRONOME_MAX_TAPS;
	_metronome_avg_interval += (since_last - _metronome_avg_interval) / (float)n;
	_metronome_count = n;
	_metronome_last_tap_time = current_time;
	*out_bpm = 60.0f / _metronome_avg_interval;
	return true;
}

void naui_app_start(void)
{
	uph_settings_set_defaults();
	const bool settings_loaded = uph_settings_load();
	uph_settings_sanitize();

	naui_localization_set_current(naui_string_format("%s-%s", uph_state.settings.general.language_code.data, uph_state.settings.general.region_code.data));
	naui_load_theme(uph_state.settings.general.theme.data);
	naui_load_font(0, "MYRIADPRO-REGULAR");	// Get font selection in settings later
	uph_audio_engine_init();
	uph_ui_widgets_init();

	uph_state.panels.song_timeline = NAUI_ATTACH_PANEL(uph_song_timeline);
	uph_state.panels.pattern_list = NAUI_ATTACH_PANEL(uph_pattern_list);
	uph_state.panels.sample_list = NAUI_ATTACH_PANEL(uph_sample_list);
	uph_state.panels.automation_list = NAUI_ATTACH_PANEL(uph_automation_list);
	uph_state.panels.mixer = NAUI_ATTACH_PANEL(uph_mixer);
	uph_state.panels.resource_editor = NAUI_ATTACH_PANEL(uph_midi_editor);
	naui_close_panel(uph_state.panels.settings = NAUI_ATTACH_PANEL(uph_settings));
	naui_close_panel(uph_state.panels.plugin_list = NAUI_ATTACH_PANEL(uph_plugin_list));

	//NAUI_ATTACH_PANEL(uph_file_dialog);

	naui_set_main_viewport(naui_dock_panel(
		naui_dock_panel(
			uph_state.panels.song_timeline,
			naui_dock_panel(naui_dock_panel(
				uph_state.panels.pattern_list,
				uph_state.panels.sample_list,
				NAUI_DOCK_DIRECTION_CENTER, 0.0f
			), uph_state.panels.automation_list,
			NAUI_DOCK_DIRECTION_CENTER, 0.0f),
			NAUI_DOCK_DIRECTION_RIGHT, 0.8f
		),
		naui_dock_panel(
			uph_state.panels.mixer,
			uph_state.panels.resource_editor,
			NAUI_DOCK_DIRECTION_LEFT, 0.7f
		),
		NAUI_DOCK_DIRECTION_BOTTOM, 0.6f
	));

	uph_settings_open_midi_ports(!settings_loaded);
	uph_project_create(naui_string_from_cstr("Test Project"));
}

void naui_app_end(void)
{
	uph_settings_save();

	cmidi_scheduler_stop_all();
	cmidi_shutdown();

	uph_audio_engine_shutdown();
}

void naui_app_update(void)
{
	uph_render_main_titlebar();

	const Leaf_Color bg_color = naui_theme_color("naui_panel_title_bg_color");
	const Leaf_Color tool_icon_color = naui_theme_color("uph_tool_icon_color");

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(15))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(12), NAUI_DPI(8)),
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.color = {bg_color},
		.child_gap = NAUI_DPI(10.0f)
	})
	{
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(4.0f)
		});

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(4.0f)
		});

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(8.0f)
		})
		{
			{
				static bool metronome_enabled;
				const bool metronome_was_enabled = metronome_enabled;
				const Naui_Vec2 icon_size = { 16, 16 };

				if (metronome_enabled) {
					Leaf_ID bpm_btn_id = leaf_id("uph_bpm_button");
					if (leaf_hovered(bpm_btn_id))
						naui_set_cursor(NAUI_CURSOR_HAND);

					if (uph_ui_image_button(naui_asset_image("uph_icon_tappad"), bpm_btn_id, naui_vec2_scale(icon_size, 2.0f), tool_icon_color)) {
						float bpm;
						if (_uph_metronome_tap(&bpm)) {
							uph_state.project.bpm = bpm;
						}
					}
				}

				if (uph_ui_image_toggle_button(naui_asset_image("uph_icon_metronome"), leaf_id("uph_metronome_toggle"), icon_size, tool_icon_color, metronome_enabled))
					metronome_enabled = !metronome_enabled;

				if (metronome_enabled && !metronome_was_enabled) {
					_uph_metronome_reset();
				}
			}

			leaf({
				.size = { LEAF_SIZE_FIXED(50), LEAF_SIZE_GROW },
				.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
			})
			{
				const Naui_String bpm = naui_string_format("BPM: %.1f", uph_state.project.bpm);
				uph_ui_drag_float(&uph_state.project.bpm, leaf_id("uph_bpm_drag"), 1.0f, 1.0f, 10000.0f, "%.2f", UPH_UI_DRAG_CLAMPED);
			}
		}
	}

	naui_render_panels_and_viewport();
	uph_ui_widgets_flush();

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
    {
        if (uph_state.project.tracks[i].instrument.loaded)
            uph_update_plugin(&uph_state.project.tracks[i].instrument);
    }
}

void naui_app_event(const Naui_AppEventData *data)
{
	if (data->type == NAUI_APP_EVENT_FILE_DROP)
	{
		uph_project_add_file(&uph_state.project, NAUI_PATH(data->file_drop.paths[0]));
	}
}