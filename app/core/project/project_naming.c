Naui_Path uph_project_unique_folder(const Naui_Path parent, const char *name)
{
	Naui_Path folder = naui_path_join(parent, naui_path_from_cstr(name));
	for (int32_t counter = 1; naui_path_exists(folder); counter++)
	{
		const Naui_String numbered = naui_string_format("%s (%d)", name, counter);
		folder = naui_path_join(parent, naui_path_from_cstr(numbered.data));
	}

	return folder;
}

Naui_Path uph_project_unique_workspace_folder(const char *name)
{
	return uph_project_unique_folder(UPHONIC_WORKSPACE_FOLDER, name);
}

Naui_String uph_project_untitled_name(void)
{
	const char *translated = NAUI_TR("project.untitled");
	Naui_String name = naui_string_from_cstr(strcmp(translated, "project.untitled") == 0 ? "Untitled" : translated);	// key missing from the language file
	for (char *c = name.data; *c; c++)
	{
		if (strchr("<>:\"/\\|?*", *c))
			*c = '_';
	}

	return name;
}

bool uph_project_name_is_untitled(const char *name)
{
	const Naui_String base = uph_project_untitled_name();
	const size_t length = strlen(base.data);
	if (strncmp(name, base.data, length) != 0)
		return false;

	const char *rest = name + length;
	if (!*rest)
		return true;

	int number = 0, consumed = 0;
	return sscanf(rest, " (%d)%n", &number, &consumed) == 1 && rest[consumed] == '\0';
}

bool uph_project_folder_is_pristine(const Naui_Path folder)
{
	if (!naui_path_is_directory(folder))
		return false;

	Naui_List(Naui_DirEntry) entries = naui_directory_filter(folder, NULL, NULL);
	bool pristine = true;
	for (size_t i = 0; i < naui_list_len(entries) && pristine; i++)
	{
		const Naui_StringView name = naui_file_filename(&entries[i].path);
		pristine = naui_string_view_equals_cstr(name, UPH_RECOVERY_LOCK_FILE, true);
	}

	naui_directory_filter_free(entries);
	return pristine;
}

void uph_project_remove_abandoned_untitled(void)
{
	Naui_List(Naui_DirEntry) entries = naui_directory_filter(UPHONIC_WORKSPACE_FOLDER, NULL, NULL);
	for (size_t i = 0; i < naui_list_len(entries); i++)
	{
		if (!entries[i].is_directory)
			continue;

		const Naui_StringView name = naui_file_filename(&entries[i].path);
		if (uph_project_name_is_untitled(name.data) && !naui_path_is_locked(entries[i].path) && uph_project_folder_is_pristine(entries[i].path))
			naui_directory_remove_all(entries[i].path);
	}

	naui_directory_filter_free(entries);
}