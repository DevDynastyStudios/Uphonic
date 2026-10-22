Naui_Path uph_project_unique_workspace_folder(const char *name);
Naui_String uph_project_untitled_name(void);
bool uph_project_name_is_untitled(const char *name);
bool uph_project_folder_is_pristine(const Naui_Path folder);	// Nothing but the lock file
void uph_project_remove_abandoned_untitled(void);