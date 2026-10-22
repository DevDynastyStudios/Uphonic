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

static _Atomic bool s_midi_instrument_monitoring; // Do not remove! This is Mr. Optimization (Ironic)

// Go through the tracks on the main thread instead of bogging down the cmidi thread.
static bool _uph_any_track_monitors_instrument(Naui_List(Uph_Track) tracks)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		if (uph_track_monitors_instrument(&tracks[i]) || _uph_any_track_monitors_instrument(tracks[i].subtracks))
			return true;
	}

	return false;
}

static int _uph_midi_thru_filter(const uint8_t* bytes, uint8_t length, void* userdata)
{
	(void)userdata;
	const bool is_note_on = (bytes[0] & 0xF0) == CMIDI_CODE_NOTE_ON && length >= 3 && bytes[2] > 0;
	return !(is_note_on && atomic_load_explicit(&s_midi_instrument_monitoring, memory_order_relaxed));
}

static void _uph_route_monitored_note_event(Uph_Track* track, const cmidi_event_t* event, bool note_on)
{
	if (uph_track_monitors_instrument(track))
		uph_plugin_queue_note_event(&track->instrument, note_on, event->note, 0, event->velocity, 0);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		_uph_route_monitored_note_event(&track->subtracks[i], event, note_on);
	}
}

void uph_midi_on_input(const cmidi_event_t* event, void* userdata)
{
	(void)userdata;
	bool note_on = event->type == CMIDI_NOTE_ON;
	uph_piano_set_external_key(&uph_midi_editor_data.piano, event->note, note_on);

	if (event->type == CMIDI_NOTE_ON || event->type == CMIDI_NOTE_OFF)
	{
		uph_midi_recorder_input(note_on, event->note, event->velocity);

		for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
		{
			_uph_route_monitored_note_event(&uph_state.project.tracks[i], event, note_on);
		}
	}

}

void naui_app_start(void)
{
	uph_settings_set_defaults();
	const bool settings_loaded = uph_settings_load();
	uph_settings_sanitize();

	uph_language_apply();
	naui_load_theme(uph_state.settings.general.theme.data);
	cmidi_set_thru_filter(_uph_midi_thru_filter, NULL);
	uph_audio_engine_init();
	uph_ui_widgets_init();

	uph_state.panels.song_timeline = NAUI_ATTACH_PANEL(uph_song_timeline);
	uph_state.panels.resource_list = NAUI_ATTACH_PANEL(uph_resource_list);
	uph_state.panels.mixer = NAUI_ATTACH_PANEL(uph_mixer);
	uph_state.panels.resource_editor = NAUI_ATTACH_PANEL(uph_resource_editor);
	naui_panel_close(uph_state.panels.settings = NAUI_ATTACH_PANEL(uph_settings));
	naui_panel_close(uph_state.panels.plugin_list = NAUI_ATTACH_PANEL(uph_plugin_list));
	naui_panel_close(uph_state.panels.automation_list = NAUI_ATTACH_PANEL(uph_automation_list));

	uph_file_dialog_register();

	naui_viewport_set(naui_panel_dock(
		naui_panel_dock(
			uph_state.panels.song_timeline,
			uph_state.panels.resource_list,
			NAUI_DOCK_DIRECTION_RIGHT, 0.8f
		),
		naui_panel_dock(
			uph_state.panels.mixer,
			uph_state.panels.resource_editor,
			NAUI_DOCK_DIRECTION_LEFT, 0.75f
		),
		NAUI_DOCK_DIRECTION_BOTTOM, 0.5f
	));

	uph_settings_open_midi_ports(!settings_loaded);
	uph_action_initialize();

	Naui_Path crashed_project = {0};
	const bool crashed = uph_recovery_find_crashed_project(UPHONIC_WORKSPACE_FOLDER, &crashed_project);
	uph_project_create(uph_project_untitled_name());

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

static void uph_update_master_track_plugins(void)
{
	Uph_MasterTrack *master_track = &uph_state.project.master_track;
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(master_track->effects); i++)
		if (master_track->effects[i].plugin.loaded)
			uph_plugin_update(&master_track->effects[i].plugin);
}

static void uph_update_all_track_plugins(Uph_Track *track)
{
	if (track->instrument.loaded)
		uph_plugin_update(&track->instrument);

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->effects); i++)
	{
		if (track->effects[i].plugin.loaded)
			uph_plugin_update(&track->effects[i].plugin);
	}

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
		uph_update_all_track_plugins(&track->subtracks[i]);
}

void naui_app_update(void)
{
	uph_mixer_apply_pending_move();
	uph_song_timeline_apply_pending_move();

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

	naui_render_panels_and_viewport();
	uph_ui_widgets_flush();

	uph_update_master_track_plugins();
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
        uph_update_all_track_plugins(&uph_state.project.tracks[i]);

	atomic_store_explicit(&s_midi_instrument_monitoring, _uph_any_track_monitors_instrument(uph_state.project.tracks), memory_order_relaxed);
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