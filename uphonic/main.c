#define UPH_JOB_THREAD_COUNT 2
#define UPH_JOB_CAPACITY 16

NAUI_APP("Uphonic")
 
Uph_State uph_state = { 0 };
 
#pragma region Crash Recovery
 
typedef struct
{
	bool exit_confirmed;
	bool exit_dialog_open;
	Naui_Path restore_project_folder;
}
Uph_CrashRecoveryState;

static Uph_CrashRecoveryState uph__crash_recovery_state;
 
static void uph_show_restore_dialog(const Naui_Path project_folder);
 
static void uph_exit_dialog_result(int32_t button, void *user_data)
{
	Uph_CrashRecoveryState *state = &uph__crash_recovery_state;
	(void)user_data;
	state->exit_dialog_open = false;
 
	if (button == 0)	// Save
	{
		if (!uph_project_save(&uph_state.project, UPH_SAVE_TYPE_CANONICAL))
		{
			naui_log(NAUI_LOG_ERROR, "Save failed, not closing");
			return;
		}
	}
	else if (button == 1)
	{
		uph_recovery_discard_project(uph_project_get_path(&uph_state.project));
		uph_autosave_mark_clean();
	}
	else
		return;	// Cancel
 
	state->exit_confirmed = true;
	naui_defer((Naui_DeferredEvent)naui_app_close, NULL, 0);
}
 
static void uph_handle_close_request(void)
{
	Uph_CrashRecoveryState *state = &uph__crash_recovery_state;
	if (state->exit_confirmed)
		return;
 
	if (state->exit_dialog_open)
	{
		naui_app_cancel_close();
		return;
	}
 
	const Naui_Path project_folder = uph_project_get_path(&uph_state.project);
	const bool never_saved = !uph_project_has_been_saved(&uph_state.project);
 
	if (!uph_autosave_has_unsaved_changes())
	{
		if (never_saved)
			uph_recovery_discard_project(project_folder);
 
		return;
	}
 
	if (!uph_state.settings.general.confirm_on_exit)
	{
		if (never_saved)
		{
			uph_recovery_discard_project(project_folder);
			if (!naui_path_exists(project_folder))
			{
				uph_autosave_mark_clean();
				return;
			}
		}
 
		uph_autosave_flush();
		return;
	}
 
	naui_app_cancel_close();
	state->exit_dialog_open = uph_dialog_open(&(Uph_DialogConfig){
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
	Uph_CrashRecoveryState *state = &uph__crash_recovery_state;
	(void)user_data;
 
	if (button == 0)	// Restore
	{
		if (!uph_project_load(&uph_state.project, state->restore_project_folder))
			naui_log(NAUI_LOG_ERROR, "Failed to restore project: %s", state->restore_project_folder.data);
 
		return;
	}
 
	uph_recovery_discard_project(state->restore_project_folder);
 
	Naui_Path next_project = {0};
	if (uph_recovery_find_recoverable_project(UPHONIC_WORKSPACE_FOLDER, &next_project))
		uph_show_restore_dialog(next_project);
}
 
static void uph_show_restore_dialog(const Naui_Path project_folder)
{
	Uph_CrashRecoveryState *state = &uph__crash_recovery_state;
	state->restore_project_folder = project_folder;
	const char *message_key = uph_recovery_project_crashed(project_folder) ? "dialog.restore.message" : "dialog.restore.unsaved_message";
	char message[512];
	snprintf(message, sizeof(message), "%s\n\n%s", NAUI_TR(message_key), naui_view_to_string(naui_file_filename(&project_folder)).data);
 
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
 
static _Atomic bool uph__midi_instrument_monitoring; // Do not remove! This is Mr. Optimization (Ironic)
 
// Go through the tracks on the main thread instead of bogging down the cmidi thread.
static bool uph__any_track_monitors_instrument(Naui_List(Uph_Track) tracks)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		if (uph_track_monitors_instrument(&tracks[i]) || uph__any_track_monitors_instrument(tracks[i].subtracks))
			return true;
	}
 
	return false;
}
 
static int uph__midi_thru_filter(const uint8_t* bytes, uint8_t length, void* userdata)
{
	(void)userdata;
	const bool is_note_on = (bytes[0] & 0xF0) == CMIDI_CODE_NOTE_ON && length >= 3 && bytes[2] > 0;
	return !(is_note_on && atomic_load_explicit(&uph__midi_instrument_monitoring, memory_order_relaxed));
}
 
static void uph__route_monitored_note_event(Uph_Track* track, const cmidi_event_t* event, bool note_on)
{
	if (uph_track_monitors_instrument(track))
		uph_plugin_queue_note_event(&track->instrument, note_on, event->note, 0, event->velocity, 0);
 
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		uph__route_monitored_note_event(&track->subtracks[i], event, note_on);
	}
}
 
