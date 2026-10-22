static void _uph_project_default_shared_state()
{
	uph_state.shared.song_timeline_playing = false;
	uph_state.shared.selected_resource.index = 0;
	uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
	uph_state.shared.song_timeline_current_block_length = 4;
}

static void _uph_project_adopt_folder_files(Uph_Project *project, const Naui_Path project_folder)
{
	uph_project_adopt_sample_files(project, project_folder);
	uph_project_adopt_pattern_files(project, project_folder);
}

void uph_project_close(Uph_Project *project)
{
	if (naui_string_is_empty(project->title))
		return;

	uph_project_sweep_deleted_samples(project);

	const Naui_Path folder = uph_project_get_path(project);
	if (uph_project_name_is_untitled(project->title.data) && uph_project_folder_is_pristine(folder))
	{
		uph_recovery_release_lock();
		naui_directory_remove_all(folder);
	}
}

bool uph_project_create(Naui_String project_name)
{
	uph_project_close(&uph_state.project);
	uph_project_remove_abandoned_untitled();
	uph_project_clear_adopted_samples();

	const Naui_Path project_dest = uph_project_unique_workspace_folder(project_name.data);
	project_name = naui_view_to_string(naui_file_filename(&project_dest));

	if (!naui_directories_create(project_dest))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to create new Uphonic project: %s", project_name.data);
		return false;
	}

	uph_audio_engine_stop();
	uint64_t current_time = naui_unix_time();
	uph_state.project.bpm = 120.0f;
	uph_state.project.time_created = current_time;
	uph_state.project.last_accessed = current_time;
	uph_state.project.last_modified = current_time;
	uph_state.project.title = project_name;
	uph_state.project.time_signature = (Uph_TimeSignature){ .numerator = 4, .denominator = 4 };
	naui_list_clear(uph_state.project.midi_patterns);
	naui_list_clear(uph_state.project.samples);

	uph_resources_clear_tracks();
	uph_resources_init_master_track();
	uph_resources_add_track(naui_string_from_cstr(NAUI_TR("song_timeline.track.title")), 0);
	uph_resources_add_pattern();
	uph_resources_remove_all_automation();
	naui_action_clear_history();
	uph_autosave_mark_clean();

	uph_state.shared.song_timeline_playhead_position = 0.0;
	_uph_project_default_shared_state();

	if (!uph_recovery_acquire_lock(project_dest))
		naui_log(NAUI_LOG_WARNING, "Project may be open in another Uphonic instance: %s", project_dest.data);

	uph_audio_engine_start();
	return true;
}

bool uph_project_save(Uph_Project* project, Uph_SaveType save_type)
{
	uph_audio_engine_stop();
	const Naui_Path project_folder = uph_project_get_path(project);
	const Naui_Path temp_folder = naui_path_join(project_folder, NAUI_PATH(UPH_PATH_TEMP)); // May not exist. CHECK!
	const Naui_Path save_dest = (save_type == UPH_SAVE_TYPE_CANONICAL) ? project_folder : temp_folder;
	Naui_List(Naui_String) sample_candidates = NULL;
	uph_recovery_invalidate_autosave(project_folder);
	
	if (save_type == UPH_SAVE_TYPE_CANONICAL)
	{
		uint64_t current_time = naui_unix_time();
		uph_state._last_autosave_time = current_time;
		uph_state._last_modified_time = current_time;
		sample_candidates = uph_project_collect_sample_candidates(project_folder, temp_folder);
		
		if(naui_path_exists(temp_folder))
		{
			if (naui_directory_merge(temp_folder, project_folder, NAUI_FILE_COPY_OVERRIDE))
				uph_project_rebase_sample_paths(project, temp_folder, project_folder);

			naui_directory_remove_all(temp_folder);
		}
	}
	
	uph_audio_engine_start();
	const bool saved = uph_io_save_project(project, save_dest);
	if (saved && save_type == UPH_SAVE_TYPE_CANONICAL)
	{
		uph_project_queue_dropped_samples(project, project_folder, sample_candidates);
		uph_autosave_mark_clean();
	}
	else if (saved)
		uph_recovery_commit_autosave(project_folder);

	naui_list_free(sample_candidates);
	return saved;
}

