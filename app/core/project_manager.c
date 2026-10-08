#define UPHONIC_FOLDER naui_path_join(naui_directory_get(NAUI_DIR_APPDATA), NAUI_PATH("Uphonic"))
#define UPHONIC_WORKSPACE_FOLDER naui_path_join(UPHONIC_FOLDER, NAUI_PATH("workspace"))
#define UPHONIC_SETTINGS_FILE naui_path_join(UPHONIC_FOLDER, NAUI_PATH(UPH_IO_FILE_SETTINGS))

#pragma region Duplicate Detection

static bool uph_path_is_inside(const Naui_Path path, const Naui_Path folder)
{
	const Naui_Path norm_path = naui_path_normalize(path);
	const Naui_Path norm_folder = naui_path_normalize(folder);
	const size_t folder_len = strlen(norm_folder.data);

	if (folder_len == 0)
		return false;

	if (strncmp(norm_path.data, norm_folder.data, folder_len) != 0)
		return false;

	return norm_folder.data[folder_len - 1] == '/' || norm_path.data[folder_len] == '/';
}

static int32_t uph_project_find_duplicate_sample(const Uph_Project *project, const Naui_Path file_path)
{
	const size_t incoming_size = naui_file_size(file_path);
	if (incoming_size == 0)
		return -1;

	const Naui_Path project_folder = uph_project_get_path(project);
	Uph_FileSignature incoming = {0};
	bool has_incoming = false;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(project->sample_data); i++)
	{
		const Naui_Path existing_path = project->sample_data[i].file_path;
		if (!existing_path.data[0] || !uph_path_is_inside(existing_path, project_folder))
			continue;

		if (naui_file_size(existing_path) != incoming_size)
			continue;

		if (!has_incoming)
		{
			if (!uph_file_signature(file_path, &incoming))
				return -1;
			has_incoming = true;
		}

		Uph_FileSignature existing;
		if (!uph_file_signature(existing_path, &existing))
			continue;

		if (uph_file_signature_equal(existing, incoming) && uph_files_identical(file_path, existing_path))
			return (int32_t)i;
	}

	return -1;
}

static void uph_project_select_sample_by_data(const Uph_Project *project, uint32_t data_index)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(project->samples); i++)
	{
		if (project->samples[i].data_index != data_index)
			continue;

		uph_state.shared.selected_resource.index = i;
		uph_state.shared.selected_resource.type = UPH_RESOURCE_SAMPLE;
		uph_state.shared.selected_resource.renaming = false;
		return;
	}
}

static void uph_project_rebase_sample_paths(Uph_Project *project, const Naui_Path from_folder, const Naui_Path to_folder)
{
	const Naui_Path norm_from = naui_path_normalize(from_folder);
	const size_t from_len = strlen(norm_from.data);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(project->sample_data); i++)
	{
		Naui_Path *path = &project->sample_data[i].file_path;
		if (!path->data[0] || !uph_path_is_inside(*path, from_folder))
			continue;

		const Naui_Path norm_path = naui_path_normalize(*path);
		const char *relative = norm_path.data + from_len;
		while (*relative == '/')
		{
			relative++;
		}

		*path = naui_path_normalize(naui_path_join(to_folder, naui_path_from_cstr(relative)));
	}
}

#pragma endregion

#pragma region Project Manager
static void _uph_project_default_shared_state()
{
	uph_state.shared.song_timeline_playing = false;
	uph_state.shared.selected_resource.index = 0;
	uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
	uph_state.shared.song_timeline_current_block_length = 4;
}

