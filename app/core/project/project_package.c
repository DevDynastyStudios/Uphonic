#define UPH_ORIGIN_FILE ".origin"

typedef struct
{
	Naui_Path relative;
	Naui_Path source;
}
Uph_ExportFile;

static bool _uph_is_audio_extension(const Naui_StringView extension)
{
	for (const char **audio = uph_audio_engine_extensions; *audio; audio++)
	{
		if (naui_string_view_equals_cstr(extension, *audio, false))
			return true;
	}

	return false;
}

// Adds a file, or replaces the one already there with the same archive path
static void _uph_export_set(Naui_List(Uph_ExportFile) *files, const Naui_Path relative, const Naui_Path source)
{
	for (size_t i = 0; i < naui_list_len(*files); i++)
	{
		if (strcmp((*files)[i].relative.data, relative.data) == 0)
		{
			(*files)[i].source = source;
			return;
		}
	}

	const Uph_ExportFile file = { .relative = relative, .source = source };
	naui_list_push(*files, file);
}

static void _uph_export_collect(Naui_List(Uph_ExportFile) *files, const Naui_Path folder)
{
	Naui_List(Naui_DirEntry) entries = naui_directory_filter_recursive(folder, NULL, NULL);
	for (size_t i = 0; i < naui_list_len(entries); i++)
	{
		if (entries[i].is_directory)
			continue;

		const char *relative = entries[i].path.data + strlen(folder.data);
		if (*relative == '/' || *relative == '\\')
			relative++;

		if (relative[0] == '.')
			continue;

		Naui_Path name = naui_path_from_cstr(relative);
		for (char *c = name.data; *c; c++)
		{
			if (*c == '\\')
				*c = '/';
		}

		_uph_export_set(files, name, entries[i].path);
	}

	naui_directory_filter_free(entries);
}

static bool _uph_export_file_is_unused_sample(const Uph_Project *project, const Naui_Path relative)
{
	const size_t folder_length = strlen(UPH_IO_FOLDER_SAMPLES) + 1;
	if (strncmp(relative.data, UPH_IO_FOLDER_SAMPLES "/", folder_length) != 0 || strchr(relative.data + folder_length, '/'))
		return false;

	if (!_uph_is_audio_extension(naui_file_extension(&relative)))
		return false;

	return !uph_project_references_sample_file(project, relative.data + folder_length);
}

static bool _uph_export_file_is_unused_pattern(Naui_List(Naui_String) defined_patterns, const Naui_Path relative)
{
	const size_t folder_length = strlen(UPH_IO_FOLDER_PATTERNS) + 1;
	if (strncmp(relative.data, UPH_IO_FOLDER_PATTERNS "/", folder_length) != 0 || strchr(relative.data + folder_length, '/'))
		return false;

	return uph_project_is_midi_extension(naui_file_extension(&relative)) && !uph_project_names_contain(defined_patterns, relative.data + folder_length);
}

static void _uph_project_write_origin(const Naui_Path folder, const Uph_FileSignature signature, const Naui_Path uph_path)
{
	char text[NAUI_PATH_MAX + 64];
	const int length = snprintf(text, sizeof(text), "%llu %llu\n%s\n", (unsigned long long)signature.size, (unsigned long long)signature.hash, uph_path.data);
	if (length > 0)
		naui_file_write_all(naui_path_join(folder, NAUI_PATH(UPH_ORIGIN_FILE)), text, (size_t)length);
}

static bool _uph_project_find_unpacked_copy(const Uph_FileSignature signature, Naui_Path *out_folder)
{
	Naui_List(Naui_DirEntry) entries = naui_directory_filter(UPHONIC_WORKSPACE_FOLDER, NULL, NULL);
	bool found = false;

	for (size_t i = 0; i < naui_list_len(entries) && !found; i++)
	{
		if (!entries[i].is_directory)
			continue;

		size_t size = 0;
		char *text = naui_file_read_all(naui_path_join(entries[i].path, NAUI_PATH(UPH_ORIGIN_FILE)), &size);
		if (!text)
			continue;

		unsigned long long file_size = 0, file_hash = 0;
		const bool same_file = sscanf(text, "%llu %llu", &file_size, &file_hash) == 2 && file_size == signature.size && file_hash == signature.hash;
		free(text);

		if (same_file && naui_path_exists(naui_path_join(entries[i].path, NAUI_PATH(UPH_IO_FILE_PROJECT))))
		{
			*out_folder = entries[i].path;
			found = true;
		}
	}

	naui_directory_filter_free(entries);
	return found;
}

