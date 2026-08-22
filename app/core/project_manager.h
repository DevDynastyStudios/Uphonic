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

bool uph_project_create(Naui_String project_name);
bool uph_project_save(Uph_Project* project, Uph_SaveType save_type);
bool uph_project_export(Uph_Project* project, const Naui_Path output_path, Uph_ExportFormat format);
bool uph_project_load(Uph_Project* project, const Naui_Path project_path);

bool uph_project_add_file(Uph_Project* project, const Naui_Path file_path);	// Add a file to the project (ie. mp3, midi, ogg, etc.)
Naui_Path uph_project_get_path(Uph_Project* project);