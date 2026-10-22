#ifdef NAUI_WINDOWS
#	define UPH_FILE_NAMES_CASE_SENSITIVE false
#else
#	define UPH_FILE_NAMES_CASE_SENSITIVE true
#endif

int uph_project_dir_entry_compare(const void *a, const void *b);
bool uph_project_names_equal(const char *a, const char *b);
bool uph_project_names_contain(Naui_List(Naui_String) names, const char *name);
bool uph_project_file_name_is_listed(Naui_List(Naui_String) names, const Naui_Path file_path);