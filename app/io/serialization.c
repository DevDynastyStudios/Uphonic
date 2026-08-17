#define PROJECT_FILENAME "project.json"
#define PATTERN_FILENAME "patterns.json"
#define SAMPLE_FILENAME "samples.json"
#define TRACK_META_FILENAME "metadata.json"
#define TRACK_BLOCK_FILENAME "blocks.json"
#define TRACK_AUTOMATION_FILENAME "automation.json"
#define INSTRUMENT_FILENAME "instrument.json"

#pragma region Project Saving
static bool uph_io_save_project(const Uph_Project* project, const Naui_Path save_path)
{
	if (!naui_path_exists(save_path))
		naui_directory_create(save_path);

	Naui_Path patterns_dir = naui_path_join(save_path, NAUI_PATH("Patterns"));
	Naui_Path samples_dir = naui_path_join(save_path, NAUI_PATH("Samples"));
	Naui_Path tracks_dir = naui_path_join(save_path, NAUI_PATH("Tracks"));
	naui_directory_create(patterns_dir);
	naui_directory_create(samples_dir);
	naui_directory_create(tracks_dir);

	bool saved = true;
	saved &= uph_io_save_settings(project, patterns_dir);
	saved &= uph_io_save_settings(project, samples_dir);
	saved &= uph_io_save_settings(project, tracks_dir);
	return saved;
}

static bool uph_io_save_settings(const Uph_Project* project, const Naui_Path save_path)
{
	//if(naui_path_exists(save_path) && naui_direc)
	return false;
}

static bool uph_io_save_patterns(const Uph_Project* project, const Naui_Path save_path)
{
	return false;
}

static bool uph_io_save_automation(const Uph_Project* project, const Naui_Path save_path)
{
	return false;
}
#pragma endregion

#pragma region Track Saving
static bool uph_io_save_tracks(const Uph_Project* project, const Naui_Path save_path)
{
	return false;
}

static bool uph_io_save_track(const Uph_Track* track, const Naui_Path track_dir)
{
	return false;
}

static bool uph_io_save_track_meta(const Uph_Track* track, const Naui_Path track_dir)
{
	return false;
}

static bool uph_io_save_track_blocks(const Uph_Track* track, const Naui_Path track_dir)
{
	return false;
}

static bool uph_io_save_track_automation(const Uph_Track* track, const Naui_Path track_dir)
{
	return false;
}
#pragma endregion

#pragma region Editor Saving
static bool uph_io_save_editor_settings(const Uph_State* state, const Naui_Path save_path)
{
	return false;
}

static bool uph_io_save_editor_settings_general(const Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}

static bool uph_io_save_editor_settings_audio(const Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}

static bool uph_io_save_editor_settings_midi(const Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}

static bool uph_io_save_editor_settings_timeline(const Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}

static bool uph_io_save_editor_settings_plugin(const Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}
#pragma endregion

#pragma region Project Loading
static bool uph_io_load_project(Uph_Project* project, const Naui_Path project_path)
{
	return false;
}

static bool uph_io_load_settings(Uph_Project* project, const Naui_Path settings_path)
{
	return false;
}

static bool uph_io_load_patterns(Uph_Project* project, const Naui_Path project_path)
{
	return false;
}

static bool uph_io_load_samples(Uph_Project* project, const Naui_Path project_path)
{
	return false;
}
#pragma endregion

#pragma region Track Loading
static bool uph_io_load_tracks(Uph_Project* project, const Naui_Path project_path)
{
	return false;
}

static bool uph_io_load_track(Uph_Track* track, const Naui_Path track_dir)
{
	return false;
}

static bool uph_io_load_track_meta(Uph_Track* track, const Naui_Path track_dir)
{
	return false;
}

static bool uph_io_load_track_blocks(Uph_Track* track, const Naui_Path track_dir)
{
	return false;
}

static bool uph_io_load_track_automation(Uph_Track* track, const Naui_Path track_dir)
{
	return false;
}
#pragma endregion

#pragma region Editor Loading
static bool uph_io_load_editor_settings(Uph_State* state, const Naui_Path save_path)
{
	return false;
}

static bool uph_io_load_editor_settings_general(Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}

static bool uph_io_load_editor_settings_audio(Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}

static bool uph_io_load_editor_settings_midi(Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}

static bool uph_io_load_editor_settings_timeline(Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}

static bool uph_io_load_editor_settings_plugin(Uph_Settings* settings, const Naui_Path editor_dir)
{
	return false;
}
#pragma endregion