bool uph_project_create(Naui_String project_name)
{

	Naui_Path project_dest = naui_path_join(UPHONIC_WORKSPACE_FOLDER, NAUI_PATH(project_name.data));
	// if (naui_path_exists(project_dest))
	// {
	// 	naui_log(NAUI_LOG_ERROR, "Project name already exists: %s", project_name.data);
	// 	return false;
	// }

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
	uph_resources_add_track(naui_string_from_cstr(NAUI_TR("song_timeline.track.title")));
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

	// The autosave is only trustworthy once a save has fully finished, so drop its marker before touching anything.
	// For a canonical save this also keeps the marker from being merged into the project folder
	uph_recovery_invalidate_autosave(project_folder);
	
	if (save_type == UPH_SAVE_TYPE_CANONICAL)
	{
		uint64_t current_time = naui_unix_time();
		uph_state._last_autosave_time = current_time;
		uph_state._last_modified_time = current_time;
		
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
		uph_autosave_mark_clean();
	else if (saved)
		uph_recovery_commit_autosave(project_folder);

	return saved;
}

bool uph_project_export(Uph_Project* project, const Naui_Path output_path, Uph_ExportFormat format)
{
	if (format == UPH_EXPORT_UPH)
	{
		naui_log(NAUI_LOG_INFO, "Making UPH file: %s", output_path.data);

		Naui_Path output_parent = naui_path_parent(output_path);
		if (!naui_directories_create(output_parent))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to create UPH directory.");
			return false;
		}

		const Naui_Path canonical_save = uph_project_get_path(project);
		const Naui_Path temp_save = naui_path_join(canonical_save, NAUI_PATH(UPH_PATH_TEMP));
		naui_path_unlock(canonical_save);

		bool success = true;
		Naui_Archive archive = NAUI_ARCHIVE_INIT;
		naui_archive_open(&archive, output_path, NAUI_ARCHIVE_MODE_WRITE);
		if (!naui_archive_is_valid(&archive))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to create archive");
			success = false;
		}

		if (success && !naui_archive_add_folder(&archive, canonical_save, NAUI_PATH(""), NAUI_ARCHIVE_EXCLUDES(".temp", ".lock")))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to add canonical folder to archive");
			success = false;
		}

		if (success && naui_path_exists(temp_save) && !naui_archive_add_folder(&archive, temp_save, NAUI_PATH(""), NAUI_ARCHIVE_EXCLUDES(UPH_RECOVERY_AUTOSAVE_MARKER)))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to add temp folder to archive");
			success = false;
		}

		naui_archive_close(&archive);
		// if (success && !naui_archive_compact(output_path))
		// 	naui_log(NAUI_LOG_WARNING, "Failed to compact exported archive at: %s", output_path.data);

		naui_path_lock(canonical_save);

		if (!success)
		{
			naui_file_delete(output_path);
			return false;
		}

		naui_log(NAUI_LOG_INFO, "Project(%s) saved to: %s", project->title.data, output_path.data);
		return true;
	}
	else if (format == UPH_EXPORT_WAV)
	{
		double length = uph_audio_engine_get_song_length_beats();
		if (fabs(length) < NAUI_EPSILON)
		{
			naui_log(NAUI_LOG_WARNING, "Unable to export WAV: project contains no audio to render.");
			return false;
		}

		uph_audio_engine_stop();
		const bool exported = uph_audio_engine_export_to_wav(output_path.data, 0, length);
		uph_audio_engine_start();
		return exported;
	}

	return false;
}