void uph_midi_on_input(const cmidi_event_t* event, void* userdata)
{
	Uph_MidiEditorState *state = &uph__midi_editor_state;
	(void)userdata;
	bool note_on = event->type == CMIDI_NOTE_ON;

	if (event->type == CMIDI_CONTROL_CHANGE)
	{
		uph_midi_recorder_pedal_input(event->cc_number, event->cc_value);
		return;
	}

	if (event->type == CMIDI_NOTE_ON || event->type == CMIDI_NOTE_OFF)
	{
		uph_piano_set_external_key(&state->piano, event->note, note_on);
		uph_midi_recorder_input(note_on, event->note, event->velocity);

		const Uph_Track *voice = uph_midi_preview_voice();
		if (!note_on || event->velocity == 0)
			uph_midi_preview_note_off(event->note);
		else if (voice && uph_midi_preview_voice_monitoring() && !uph_track_monitors_instrument(voice))
			uph_midi_preview_note_on(event->note, event->velocity);
 
		for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
		{
			uph__route_monitored_note_event(&uph_state.project.tracks[i], event, note_on);
		}
	}
 
}
 
void naui_app_start(void)
{
	uph_settings_set_defaults();
	const bool settings_loaded = uph_settings_load();
	uph_settings_sanitize();
 
	uph_language_apply();
	naui_app_set_ui_scale(uph_state.settings.general.ui_scale);
	naui_app_set_animations_enabled(uph_state.settings.general.animations);
	uph_theme_load(uph_state.settings.general.theme.data);
	uph_io_load_scales(&uph_state, naui_path_join(naui_directory_get(NAUI_DIR_ASSETS), NAUI_PATH("Scales")));
	cmidi_set_thru_filter(uph__midi_thru_filter, NULL);
	uph_audio_engine_init();
	uph_ui_widgets_init();
	uph_ui_cursors_init();
	uph_icons_load();
	uph_plugin_modules_init();
	naui_jobs_init(UPH_JOB_THREAD_COUNT, UPH_JOB_CAPACITY);
 
	uph_state.panels.song_timeline = NAUI_ATTACH_PANEL(uph_song_timeline);
	uph_state.panels.resource_list = NAUI_ATTACH_PANEL(uph_resource_list);
	uph_state.panels.mixer = NAUI_ATTACH_PANEL(uph_mixer);
	uph_state.panels.resource_editor = NAUI_ATTACH_PANEL(uph_resource_editor);
	naui_panel_close(uph_state.panels.settings = NAUI_ATTACH_PANEL(uph_settings));
	naui_panel_close(uph_state.panels.plugin_list = NAUI_ATTACH_PANEL(uph_plugin_list));
	naui_panel_close(uph_state.panels.automation_list = NAUI_ATTACH_PANEL(uph_automation_list));
	naui_panel_close(uph_state.panels.render = NAUI_ATTACH_PANEL(uph_render));
	uph_state.panels.sample_browser = NAUI_ATTACH_PANEL(uph_sample_browser);
 
	naui_panel_enable_flags(uph_state.panels.song_timeline, NAUI_PANEL_FLAG_SERIALIZABLE);
	naui_panel_enable_flags(uph_state.panels.resource_list, NAUI_PANEL_FLAG_SERIALIZABLE);
	naui_panel_enable_flags(uph_state.panels.mixer, NAUI_PANEL_FLAG_SERIALIZABLE);
	naui_panel_enable_flags(uph_state.panels.resource_editor, NAUI_PANEL_FLAG_SERIALIZABLE);
	naui_panel_enable_flags(uph_state.panels.sample_browser, NAUI_PANEL_FLAG_SERIALIZABLE);
 
	uph_file_dialog_register();
 
	uph_layouts_apply_saved();
 
	uph_settings_open_midi_ports(!settings_loaded);
	uph_action_initialize();
 
	Naui_Path recoverable_project = {0};
	const bool recoverable = uph_recovery_find_recoverable_project(UPHONIC_WORKSPACE_FOLDER, &recoverable_project);
	uph_project_create(uph_project_untitled_name());

	const Naui_Path reopen_path = uph_state.settings.general.reopen_project;
	if (reopen_path.data[0])
	{
		uph_state.settings.general.reopen_project = (Naui_Path){ 0 };
		uph_settings_save();

		if (!uph_project_load(&uph_state.project, reopen_path))
			naui_log(NAUI_LOG_ERROR, "Failed to reopen project after restart: %s", reopen_path.data);
	}
 
	if (recoverable)
		uph_show_restore_dialog(recoverable_project);
}
 