// Export the project as it is right now
static bool _uph_project_export_uph(Uph_Project *project, const Naui_Path output_path)
{
	naui_log(NAUI_LOG_INFO, "Making UPH file: %s", output_path.data);

	if (!naui_directories_create(naui_path_parent(output_path)))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to create UPH directory.");
		return false;
	}

	const Naui_Path canonical_save = uph_project_get_path(project);
	const Naui_Path temp_save = naui_path_join(canonical_save, NAUI_PATH(UPH_PATH_TEMP));
	const bool had_temp = naui_path_exists(temp_save);

	if (!uph_project_save(project, UPH_SAVE_TYPE_TEMP))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to write the current state of the project");
		return false;
	}

	Naui_List(Naui_String) defined_patterns = uph_io_defined_pattern_files(naui_path_join(temp_save, NAUI_PATH(UPH_IO_FOLDER_PATTERNS)));

	Naui_List(Uph_ExportFile) files = NULL;
	_uph_export_collect(&files, canonical_save);
	if (naui_path_exists(temp_save))
		_uph_export_collect(&files, temp_save);

	naui_path_unlock(canonical_save);

	bool success = true;
	Naui_Archive archive = NAUI_ARCHIVE_INIT;
	naui_archive_open(&archive, output_path, NAUI_ARCHIVE_MODE_WRITE);
	if (!naui_archive_is_valid(&archive))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to create archive");
		success = false;
	}

	for (size_t i = 0; success && i < naui_list_len(files); i++)
	{
		if (_uph_export_file_is_unused_sample(project, files[i].relative) || _uph_export_file_is_unused_pattern(defined_patterns, files[i].relative))
		{
			naui_log(NAUI_LOG_INFO, "Leaving unused file out of the export: %s", files[i].relative.data);
			continue;
		}

		if (!naui_archive_add_file(&archive, files[i].source, files[i].relative))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to add file to archive: %s", files[i].relative.data);
			success = false;
		}
	}

	naui_archive_close(&archive);
	naui_path_lock(canonical_save);
	naui_list_free(files);
	naui_list_free(defined_patterns);

	if (!had_temp)
		uph_recovery_discard_autosave(canonical_save);

	if (!success)
	{
		naui_file_delete(output_path);
		return false;
	}

	naui_log(NAUI_LOG_INFO, "Project(%s) saved to: %s", project->title.data, output_path.data);
	return true;
}

// Unpacks into a new workspace folder, unless that exact file was unpacked before.
bool uph_project_extract_uph(const Naui_Path uph_path, Naui_Path *out_folder, bool *out_reused)
{
	*out_reused = false;

	Uph_FileSignature signature = {0};
	const bool has_signature = uph_file_signature(uph_path, &signature);
	if (has_signature && _uph_project_find_unpacked_copy(signature, out_folder))
	{
		*out_reused = true;
		return true;
	}

	Naui_Archive archive = NAUI_ARCHIVE_INIT;
	naui_archive_open(&archive, uph_path, NAUI_ARCHIVE_MODE_READ);
	if (!naui_archive_is_valid(&archive))
	{
		naui_log(NAUI_LOG_ERROR, "Not a valid UPH file: %s", uph_path.data);
		return false;
	}

	const Naui_String stem = naui_view_to_string(naui_file_stem(&uph_path));
	const Naui_Path folder = uph_project_unique_workspace_folder(stem.data[0] ? stem.data : "Project");

	bool ok = naui_directories_create(folder);
	if (!ok)
		naui_log(NAUI_LOG_ERROR, "Project folder failed to create (%s)", folder.data);
	else
		ok = naui_archive_extract_to(&archive, folder);

	naui_archive_close(&archive);

	if (ok && !naui_path_exists(naui_path_join(folder, NAUI_PATH(UPH_IO_FILE_PROJECT))))
	{
		naui_log(NAUI_LOG_ERROR, "UPH file does not contain a project: %s", uph_path.data);
		ok = false;
	}

	if (!ok)
	{
		if (naui_path_exists(folder))
			naui_directory_remove_all(folder);

		return false;
	}

	naui_file_delete(naui_path_join(folder, NAUI_PATH(UPH_PENDING_DELETE_FILE)));
	naui_file_delete(naui_path_join(folder, NAUI_PATH(UPH_RECOVERY_LOCK_FILE)));
	if (has_signature)
		_uph_project_write_origin(folder, signature, uph_path);

	*out_folder = folder;
	return true;
}

bool uph_project_export(Uph_Project* project, const Naui_Path output_path, Uph_ExportFormat format)
{
	if (format == UPH_EXPORT_UPH)
		return _uph_project_export_uph(project, output_path);
	else if (format == UPH_EXPORT_WAV)
	{
		double length = uph_audio_engine_get_song_length_beats();
		if (fabs(length) < NAUI_EPSILON)
		{
			naui_log(NAUI_LOG_WARNING, "Unable to export WAV: project contains no audio to render.");
			return false;
		}

		uph_audio_engine_lock();
		const bool exported = uph_audio_engine_export_to_wav(output_path.data, 0, length);
		uph_audio_engine_unlock();
		return exported;
	}

	return false;
}