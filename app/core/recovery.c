static Naui_Path s_recovery_locked_folder;

static Naui_Path _uph_recovery_lock_file(const Naui_Path project_folder)
{
	return naui_path_join(project_folder, NAUI_PATH(UPH_RECOVERY_LOCK_FILE));
}

static Naui_Path _uph_recovery_temp_folder(const Naui_Path project_folder)
{
	return naui_path_join(project_folder, NAUI_PATH(UPH_PATH_TEMP));
}

static Naui_Path _uph_recovery_marker(const Naui_Path project_folder)
{
	return naui_path_join(_uph_recovery_temp_folder(project_folder), NAUI_PATH(UPH_RECOVERY_AUTOSAVE_MARKER));
}

#define UPH_RECOVERY_EXTERNAL_LIST "external_projects.txt"

static Naui_Path _uph_recovery_external_file(void)
{
	return naui_path_join(UPHONIC_FOLDER, NAUI_PATH(UPH_RECOVERY_EXTERNAL_LIST));
}

static bool _uph_recovery_in_workspace(const Naui_Path folder)
{
	return strcmp(naui_path_normalize(naui_path_parent(folder)).data, naui_path_normalize(UPHONIC_WORKSPACE_FOLDER).data) == 0;
}

static Naui_List(Naui_Path) _uph_recovery_external_read(void)
{
	Naui_List(Naui_Path) list = NULL;

	size_t size = 0;
	char *text = naui_file_read_all(_uph_recovery_external_file(), &size);
	if (!text)
		return list;

	char *copy = (char*)malloc(size + 1);
	if (copy)
	{
		memcpy(copy, text, size);
		copy[size] = '\0';

		for (char *line = strtok(copy, "\r\n"); line; line = strtok(NULL, "\r\n"))
		{
			const Naui_Path folder = naui_path_normalize(naui_path_from_cstr(line));
			naui_list_push(list, folder);
		}

		free(copy);
	}

	free(text);
	return list;
}

static void _uph_recovery_external_write(const Naui_List(Naui_Path) list)
{
	const Naui_Path file = _uph_recovery_external_file();
	if (naui_list_len(list) == 0)
	{
		if (naui_path_exists(file))
			naui_file_delete(file);

		return;
	}

	size_t size = 0;
	for (size_t i = 0; i < naui_list_len(list); i++)
		size += strlen(list[i].data) + 1;

	char *text = (char*)malloc(size + 1);
	if (!text)
		return;

	size_t offset = 0;
	for (size_t i = 0; i < naui_list_len(list); i++)
		offset += (size_t)snprintf(text + offset, size - offset + 1, "%s\n", list[i].data);

	naui_directories_create(UPHONIC_FOLDER);
	naui_file_write_all(file, text, size);
	free(text);
}

static void _uph_recovery_external_add(const Naui_Path folder)
{
	if (_uph_recovery_in_workspace(folder))
		return;

	Naui_List(Naui_Path) list = _uph_recovery_external_read();
	bool listed = false;
	for (size_t i = 0; i < naui_list_len(list) && !listed; i++)
		listed = strcmp(list[i].data, folder.data) == 0;

	if (!listed)
	{
		naui_list_push(list, folder);
		_uph_recovery_external_write(list);
	}

	naui_list_free(list);
}

bool uph_recovery_acquire_lock(const Naui_Path project_folder)
{
	const Naui_Path folder = naui_path_normalize(project_folder);
	if (s_recovery_locked_folder.data[0] && strcmp(s_recovery_locked_folder.data, folder.data) == 0)
		return true;

	uph_recovery_release_lock();
	if (!naui_directories_create(folder) || !naui_path_lock(folder))
		return false;

	s_recovery_locked_folder = folder;
	_uph_recovery_external_add(folder);
	return true;
}

void uph_recovery_release_lock(void)
{
	if (!s_recovery_locked_folder.data[0])
		return;

	naui_path_unlock(s_recovery_locked_folder);
	naui_file_delete(_uph_recovery_lock_file(s_recovery_locked_folder));
	s_recovery_locked_folder = (Naui_Path){0};
}