void naui_app_end(void)
{
	uph_recovery_release_lock();
	uph_plugin_scan_shutdown();
	uph_sample_browser_shutdown();
	uph_project_flush_pending_samples(false);
	naui_jobs_shutdown();

	naui_log(NAUI_LOG_INFO, "Saving Settings...");
	uph_settings_save();
 
	naui_log(NAUI_LOG_INFO, "Closing Audio Engine...");
	uph_audio_engine_shutdown();
	uph_tempo_map_shutdown();
 
	naui_log(NAUI_LOG_INFO, "Destoying Actions...");
	naui_action_clear_history();
 
	naui_log(NAUI_LOG_INFO, "Clearing Tracks...");
	uph_resources_clear_tracks();

	naui_log(NAUI_LOG_INFO, "Stopping Plugin Host...");
	uph_plugin_modules_shutdown();
 
	naui_log(NAUI_LOG_INFO, "Stopping MIDI Engine...");
	cmidi_scheduler_stop_all();
	cmidi_shutdown();
 
	naui_log(NAUI_LOG_INFO, "Thanks For Using Uphonic.");

#if NAUI_WINDOWS
	fflush(NULL);
	TerminateProcess(GetCurrentProcess(), 0);
#endif
}
 
static void uph_update_track_plugin(Uph_Track *track, Uph_Plugin *plugin)
{
	const uint64_t load_ticket = uph_plugin_load_ticket(plugin);
	uph_plugin_update(plugin);
	if (load_ticket && !uph_plugin_loading(plugin))
		uph_resources_link_pending_automation(track, load_ticket, plugin);
}

static void uph_update_track(Uph_Track *track)
{
	uph_level_meter_smooth(track);

	uph_update_track_plugin(track, &track->instrument);

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->effects); i++)
		uph_update_track_plugin(track, &track->effects[i].plugin);
 
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
		uph_update_track(&track->subtracks[i]);
}
 
static void uph__update_panel_titles(void)
{
	const Uph_Panels *panels = &uph_state.panels;
	naui_panel_set_title(panels->song_timeline, NAUI_TR("song_timeline.title"));
	naui_panel_set_title(panels->resource_editor, NAUI_TR("resource_editor.title"));
	naui_panel_set_title(panels->resource_list, NAUI_TR("resource_list.title"));
	naui_panel_set_title(panels->mixer, NAUI_TR("mixer.title"));
	naui_panel_set_title(panels->settings, NAUI_TR("settings.title"));
	naui_panel_set_title(panels->plugin_list, NAUI_TR("plugin.menu.title"));
	naui_panel_set_title(panels->automation_list, NAUI_TR("automation.menu.title"));
	naui_panel_set_title(panels->sample_browser, NAUI_TR("sample_browser.title"));
	naui_panel_set_title(panels->render, NAUI_TR("render.title"));
}

void naui_app_update(void)
{
	uph_mixer_apply_pending_move();
	uph_song_timeline_apply_pending_move();
	uph_song_timeline_update_record_track();
	uph_tempo_map_update();
	uph_audio_engine_update();
	uph_project_update_pending_samples();

	uph__update_panel_titles();
	naui_keybind_set_enabled(!uph_dialog_blocks_input() && !uph_ui_any_textfield_active() && !uph_plugin_editor_take_keyboard());
 
	uph_render_update();
	uph_render_main_titlebar();
 
	naui_render_panels_and_viewport();
	uph_sample_browser_end_frame();
	uph_dialog_render();
	uph_ui_widgets_flush();
	uph_drag_flush();
 
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
        uph_update_track(&uph_state.project.tracks[i]);
 
	const bool midi_editor_open = uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN
		&& uph_note_pattern(uph_state.shared.selected_resource.index);
	const bool midi_editor_focused = midi_editor_open && naui_panel_focused() == uph_state.panels.resource_editor;
	const bool tracks_monitoring = uph__any_track_monitors_instrument(uph_state.project.tracks);
	uph_midi_preview_set_voice_monitoring(midi_editor_focused || (midi_editor_open && !tracks_monitoring));
	const bool voice_monitoring = uph_midi_preview_voice_monitoring();
	atomic_store_explicit(&uph__midi_instrument_monitoring, voice_monitoring || tracks_monitoring, memory_order_relaxed);
	uph_autosave_update();
}
 
void naui_app_event(const Naui_AppEventData *data)
{
	if (data->type == NAUI_APP_EVENT_WINDOW_CLOSE_REQUEST)
		uph_handle_close_request();
	else if (data->type == NAUI_APP_EVENT_FILE_DROP && !uph_dialog_blocks_input())
	{
		for (uint32_t i = 0; i < data->file_drop.path_count; i++)
			uph_project_add_file(&uph_state.project, NAUI_PATH(data->file_drop.paths[i]));
	}
}