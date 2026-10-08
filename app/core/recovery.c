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

bool uph_recovery_acquire_lock(const Naui_Path project_folder)
{
	const Naui_Path folder = naui_path_normalize(project_folder);
	if (s_recovery_locked_folder.data[0] && strcmp(s_recovery_locked_folder.data, folder.data) == 0)
		return true;

	uph_recovery_release_lock();
	if (!naui_directories_create(folder) || !naui_path_lock(folder))
		return false;

	s_recovery_locked_folder = folder;
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

bool uph_recovery_find_crashed_project(const Naui_Path workspace_folder, Naui_Path *out_project_folder)
{
	Naui_List(Naui_DirEntry) entries = naui_directory_filter(workspace_folder, NULL, NULL);
	bool found = false;
	uint64_t best_time = 0;
	for (size_t i = 0; i < naui_list_len(entries); i++)
	{
		if (!entries[i].is_directory)
			continue;

		const Naui_Path folder = entries[i].path;
		if (!uph_recovery_project_crashed(folder))
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
	return found;
}