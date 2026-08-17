static bool uph_io_save_project(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_settings(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_patterns(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_automation(const Uph_Project* project, const Naui_Path save_path);

static bool uph_io_save_tracks(const Uph_Project* project, const Naui_Path save_path);
static bool uph_io_save_track(const Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_save_track_meta(const Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_save_track_blocks(const Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_save_track_automation(const Uph_Track* track, const Naui_Path track_dir);

static bool uph_io_save_editor_settings(const Uph_State* state, const Naui_Path save_path);
static bool uph_io_save_editor_settings_general(const Uph_Settings* settings, const Naui_Path editor_dir);
static bool uph_io_save_editor_settings_audio(const Uph_Settings* settings, const Naui_Path editor_dir);
static bool uph_io_save_editor_settings_midi(const Uph_Settings* settings, const Naui_Path editor_dir);
static bool uph_io_save_editor_settings_timeline(const Uph_Settings* settings, const Naui_Path editor_dir);
static bool uph_io_save_editor_settings_plugin(const Uph_Settings* settings, const Naui_Path editor_dir);

static bool uph_io_load_project(Uph_Project* project, const Naui_Path project_path);
static bool uph_io_load_settings(Uph_Project* project, const Naui_Path settings_path);
static bool uph_io_load_patterns(Uph_Project* project, const Naui_Path project_path);
static bool uph_io_load_samples(Uph_Project* project, const Naui_Path project_path);

static bool uph_io_load_tracks(Uph_Project* project, const Naui_Path project_path);
static bool uph_io_load_track(Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_load_track_meta(Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_load_track_blocks(Uph_Track* track, const Naui_Path track_dir);
static bool uph_io_load_track_automation(Uph_Track* track, const Naui_Path track_dir);

static bool uph_io_load_editor_settings(Uph_State* state, const Naui_Path save_path);
static bool uph_io_load_editor_settings_general(Uph_Settings* settings, const Naui_Path editor_dir);
static bool uph_io_load_editor_settings_audio(Uph_Settings* settings, const Naui_Path editor_dir);
static bool uph_io_load_editor_settings_midi(Uph_Settings* settings, const Naui_Path editor_dir);
static bool uph_io_load_editor_settings_timeline(Uph_Settings* settings, const Naui_Path editor_dir);
static bool uph_io_load_editor_settings_plugin(Uph_Settings* settings, const Naui_Path editor_dir);