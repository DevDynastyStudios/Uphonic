Uph_Project uph_create_project(void);
bool uph_save_project(Uph_Project* project);
bool uph_save_project_as(Uph_Project* project, Naui_Path save_path);
bool uph_load_project(Naui_Path project_path);

bool uph_add_file(Naui_Path file_path);	// Add a file to the project (ie. mp3, midi, ogg, etc.)