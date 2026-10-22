int uph_project_dir_entry_compare(const void *a, const void *b)
{
	return strcmp(((const Naui_DirEntry *)a)->path.data, ((const Naui_DirEntry *)b)->path.data);
}

bool uph_project_names_equal(const char *a, const char *b)
{
	return naui_string_view_equals_cstr((Naui_StringView){ .data = (char *)a, .length = strlen(a) }, b, UPH_FILE_NAMES_CASE_SENSITIVE);
}

bool uph_project_names_contain(Naui_List(Naui_String) names, const char *name)
{
	for (size_t i = 0; i < naui_list_len(names); i++)
	{
		if (uph_project_names_equal(names[i].data, name))
			return true;
	}

	return false;
}

bool uph_project_file_name_is_listed(Naui_List(Naui_String) names, const Naui_Path file_path)
{
	const Naui_StringView file_name = naui_file_filename(&file_path);
	return uph_project_names_contain(names, file_name.data);
}