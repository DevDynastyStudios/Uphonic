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
	const Naui_Path old_folder = uph_project_get_path(&uph_state.project);
	const bool discard_old = !naui_string_is_empty(uph_state.project.title) && !uph_project_has_been_saved(&uph_state.project);

	uph_project_close(&uph_state.project);
	if (discard_old && naui_path_exists(old_folder))
	{
		uph_recovery_release_lock();
		uph_recovery_discard_project(old_folder);
	}

	uph_project_remove_abandoned_untitled();
	uph_project_clear_adopted_samples();

	const Naui_Path project_dest = uph_project_unique_workspace_folder(project_name.data);
	project_name = naui_view_to_string(naui_file_filename(&project_dest));

	if (!naui_directories_create(project_dest))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to create new Uphonic project: %s", project_name.data);
		return false;
	}

	uph_audio_engine_lock();
	uint64_t current_time = naui_unix_time();
	uph_state.project.bpm = 120.0f;
	uph_state.project.time_created = current_time;
	uph_state.project.last_accessed = current_time;
	uph_state.project.last_modified = current_time;
	uph_state.project.title = project_name;
	uph_state.project.folder = naui_path_normalize(project_dest);
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
	uph_song_timeline_reset_scroll();
	uph_midi_editor_reset_scroll();

	if (!uph_recovery_acquire_lock(project_dest))
		naui_log(NAUI_LOG_WARNING, "Project may be open in another Uphonic instance: %s", project_dest.data);

	uph_audio_engine_unlock();
	return true;
}

bool uph_project_save(Uph_Project* project, Uph_SaveType save_type)
{
	uph_audio_engine_lock();
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
	
	uph_audio_engine_unlock();
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

bool uph_project_has_been_saved(const Uph_Project* project)
{
	const Naui_Path folder = uph_project_get_path(project);
	if (naui_string_is_empty(project->title) || !folder.data[0])
		return false;

	return naui_path_exists(naui_path_join(folder, NAUI_PATH(UPH_IO_FILE_PROJECT)));
}

#pragma region Save As

static Naui_String _uph_project_clean_name(const char *name)
{
	char buffer[NAUI_STRING_MAX_SIZE];
	snprintf(buffer, sizeof(buffer), "%s", name ? name : "");

	size_t length = strlen(buffer);
	const size_t extension_length = strlen(".uph");
	if (length > extension_length)
	{
		const char *tail = buffer + length - extension_length;
		bool is_extension = true;
		for (size_t i = 0; i < extension_length && is_extension; i++)
			is_extension = tolower((unsigned char)tail[i]) == ".uph"[i];

		if (is_extension)
			buffer[length -= extension_length] = '\0';
	}

	while (length > 0 && (buffer[length - 1] == ' ' || buffer[length - 1] == '.'))
		buffer[--length] = '\0';

	const char *start = buffer;
	while (*start == ' ')
		start++;

	Naui_String clean = naui_string_from_cstr(start);
	for (char *c = clean.data; *c; c++)
	{
		if ((unsigned char)*c < 0x20 || strchr("<>:\"/\\|?*", *c))
			*c = '_';
	}

	return clean;
}

static void _uph_project_adopt_new_folder(Uph_Project *project, const Naui_Path old_folder, const Naui_Path new_folder)
{
	uph_audio_engine_lock();
	project->title = naui_view_to_string(naui_file_filename(&new_folder));
	project->folder = naui_path_normalize(new_folder);
	uph_project_rebase_sample_paths(project, old_folder, new_folder);
	uph_audio_engine_unlock();

	if (!uph_recovery_acquire_lock(new_folder))
		naui_log(NAUI_LOG_WARNING, "Project may be open in another Uphonic instance: %s", new_folder.data);
}

static bool _uph_project_move_to_folder(Uph_Project *project, const Naui_Path old_folder, const Naui_Path new_folder)
{
	uph_recovery_release_lock();
	if (!naui_directory_rename(old_folder, new_folder))
	{
		if (!naui_directory_merge(old_folder, new_folder, NAUI_FILE_COPY_OVERRIDE))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to move the project folder to: %s", new_folder.data);
			naui_directory_remove_all(new_folder);
			uph_recovery_acquire_lock(old_folder);
			return false;
		}

		naui_directory_remove_all(old_folder);
	}

	_uph_project_adopt_new_folder(project, old_folder, new_folder);
	return true;
}

