bool uph_project_create(Naui_String project_name);
bool uph_project_save(Uph_Project* project, Uph_SaveType save_type);
bool uph_project_export(Uph_Project* project, const Naui_Path output_path, Uph_ExportFormat format);
bool uph_project_load(Uph_Project* project, const Naui_Path project_path);

bool uph_project_add_file(Uph_Project* project, const Naui_Path file_path);	// Add a file to the project (ie. mp3, midi, ogg, etc.)
Naui_Path uph_project_get_path(const Uph_Project* project);