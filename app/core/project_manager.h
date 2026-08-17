typedef enum
{
	UPH_SAVE_TYPE_CANONICAL,
	UPH_SAVE_TYPE_TEMP
} Uph_SaveType;

typedef enum
{
	UPH_EXPORT_UPH,
	UPH_EXPORT_WAV,
	UPH_EXPORT_MP3,
	UPH_EXPORT_OGG,
	UPH_EXPORT_FLAC,
	UPH_EXPORT_MIDI
} Uph_ExportFormat;

bool uph_create_project(Naui_String project_name);
bool uph_save_project(Uph_Project* project, Uph_SaveType save_type);
bool uph_export_project(Uph_Project* project, Naui_Path output_path, Uph_ExportFormat format);
bool uph_load_project(Naui_Path project_path);

bool uph_add_file(Naui_Path file_path, bool make_copy);	// Add a file to the project (ie. mp3, midi, ogg, etc.)
Naui_Path uph_get_project_path();