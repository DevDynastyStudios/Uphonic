NAUI_APP("Uphonic")

Uph_State uph_state = { 0 };

#pragma region Crash Recovery

static bool s_exit_confirmed;
static bool s_exit_dialog_open;
static Naui_Path s_restore_project_folder;

static void uph_exit_dialog_result(int32_t button, void *user_data)
{
	(void)user_data;
	s_exit_dialog_open = false;

	if (button == 0)	// Save
	{
		if (!uph_project_save(&uph_state.project, UPH_SAVE_TYPE_CANONICAL))
		{
			naui_log(NAUI_LOG_ERROR, "Save failed, not closing");
			return;
		}
	}
	else if (button == 1)
		uph_recovery_discard_autosave(uph_project_get_path(&uph_state.project));
	else
		return;	// Cancel

	s_exit_confirmed = true;
	naui_defer((Naui_DeferredEvent)naui_app_close, NULL, 0);
}

static void uph_handle_close_request(void)
{
	if (s_exit_confirmed)
		return;

	if (s_exit_dialog_open)
	{
		naui_app_cancel_close();
		return;
	}

	if (!uph_autosave_has_unsaved_changes())
		return;

	if (!uph_state.settings.general.confirm_on_exit)
	{
		uph_autosave_flush();
		return;
	}

	naui_app_cancel_close();
	s_exit_dialog_open = uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("dialog.exit.title"),
		.message = NAUI_TR("dialog.exit.message"),
		.buttons = { NAUI_TR("dialog.exit.save"), NAUI_TR("dialog.exit.discard"), NAUI_TR("dialog.cancel") },
		.default_button = 0,
		.cancel_button = 2,
		.callback = uph_exit_dialog_result
	});
}

static void uph_restore_dialog_result(int32_t button, void *user_data)
{
	(void)user_data;

	if (button == 0)	// Restore
	{
		if (!uph_project_load(&uph_state.project, s_restore_project_folder))
			naui_log(NAUI_LOG_ERROR, "Failed to restore project: %s", s_restore_project_folder.data);
		return;
	}

	uph_recovery_discard_autosave(s_restore_project_folder);
	uph_recovery_clear_crash(s_restore_project_folder);
}

static void uph_show_restore_dialog(const Naui_Path project_folder)
{
	s_restore_project_folder = project_folder;
	char message[512];
	snprintf(message, sizeof(message), "%s\n\n%s", NAUI_TR("dialog.restore.message"), naui_view_to_string(naui_file_filename(&project_folder)).data);

	uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("dialog.restore.title"),
		.message = message,
		.buttons = { NAUI_TR("dialog.restore.restore"), NAUI_TR("dialog.restore.discard") },
		.default_button = 0,
		.cancel_button = -1,
		.callback = uph_restore_dialog_result
	});
}

#pragma endregion

#pragma region Metronome
#define UPH_METRONOME_TIME_RESET 2.0f
#define UPH_METRONOME_MAX_TAPS 8
#define UPH_METRONOME_MIN_INTERVAL (60.0f / 300.0f)
#define UPH_METRONOME_OUTLIER_LOW 0.5f
#define UPH_METRONOME_OUTLIER_HIGH 2.0f

static bool _metronome_active = false;
static float _metronome_last_tap_time;
static float _metronome_avg_interval;
static uint32_t _metronome_count = 0;

