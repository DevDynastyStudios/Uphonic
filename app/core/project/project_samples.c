static Naui_List(Naui_String) s_adopted_sample_files;

static bool _uph_path_is_inside(const Naui_Path path, const Naui_Path folder)
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

static bool _uph_is_plain_file_name(const char *name)
{
	return name[0] && strcmp(name, ".") != 0 && strcmp(name, "..") != 0 && !strpbrk(name, "/\\:");
}

static int32_t _uph_project_find_duplicate_sample(const Uph_Project *project, const Naui_Path file_path)
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
		if (!existing_path.data[0] || !_uph_path_is_inside(existing_path, project_folder))
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

static Naui_Path _uph_project_unique_sample_destination(const Uph_Project *project, const Naui_Path file_path)
{
	const Naui_Path project_folder = uph_project_get_path(project);
	const Naui_Path temp_dir = naui_path_join(project_folder, NAUI_PATH(UPH_PATH_TEMP, UPH_IO_FOLDER_SAMPLES));
	const Naui_Path saved_dir = naui_path_join(project_folder, NAUI_PATH(UPH_IO_FOLDER_SAMPLES));

	const Naui_String stem = naui_view_to_string(naui_file_stem(&file_path));
	const Naui_String extension = naui_view_to_string(naui_file_extension(&file_path));
	Naui_String name = naui_view_to_string(naui_file_filename(&file_path));

	for (int32_t counter = 1; ; counter++)
	{
		const Naui_Path in_temp = naui_path_join(temp_dir, naui_path_from_cstr(name.data));
		const Naui_Path in_saved = naui_path_join(saved_dir, naui_path_from_cstr(name.data));
		if (!naui_path_exists(in_temp) && !naui_path_exists(in_saved))
			return in_temp;

		name = naui_string_format("%s (%d)%s", stem.data, counter, extension.data);
	}
}

static void _uph_project_select_sample_by_data(const Uph_Project *project, uint32_t data_index)
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

static bool _uph_project_has_sample_file(const Uph_Project *project, const Naui_Path file_path)
{
	const Naui_Path wanted = naui_path_normalize(file_path);
	const Naui_StringView wanted_view = { .data = (char *)wanted.data, .length = strlen(wanted.data) };

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(project->sample_data); i++)
	{
		const Naui_Path existing = naui_path_normalize(project->sample_data[i].file_path);
		if (naui_string_view_equals_cstr(wanted_view, existing.data, UPH_FILE_NAMES_CASE_SENSITIVE))
			return true;
	}

	return false;
}

static Naui_Path _uph_pending_delete_path(const Naui_Path project_folder)
{
	return naui_path_join(project_folder, NAUI_PATH(UPH_PENDING_DELETE_FILE));
}

static Naui_List(Naui_String) _uph_pending_delete_read(const Naui_Path project_folder)
{
	Naui_List(Naui_String) names = NULL;
	size_t size = 0;
	char *text = naui_file_read_all(_uph_pending_delete_path(project_folder), &size);
	if (!text)
		return names;

	for (char *line = strtok(text, "\r\n"); line; line = strtok(NULL, "\r\n"))
	{
		if (_uph_is_plain_file_name(line) && !uph_project_names_contain(names, line))
			naui_list_push(names, naui_string_from_cstr(line));
	}

	free(text);
	return names;
}

static void _uph_pending_delete_write(const Naui_Path project_folder, Naui_List(Naui_String) names)
{
	const Naui_Path file = _uph_pending_delete_path(project_folder);
	if (naui_list_len(names) == 0)
	{
		if (naui_path_exists(file))
			naui_file_delete(file);

		return;
	}

	const size_t capacity = naui_list_len(names) * (NAUI_STRING_MAX_SIZE + 1) + 1;
	char *text = malloc(capacity);
	if (!text)
		return;

	size_t length = 0;
	for (size_t i = 0; i < naui_list_len(names); i++)
		length += (size_t)snprintf(text + length, capacity - length, "%s\n", names[i].data);

	naui_file_write_all(file, text, length);
	free(text);
}

bool uph_project_add_sample_file(Uph_Project *project, const Naui_Path file_path)
{
	naui_log(NAUI_LOG_INFO, "(%s) Adding sample link: %s", project->title.data, file_path.data);
	if (!uph_state.settings.general.copy_resources)
		return uph_resources_add_sample_from_file(file_path);

	const int32_t duplicate = _uph_project_find_duplicate_sample(project, file_path);
	if (duplicate >= 0)
	{
		naui_log(NAUI_LOG_INFO, "Skipping copy, identical file already in project: %s", project->sample_data[duplicate].file_path.data);
		_uph_project_select_sample_by_data(project, (uint32_t)duplicate);
		return true;
	}

	naui_log(NAUI_LOG_INFO, "Copying to temp: %s", file_path.data);
	Naui_Path copy_path = naui_path_join(uph_project_get_path(project), NAUI_PATH(UPH_PATH_TEMP, UPH_IO_FOLDER_SAMPLES));
	naui_directories_create(copy_path);

	Naui_Path file_dest = _uph_project_unique_sample_destination(project, file_path);
	naui_file_copy(file_path, file_dest, NAUI_FILE_COPY_OVERRIDE);
	if (!uph_resources_add_sample_from_file(file_dest))
	{
		naui_file_delete(file_dest);
		return false;
	}

	return true;
}

void uph_project_clear_adopted_samples(void)
{
	naui_list_clear(s_adopted_sample_files);
}

