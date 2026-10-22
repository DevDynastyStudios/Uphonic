#define UPH_PATH_TEMP ".temp"
#define UPHONIC_FOLDER naui_path_join(naui_directory_get(NAUI_DIR_APPDATA), NAUI_PATH("Uphonic"))
#define UPHONIC_WORKSPACE_FOLDER naui_path_join(UPHONIC_FOLDER, NAUI_PATH("workspace"))
#define UPHONIC_SETTINGS_FILE naui_path_join(UPHONIC_FOLDER, NAUI_PATH(UPH_IO_FILE_SETTINGS))

bool uph_project_create(Naui_String project_name);
void uph_project_close(Uph_Project* project);
bool uph_project_save(Uph_Project* project, Uph_SaveType save_type);
bool uph_project_export(Uph_Project* project, const Naui_Path output_path, Uph_ExportFormat format);
bool uph_project_load(Uph_Project* project, const Naui_Path project_path);

bool uph_project_add_file(Uph_Project* project, const Naui_Path file_path);
Naui_Path uph_project_get_path(const Uph_Project* project);