bool uph_project_load(Uph_Project* project, const Naui_Path project_path)
{
	uph_state.shared.song_timeline_playing = false;
	uph_audio_engine_stop();

	Naui_Path load_path = project_path;
	const bool from_uph = !naui_path_is_directory(project_path);
	bool reused_copy = false;
	if (from_uph && !uph_project_extract_uph(project_path, &load_path, &reused_copy))
	{
		uph_audio_engine_start();
		return false;
	}

	if (reused_copy && !naui_string_is_empty(project->title) && strcmp(naui_path_normalize(uph_project_get_path(project)).data, naui_path_normalize(load_path).data) == 0)
	{
		naui_log(NAUI_LOG_INFO, "This file's project is already open: %s", project->title.data);
		uph_audio_engine_start();
		return true;
	}

	uph_project_close(project);
	uph_project_clear_adopted_samples();

	uph_audio_engine_stop_all_notes();
	naui_list_clear(project->midi_patterns);
	uph_resources_remove_all_automation();
	uph_resources_clear_tracks();

	bool from_autosave = uph_recovery_has_autosave(load_path);
	bool loaded = uph_io_load_project(project, from_autosave ? naui_path_join(load_path, NAUI_PATH(UPH_PATH_TEMP)) : load_path);

	if (!loaded && from_autosave)
	{
		naui_log(NAUI_LOG_WARNING, "Could not read autosave, loading the last saved version...");
		naui_list_clear(project->midi_patterns);
		uph_resources_clear_tracks();
		uph_resources_remove_all_automation();
		from_autosave = false;
		loaded = uph_io_load_project(project, load_path);
	}

	if (loaded)
	{
		const Naui_Path normalized_load_path = naui_path_normalize(load_path);
		project->title = naui_view_to_string(naui_file_filename(&normalized_load_path));
		uph_project_sweep_deleted_samples(project);	// a crash can leave a pending delete list behind
		naui_action_clear_history();

		if (from_autosave)
		{
			uph_autosave_mark_recovered();
			naui_log(NAUI_LOG_INFO, "Restored unsaved changes from the autosave");
		}
		else
		{
			_uph_project_adopt_folder_files(project, normalized_load_path);
			uph_autosave_mark_clean();
		}

		_uph_project_default_shared_state();
		uph_state.shared.song_timeline_playhead_position = 0.0;
		uph_recovery_acquire_lock(uph_project_get_path(project));
	}
	else if (from_uph && !reused_copy)
		naui_directory_remove_all(load_path);

	if (naui_list_len(project->tracks) == 0)
		uph_resources_add_track(naui_string_from_cstr(NAUI_TR("song_timeline.track.title")), 0);

	uph_state.shared.song_timeline_playing = false;
	uph_state.shared.selected_resource.index = 0;
	uph_state.shared.selected_resource.type = naui_list_len(project->midi_patterns) > 0 ? UPH_RESOURCE_PATTERN : UPH_RESOURCE_NONE;
	uph_audio_engine_start();
	return loaded;
}

bool uph_project_add_file(Uph_Project* project, const Naui_Path file_path)
{
	if (!naui_path_exists(file_path) || naui_path_is_directory(file_path) || naui_string_is_empty(project->title))
		return false;

	if (uph_project_is_midi_extension(naui_file_extension(&file_path)))
		return uph_project_add_pattern_file(project, file_path);

	return uph_project_add_sample_file(project, file_path);
}

Naui_Path uph_project_get_path(const Uph_Project* project)
{
	return naui_path_normalize(naui_path_join(UPHONIC_WORKSPACE_FOLDER, NAUI_PATH(project->title.data)));
}