void uph_project_rebase_sample_paths(Uph_Project *project, const Naui_Path from_folder, const Naui_Path to_folder)
{
	const Naui_Path norm_from = naui_path_normalize(from_folder);
	const size_t from_len = strlen(norm_from.data);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(project->sample_data); i++)
	{
		Naui_Path *path = &project->sample_data[i].file_path;
		if (!path->data[0] || !_uph_path_is_inside(*path, from_folder))
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

bool uph_project_references_sample_file(const Uph_Project *project, const char *name)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(project->sample_data); i++)
	{
		const Naui_Path path = project->sample_data[i].file_path;
		if (!uph_io_is_in_samples_folder(path))
			continue;

		const Naui_StringView file_name = naui_file_filename(&path);
		if (uph_project_names_equal(file_name.data, name))
			return true;
	}

	return false;
}

Naui_List(Naui_String) uph_project_collect_sample_candidates(const Naui_Path project_folder, const Naui_Path temp_folder)
{
	Naui_List(Naui_String) candidates = uph_io_defined_sample_files(naui_path_join(project_folder, NAUI_PATH(UPH_IO_FOLDER_SAMPLES)));

	Naui_List(Naui_DirEntry) temp_files = naui_directory_filter(naui_path_join(temp_folder, NAUI_PATH(UPH_IO_FOLDER_SAMPLES)), NULL, uph_audio_engine_extensions);
	for (size_t i = 0; i < naui_list_len(temp_files); i++)
	{
		const Naui_StringView name = naui_file_filename(&temp_files[i].path);
		if (!temp_files[i].is_directory && !uph_project_names_contain(candidates, name.data))
			naui_list_push(candidates, naui_view_to_string(name));
	}
	naui_directory_filter_free(temp_files);

	for (size_t i = 0; i < naui_list_len(s_adopted_sample_files); i++)
	{
		if (!uph_project_names_contain(candidates, s_adopted_sample_files[i].data))
			naui_list_push(candidates, s_adopted_sample_files[i]);
	}

	return candidates;
}

void uph_project_queue_dropped_samples(const Uph_Project *project, const Naui_Path project_folder, Naui_List(Naui_String) candidates)
{
	Naui_List(Naui_String) queue = _uph_pending_delete_read(project_folder);
	const Naui_Path samples_dir = naui_path_join(project_folder, NAUI_PATH(UPH_IO_FOLDER_SAMPLES));

	for (size_t i = naui_list_len(queue); i-- > 0; )
	{
		if (uph_project_references_sample_file(project, queue[i].data))
			naui_list_remove(queue, i);
	}

	for (size_t i = 0; i < naui_list_len(candidates); i++)
	{
		const char *name = candidates[i].data;
		if (!_uph_is_plain_file_name(name) || uph_project_references_sample_file(project, name) || uph_project_names_contain(queue, name))
			continue;

		if (naui_path_exists(naui_path_join(samples_dir, naui_path_from_cstr(name))))
			naui_list_push(queue, candidates[i]);
	}

	_uph_pending_delete_write(project_folder, queue);
	naui_list_free(queue);
}

void uph_project_sweep_deleted_samples(const Uph_Project *project)
{
	const Naui_Path folder = uph_project_get_path(project);
	if (!folder.data[0])
		return;

	Naui_List(Naui_String) queue = _uph_pending_delete_read(folder);
	if (naui_list_len(queue) == 0)
	{
		naui_list_free(queue);
		return;
	}

	const Naui_Path samples_dir = naui_path_join(folder, NAUI_PATH(UPH_IO_FOLDER_SAMPLES));
	Naui_List(Naui_String) saved = uph_io_defined_sample_files(samples_dir);
	Naui_List(Naui_String) kept = NULL;

	for (size_t i = 0; i < naui_list_len(queue); i++)
	{
		const char *name = queue[i].data;
		if (uph_project_references_sample_file(project, name))
			continue;

		if (uph_project_names_contain(saved, name))
		{
			naui_list_push(kept, queue[i]);
			continue;
		}

		naui_log(NAUI_LOG_INFO, "Deleting sample file that is no longer used: %s", name);
		naui_file_delete(naui_path_join(samples_dir, naui_path_from_cstr(name)));
	}

	_uph_pending_delete_write(folder, kept);
	naui_list_free(queue);
	naui_list_free(saved);
	naui_list_free(kept);
}

// Adds audio files from the samples folder that samples.json doesn't define
void uph_project_adopt_sample_files(Uph_Project *project, const Naui_Path project_folder)
{
	const Naui_Path samples_dir = naui_path_join(project_folder, NAUI_PATH(UPH_IO_FOLDER_SAMPLES));
	Naui_List(Naui_DirEntry) files = naui_directory_filter(samples_dir, NULL, uph_audio_engine_extensions);
	if (!files)
		return;

	qsort(files, naui_list_len(files), sizeof(Naui_DirEntry), uph_project_dir_entry_compare);
	Naui_List(Naui_String) defined = uph_io_defined_sample_files(samples_dir);

	for (size_t i = 0; i < naui_list_len(files); i++)
	{
		const Naui_Path file = files[i].path;
		if (files[i].is_directory || uph_project_file_name_is_listed(defined, file) || _uph_project_has_sample_file(project, file))
			continue;

		if (_uph_project_find_duplicate_sample(project, file) >= 0)
		{
			naui_log(NAUI_LOG_INFO, "Not adding sample, an identical file is already in the project: %s", file.data);
			continue;
		}

		if (!uph_resources_add_sample_from_file(file))
		{
			naui_log(NAUI_LOG_WARNING, "Deleting sample that failed to load: %s", file.data);
			naui_file_delete(file);
			continue;
		}

		naui_log(NAUI_LOG_INFO, "Added sample found in the project folder: %s", file.data);
		naui_list_push(s_adopted_sample_files, naui_view_to_string(naui_file_filename(&file)));
	}

	naui_list_free(defined);
	naui_directory_filter_free(files);
}