bool uph_recovery_project_crashed(const Naui_Path project_folder)
{
	return naui_path_exists(_uph_recovery_lock_file(project_folder)) && !naui_path_is_locked(project_folder);
}

void uph_recovery_clear_crash(const Naui_Path project_folder)
{
	if (uph_recovery_project_crashed(project_folder))
		naui_file_delete(_uph_recovery_lock_file(project_folder));
}

bool uph_recovery_has_autosave(const Naui_Path project_folder)
{
	return naui_path_exists(_uph_recovery_marker(project_folder));
}

void uph_recovery_discard_autosave(const Naui_Path project_folder)
{
	const Naui_Path temp_folder = _uph_recovery_temp_folder(project_folder);
	if (naui_path_exists(temp_folder))
		naui_directory_remove_all(temp_folder);
}

void uph_recovery_discard_project(const Naui_Path project_folder)
{
	const Naui_Path folder = naui_path_normalize(project_folder);
	if (naui_path_exists(naui_path_join(folder, NAUI_PATH(UPH_IO_FILE_PROJECT))))
	{
		uph_recovery_discard_autosave(folder);
		uph_recovery_clear_crash(folder);
		return;
	}

	const Naui_Path workspace = naui_path_normalize(UPHONIC_WORKSPACE_FOLDER);
	if (strcmp(naui_path_normalize(naui_path_parent(folder)).data, workspace.data) != 0)
		return;

	if (s_recovery_locked_folder.data[0] && strcmp(s_recovery_locked_folder.data, folder.data) == 0)
		uph_recovery_release_lock();

	naui_directory_remove_all(folder);
}

void uph_recovery_invalidate_autosave(const Naui_Path project_folder)
{
	const Naui_Path marker = _uph_recovery_marker(project_folder);
	if (naui_path_exists(marker))
		naui_file_delete(marker);
}

void uph_recovery_commit_autosave(const Naui_Path project_folder)
{
	char text[32];
	const int length = snprintf(text, sizeof(text), "%llu", (unsigned long long)naui_unix_time());
	naui_file_write_all(_uph_recovery_marker(project_folder), text, (size_t)length);
}

static uint64_t _uph_recovery_autosave_time(const Naui_Path project_folder)
{
	size_t size = 0;
	char *text = naui_file_read_all(_uph_recovery_marker(project_folder), &size);
	if (!text)
		return 0;

	const uint64_t time = (uint64_t)strtoull(text, NULL, 10);
	free(text);
	return time;
}

bool uph_recovery_find_recoverable_project(const Naui_Path workspace_folder, Naui_Path *out_project_folder)
{
	Naui_List(Naui_DirEntry) entries = naui_directory_filter(workspace_folder, NULL, NULL);
	bool found = false;
	uint64_t best_time = 0;
	for (size_t i = 0; i < naui_list_len(entries); i++)
	{
		if (!entries[i].is_directory)
			continue;

		const Naui_Path folder = entries[i].path;
		if (naui_path_is_locked(folder))
			continue;

		if (!uph_recovery_has_autosave(folder))
		{
			uph_recovery_clear_crash(folder);
			continue;
		}

		const uint64_t autosave_time = _uph_recovery_autosave_time(folder);
		if (!found || autosave_time > best_time)
		{
			*out_project_folder = folder;
			best_time = autosave_time;
			found = true;
		}
	}

	naui_directory_filter_free(entries);
	Naui_List(Naui_Path) external = _uph_recovery_external_read();
	bool pruned = false;
	for (size_t i = naui_list_len(external); i > 0; i--)
	{
		const Naui_Path folder = external[i - 1];
		bool keep = naui_path_is_directory(folder);
		if (keep && !naui_path_is_locked(folder))
		{
			if (uph_recovery_has_autosave(folder))
			{
				const uint64_t autosave_time = _uph_recovery_autosave_time(folder);
				if (!found || autosave_time > best_time)
				{
					*out_project_folder = folder;
					best_time = autosave_time;
					found = true;
				}
			}
			else
			{
				uph_recovery_clear_crash(folder);
				keep = false;
			}
		}

		if (!keep)
		{
			naui_list_remove(external, i - 1);
			pruned = true;
		}
	}

	if (pruned)
		_uph_recovery_external_write(external);

	naui_list_free(external);
	return found;
}