bool uph_project_load(Uph_Project* project, const Naui_Path project_path)
{
	uph_state.shared.song_timeline_playing = false;
	Naui_Path load_path = project_path;
	Naui_Archive archive = NAUI_ARCHIVE_INIT;

	if (!naui_path_is_directory(project_path))
		naui_archive_open(&archive, project_path, NAUI_ARCHIVE_MODE_READ);

	uph_audio_engine_stop();
	if (naui_archive_is_valid(&archive))
	{
		Naui_String filename = naui_view_to_string(naui_file_stem(&project_path));
		load_path = naui_path_join(UPHONIC_WORKSPACE_FOLDER, naui_path_from_cstr(filename.data));
		if(!naui_directories_create(load_path))
		{
			naui_log(NAUI_LOG_WARNING, "Project folder failed to create (%s)", load_path.data);
			// Resolve false condition, duplicate project names. MUST BE UNIQUE!
		}
		
		bool extracted = naui_archive_extract_to(&archive, load_path);
		if (extracted)
		{
			if (!uph_io_load_project(project, load_path))
			{
				naui_log(NAUI_LOG_ERROR, "Failed to create project from UPH file");
				naui_directory_remove_all(load_path);
				uph_audio_engine_start();
				return false;
			}

			naui_action_clear_history();
			uph_autosave_mark_clean();
			_uph_project_default_shared_state();
			uph_recovery_acquire_lock(uph_project_get_path(project));
			naui_log(NAUI_LOG_INFO, "Successfully loaded uph file: %s", filename.data);
		}
		else
			naui_log(NAUI_LOG_ERROR, "Failed to load UPH file at: %s", project_path);

		uph_audio_engine_start();
		return extracted;
	}

	uph_audio_engine_stop_all_notes();
	naui_list_clear(project->midi_patterns);
	uph_resources_clear_tracks();

	// An autosave in .temp is newer than the project folder, so it wins. It is NOT merged here: the project
	// folder stays the last explicit save until the user saves (or discards the autosave by not saving)
	bool from_autosave = uph_recovery_has_autosave(load_path);
	bool loaded = uph_io_load_project(project, from_autosave ? naui_path_join(load_path, NAUI_PATH(UPH_PATH_TEMP)) : load_path);

	if (!loaded && from_autosave)
	{
		naui_log(NAUI_LOG_WARNING, "Autosave could not be read, loading the last saved version instead");
		naui_list_clear(project->midi_patterns);
		uph_resources_clear_tracks();
		uph_resources_remove_all_automation();
		from_autosave = false;
		loaded = uph_io_load_project(project, load_path);
	}

	if (loaded)
	{
		// The folder is the project's identity (saves, autosaves and the lock all go to workspace/<title>),
		// so a folder that was renamed wins over the name stored inside the project file
		const Naui_Path normalized_load_path = naui_path_normalize(load_path);
		project->title = naui_view_to_string(naui_file_filename(&normalized_load_path));

		naui_action_clear_history();
		if (from_autosave)
		{
			uph_autosave_mark_recovered();
			naui_log(NAUI_LOG_INFO, "Restored unsaved changes from the autosave");
		}
		else
			uph_autosave_mark_clean();

		_uph_project_default_shared_state();
		uph_recovery_acquire_lock(uph_project_get_path(project));
	}

	if (naui_list_len(project->tracks) == 0)
		uph_resources_add_track(naui_string_from_cstr(NAUI_TR("song_timeline.track.title")));

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

	const char* midi_extensions[] = { ".mid", ".midi", ".rmi", ".kar" };
	const size_t midi_extension_count = sizeof(midi_extensions) / sizeof(midi_extensions[0]);

	Naui_StringView extension = naui_file_extension(&file_path);
	for (size_t e = 0; e < midi_extension_count; e++)
	{
		if (naui_string_view_equals_cstr(extension, midi_extensions[e], true))
		{
			// const int32_t duplicate = uph_project_find_duplicate_sample(project, file_path);	Make Generic version to check if two files are the same
			// if (duplicate >= 0)
			// {
			// 	naui_log(NAUI_LOG_INFO, "Skipping copy, identical file already in project: %s", file_path.data);
			// 	return true;
			// }

			Naui_StringView pattern_name = naui_file_stem(&file_path);
			Uph_MidiPattern pattern = uph_midi_convert_to_pattern(file_path);
			if (naui_list_len(pattern.notes) == 0)
			{
				naui_log(NAUI_LOG_WARNING, "Failed to load MIDI Pattern (%s)", file_path.data);
				return false;
			}

			naui_string_copy_view(&pattern.name, pattern_name);
			naui_list_push(project->midi_patterns, pattern);
			uph_state.shared.selected_resource.index = naui_list_len(project->midi_patterns) - 1;
			uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
			naui_log(NAUI_LOG_INFO, "Added MIDI Pattern (%s)", file_path.data);
			return true;
		}
	}

	naui_log(NAUI_LOG_INFO, "(%s) Adding sample link: %s", project->title, file_path.data);
	if (!uph_state.settings.general.copy_resources)
		return uph_resources_add_sample_from_file(file_path);

	const int32_t duplicate = uph_project_find_duplicate_sample(project, file_path);
	if (duplicate >= 0)
	{
		naui_log(NAUI_LOG_INFO, "Skipping copy, identical file already in project: %s", project->sample_data[duplicate].file_path.data);
		uph_project_select_sample_by_data(project, (uint32_t)duplicate);
		return true;
	}

	naui_log(NAUI_LOG_INFO, "Copying to temp: %s", file_path.data);
	Naui_Path copy_path = naui_path_join(uph_project_get_path(project), NAUI_PATH(".temp", UPH_IO_FOLDER_SAMPLES));
	naui_directories_create(copy_path);

	Naui_Path file_dest = naui_file_unique_name(file_path, copy_path);
	naui_file_copy(file_path, file_dest, NAUI_FILE_COPY_OVERRIDE); 	// Use override since unique will do more work while not returning the file dest
	if (!uph_resources_add_sample_from_file(file_dest))	// Inefficient, but works
	{
		naui_file_delete(file_dest);
		return false;
	}

	return true;
}

Naui_Path uph_project_get_path(const Uph_Project* project)
{
	return naui_path_normalize(naui_path_join(UPHONIC_WORKSPACE_FOLDER, NAUI_PATH(project->title.data)));
}
#pragma endregion