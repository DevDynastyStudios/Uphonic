bool uph_project_is_midi_extension(const Naui_StringView extension);
bool uph_project_add_pattern_file(Uph_Project *project, const Naui_Path file_path);
void uph_project_adopt_pattern_files(Uph_Project *project, const Naui_Path project_folder);