void uph_midi_on_input(const cmidi_event_t* event, void* userdata)
{
	uph_piano_set_external_key(&uph_midi_editor_data.piano, event->note, event->type == CMIDI_NOTE_ON);

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

#pragma endregion

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
	uph_state.panels.resource_list = NAUI_ATTACH_PANEL(uph_resource_list);
	uph_state.panels.mixer = NAUI_ATTACH_PANEL(uph_mixer);
	uph_state.panels.effect_list = NAUI_ATTACH_PANEL(uph_effect_list);
	uph_state.panels.resource_editor = NAUI_ATTACH_PANEL(uph_midi_editor);
	naui_close_panel(uph_state.panels.settings = NAUI_ATTACH_PANEL(uph_settings));
	naui_close_panel(uph_state.panels.plugin_list = NAUI_ATTACH_PANEL(uph_plugin_list));
	naui_close_panel(uph_state.panels.automation_list = NAUI_ATTACH_PANEL(uph_automation_list));

	uph_file_dialog_register();

	naui_set_main_viewport(naui_dock_panel(
		naui_dock_panel(
			uph_state.panels.song_timeline,
			uph_state.panels.resource_list,
			NAUI_DOCK_DIRECTION_RIGHT, 0.8f
		),
		naui_dock_panel(
			naui_dock_panel(
				uph_state.panels.mixer,
				uph_state.panels.effect_list,
				NAUI_DOCK_DIRECTION_RIGHT,
				0.7f
			),
			uph_state.panels.resource_editor,
			NAUI_DOCK_DIRECTION_LEFT, 0.6f
		),
		NAUI_DOCK_DIRECTION_BOTTOM, 0.6f
	));

	uph_settings_open_midi_ports(!settings_loaded);
	uph_action_initialize();

	Naui_Path crashed_project = {0};
	const bool crashed = uph_recovery_find_crashed_project(UPHONIC_WORKSPACE_FOLDER, &crashed_project);
	uph_project_create(naui_string_from_cstr("Test Project"));

	if (crashed)
		uph_show_restore_dialog(crashed_project);
}

void naui_app_end(void)
{
	uph_recovery_release_lock();

	naui_log(NAUI_LOG_INFO, "Saving Settings...");
	uph_settings_save();

	naui_log(NAUI_LOG_INFO, "Closing Audio Engine...");
	uph_audio_engine_shutdown();

	naui_log(NAUI_LOG_INFO, "Destoying Actions...");
	uph_action_shutdown();

	naui_log(NAUI_LOG_INFO, "Clearing Tracks...");
	uph_resources_clear_tracks();

	naui_log(NAUI_LOG_INFO, "Stopping MIDI Engine...");
	cmidi_scheduler_stop_all();
	cmidi_shutdown();

	naui_log(NAUI_LOG_INFO, "Thanks For Using Uphonic.");
}

static void uph_update_all_track_plugins(Uph_Track *track)
{
	if (track->instrument.loaded)
		uph_update_plugin(&track->instrument);

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->effects); i++)
	{
		if (track->effects[i].plugin.loaded)
			uph_update_plugin(&track->effects[i].plugin);
	}

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
		uph_update_all_track_plugins(&track->subtracks[i]);
}

void naui_app_update(void)
{
	uph_render_main_titlebar();

	const Leaf_Color bg_color = naui_theme_color("naui_panel_title_bg_color");
	const Leaf_Color tool_icon_color = naui_theme_color("uph_tool_icon_color");

	if (naui_key_down(NAUI_KEY_LCONTROL))
	{
		if (naui_key_pressed(NAUI_KEY_P))
			naui_action_execute_stack(UPH_ACTION_PATTERN_CREATE, (Uph_ActionResourceCreate){0});

		if (naui_key_pressed(NAUI_KEY_Z))
			naui_action_undo();
		else if (naui_key_pressed(NAUI_KEY_Y))
			naui_action_redo();
	}

	/*leaf({
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
		}
	}*/

	if (uph_dialog_is_open())
		naui_occlude_all_panels();

	naui_render_panels_and_viewport();
	uph_ui_widgets_flush();

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
        uph_update_all_track_plugins(&uph_state.project.tracks[i]);

	uph_autosave_update();
}

void naui_app_event(const Naui_AppEventData *data)
{
	if (data->type == NAUI_APP_EVENT_FILE_DROP)
	{
		for (uint32_t i = 0; i < data->file_drop.path_count; i++)
			uph_project_add_file(&uph_state.project, NAUI_PATH(data->file_drop.paths[i]));
	}
}