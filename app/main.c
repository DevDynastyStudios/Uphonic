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
	uph_state.settings.audio = (Uph_AudioSettings){
		.sample_rate = 48000
	};
	uph_state.project.bpm = 120.0f;

	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
	uph_audio_engine_init();
	naui_set_main_viewport(naui_dock_panel(
		naui_dock_panel(
			NAUI_ATTACH_PANEL(uph_song_timeline),
			naui_dock_panel(
				NAUI_ATTACH_PANEL(uph_pattern_list),
				NAUI_ATTACH_PANEL(uph_sample_list),
				NAUI_DOCK_DIRECTION_CENTER, 0.0f
			),
			NAUI_DOCK_DIRECTION_RIGHT, 0.8f
		),
		naui_dock_panel(
			NAUI_ATTACH_PANEL(uph_mixer),
			NAUI_ATTACH_PANEL(uph_midi_editor),
			NAUI_DOCK_DIRECTION_LEFT, 0.7f
		),
		NAUI_DOCK_DIRECTION_BOTTOM, 0.6f
	));

	uph_project_create(naui_string_from_cstr("Test Project"));
}

void naui_app_end(void)
{
	uph_audio_engine_shutdown();
}

void naui_app_update(void)
{
	uph_render_main_titlebar();

	const Leaf_Color bg_color = naui_theme_color("naui_panel_title_bg_color");
	const Leaf_Color tool_icon_color = naui_theme_color("uph_tool_icon_color");
	const Leaf_Color play_icon_color = naui_theme_color("uph_play_icon_color");
	const Leaf_Color pause_icon_color = naui_theme_color("uph_pause_icon_color");
	const Leaf_Color stop_icon_color = naui_theme_color("uph_stop_icon_color");

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(15.0f))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(12.0f), NAUI_DPI(8.0f)),
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.color = bg_color,
		.child_gap = NAUI_DPI(10.0f)
	})
	{
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(4.0f)
		})
		{
			/*naui_image_button(
				naui_asset_image("uph_icon_select"),
				naui_vec2(15.0f, 15.0f),
				tool_icon_color
			);

			naui_image_button(
				naui_asset_image("uph_icon_draw"),
				naui_vec2(15.0f, 15.0f),
				tool_icon_color
			);

			naui_image_button(
				naui_asset_image("uph_icon_cut"),
				naui_vec2(15.0f, 15.0f),
				tool_icon_color
			);*/
		}
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
					if (uph_ui_widget_button(naui_asset_image("uph_icon_tappad"), leaf_id("uph_bpm_button"), naui_vec2_scale(icon_size, 2.0f), tool_icon_color, NAUI_CURSOR_HAND)) {
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
}

void naui_app_event(const Naui_AppEventData *data)
{
	if (data->type == NAUI_APP_EVENT_FILE_DROP)
	{
		uph_resources_add_sample_from_file(NAUI_PATH(data->file_drop.paths[0]));
	}
}