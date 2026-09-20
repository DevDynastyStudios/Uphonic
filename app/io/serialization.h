#define UPH_IO_FOLDER_PROJECT "project"
#define UPH_IO_FOLDER_SAMPLES "samples"
#define UPH_IO_FOLDER_PATTERNS "patterns"
#define UPH_IO_FOLDER_AUTOMATION "automation"
#define UPH_IO_FOLDER_TRACKS "tracks"

#define UPH_IO_FILE_PROJECT "project.json"
#define UPH_IO_FILE_SAMPLES "samples.json"
#define UPH_IO_FILE_PATTERNS "patterns.json"
#define UPH_IO_FILE_AUTOMATION "automation.json"
#define UPH_IO_FILE_TRACKS "tracks.json"
#define UPH_IO_FILE_TRACK_BLOCK "blocks.json"
#define UPH_IO_FILE_INSTRUMENT "instrument.json"
#define UPH_IO_FILE_META "metadata.json"
#define UPH_IO_FILE_SETTINGS "settings.json"

#define UPH_IO_FORMAT_VERSION 1

static bool uph_io_save_project(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_settings(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_patterns(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_samples(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_automation(const Uph_Project* project, const Naui_Path save_path);

static bool uph_io_save_tracks(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_track(const Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_save_track_meta(const Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_save_track_blocks(const Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_save_track_automation(const Uph_Track* track, const Naui_Path track_dir);

static bool uph_io_save_editor_settings(const Uph_State* state, const Naui_Path save_path);
static void uph_io_save_editor_settings_general(Naui_Json* json, Naui_JsonValue* object, const Uph_GeneralSettings* general);
static void uph_io_save_editor_settings_audio(Naui_Json* json, Naui_JsonValue* object, const Uph_AudioSettings* audio);
static void uph_io_save_editor_settings_midi(Naui_Json* json, Naui_JsonValue* object, const Uph_MIDISettings* midi);
static void uph_io_save_editor_settings_ui(Naui_Json* json, Naui_JsonValue* object, const Uph_UISettings* ui);
static void uph_io_save_editor_settings_plugin(Naui_Json* json, Naui_JsonValue* object, const Uph_PluginSettings* plugin);

static bool uph_io_load_project(Uph_Project* project, const Naui_Path load_path);
static bool uph_io_load_settings(Uph_Project* project, const Naui_Path load_path);
static bool uph_io_load_patterns(Uph_Project* project, const Naui_Path load_path);
static bool uph_io_load_samples(Uph_Project* project, const Naui_Path load_path);

static bool uph_io_load_tracks(Uph_Project* project, const Naui_Path load_path);
static bool uph_io_load_track(Uph_Track* track, const Naui_Path load_dir);
static bool uph_io_load_track_meta(Uph_Track* track, const Naui_Path load_dir);
static bool uph_io_load_track_blocks(Uph_Track* track, const Naui_Path load_dir);
static bool uph_io_load_track_automation(Uph_Track* track, const Naui_Path load_dir);

static bool uph_io_load_editor_settings(Uph_State* state, const Naui_Path load_path);
static void uph_io_load_editor_settings_general(Uph_GeneralSettings* general, const Naui_JsonValue* object);
static void uph_io_load_editor_settings_audio(Uph_AudioSettings* audio, const Naui_JsonValue* object);
static void uph_io_load_editor_settings_midi(Uph_MIDISettings* midi, const Naui_JsonValue* object);
static void uph_io_load_editor_settings_ui(Uph_UISettings* ui, const Naui_JsonValue* object);
static void uph_io_load_editor_settings_plugin(Uph_PluginSettings* plugin, const Naui_JsonValue* object);

static bool uph_io_load_scales(Uph_State* state, const Naui_Path scales_dir);	// Load all scales in directory