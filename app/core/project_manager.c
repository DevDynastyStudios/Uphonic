#define UPHONIC_FOLDER naui_path_join(naui_directory_get(NAUI_DIR_APPDATA), NAUI_PATH("Uphonic"))

Uph_Project uph_create_project(void)
{
	return (Uph_Project){};
}

bool uph_save_project(Uph_Project* project)
{
	Naui_Path project_folder = naui_path_join(UPHONIC_FOLDER, NAUI_PATH("project name"));

	// Save project in the Uphonic folder
	return false;
}

bool uph_save_project_as(Uph_Project* project, Naui_Path save_path)
{
	if(!naui_path_exists(save_path))
		naui_directory_create(save_path);

	// Save as uph file

	return false;
}

bool uph_load_project(Naui_Path project_path)
{
	
	return false;
}

bool uph_add_file(Naui_Path file_path, bool make_copy)
{

	return false;
}