static bool _uph_project_copy_to_folder(Uph_Project *project, const Naui_Path old_folder, const Naui_Path new_folder)
{
	uph_recovery_release_lock();
	if (!naui_directory_merge(old_folder, new_folder, NAUI_FILE_COPY_OVERRIDE))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to copy the project folder to: %s", new_folder.data);
		naui_directory_remove_all(new_folder);
		uph_recovery_acquire_lock(old_folder);
		return false;
	}

	const Naui_Path origin = naui_path_join(new_folder, NAUI_PATH(UPH_ORIGIN_FILE));
	if (naui_path_exists(origin))
		naui_file_delete(origin);

	_uph_project_adopt_new_folder(project, old_folder, new_folder);
	return true;
}

bool uph_project_save_as(Uph_Project* project, const Naui_Path destination)
{
	const Naui_String clean_name = _uph_project_clean_name(naui_view_to_string(naui_file_filename(&destination)).data);
	if (clean_name.length == 0)
		return false;

	const Naui_Path parent = naui_path_normalize(naui_path_parent(destination));
	const Naui_Path old_folder = uph_project_get_path(project);
	const Naui_Path chosen_folder = naui_path_normalize(naui_path_join(parent, naui_path_from_cstr(clean_name.data)));

	if (uph_project_names_equal(chosen_folder.data, naui_path_normalize(old_folder).data))
		return uph_project_save(project, UPH_SAVE_TYPE_CANONICAL);

	if (_uph_path_is_inside(chosen_folder, old_folder))
	{
		naui_log(NAUI_LOG_ERROR, "Can't save a project inside its own folder: %s", chosen_folder.data);
		return false;
	}

	if (!naui_path_exists(parent) && !naui_directories_create(parent))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to create the folder to save into: %s", parent.data);
		return false;
	}

	const Naui_Path new_folder = uph_project_unique_folder(parent, clean_name.data);
	const bool moving = !uph_project_has_been_saved(project);

	if (!moving && !uph_project_save(project, UPH_SAVE_TYPE_TEMP))
		return false;

	const bool relocated = moving ?
		_uph_project_move_to_folder(project, old_folder, new_folder) :
		_uph_project_copy_to_folder(project, old_folder, new_folder);

	if (!relocated)
		return false;

	const bool saved = uph_project_save(project, UPH_SAVE_TYPE_CANONICAL);
	if (saved && !moving)
		uph_recovery_discard_autosave(old_folder);

	return saved;
}

#pragma endregion

bool uph_project_load(Uph_Project* project, const Naui_Path project_path)
{
	uph_state.shared.song_timeline_playing = false;
	uph_audio_engine_lock();

	Naui_Path load_path = project_path;
	const bool from_uph = !naui_path_is_directory(project_path);
	bool reused_copy = false;
	if (from_uph && !uph_project_extract_uph(project_path, &load_path, &reused_copy))
	{
		uph_audio_engine_unlock();
		return false;
	}

	if (reused_copy && !naui_string_is_empty(project->title) && strcmp(naui_path_normalize(uph_project_get_path(project)).data, naui_path_normalize(load_path).data) == 0)
	{
		naui_log(NAUI_LOG_INFO, "This file's project is already open: %s", project->title.data);
		uph_audio_engine_unlock();
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
		project->folder = normalized_load_path;
		uph_project_sweep_deleted_samples(project);
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
		uph_song_timeline_reset_scroll();
		uph_midi_editor_reset_scroll();
		uph_recovery_acquire_lock(uph_project_get_path(project));
	}
	else if (from_uph && !reused_copy)
		naui_directory_remove_all(load_path);

	if (naui_list_len(project->tracks) == 0)
		uph_resources_add_track(naui_string_from_cstr(NAUI_TR("song_timeline.track.title")), 0);

	uph_state.shared.song_timeline_playing = false;
	uph_state.shared.selected_resource.index = 0;
	uph_state.shared.selected_resource.type = naui_list_len(project->midi_patterns) > 0 ? UPH_RESOURCE_PATTERN : UPH_RESOURCE_NONE;
	uph_audio_engine_unlock();
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
	if (project->folder.data[0])
		return project->folder;

	return (Naui_Path){ 0 };
}