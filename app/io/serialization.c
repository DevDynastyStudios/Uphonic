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

	const Naui_Path patterns_dir = naui_path_join(save_path, NAUI_PATH("patterns"));
	const Naui_Path samples_dir = naui_path_join(save_path, NAUI_PATH("samples"));
	const Naui_Path tracks_dir = naui_path_join(save_path, NAUI_PATH("tracks"));
	naui_directory_create(patterns_dir);
	naui_directory_create(samples_dir);
	naui_directory_create(tracks_dir);

	bool saved = true;
	saved &= uph_io_save_settings(project, patterns_dir);
	saved &= uph_io_save_patterns(project, samples_dir);
	saved &= uph_io_save_tracks(project, tracks_dir);
	return saved;
}

static bool uph_io_save_settings(const Uph_Project* project, const Naui_Path save_path)
{
	if(naui_path_exists(save_path) && !naui_path_is_directory(save_path))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to save. Destination is not a directory (%s)", save_path.data);
		return false;
	}

	Naui_Json settings = naui_json_result_create();
	Naui_JsonValue* root = naui_json_object(&settings);

	Uph_Version version = project->project_version; 
	Naui_JsonValue* version_obj = naui_json_set_array(&settings, root, "version");
	naui_json_set_int(&settings, version_obj, "major", version.major);
	naui_json_set_int(&settings, version_obj, "minor", version.minor);
	naui_json_set_int(&settings, version_obj, "patch", version.patch);
	
	naui_json_set_string(&settings, root, "projectName", project->title.data);
	naui_json_set_int(&settings, root, "bpm", project->bpm);
	return naui_json_write_file(root, naui_path_join(save_path, NAUI_PATH("project.json")), true);
}

static bool uph_io_save_patterns(const Uph_Project* project, const Naui_Path save_path)
{
	if(naui_path_exists(save_path) && !naui_path_is_directory(save_path))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to save. Destination is not a directory (%s)", save_path.data);
		return false;
	}

	Naui_Json pattern_json = naui_json_result_create();
	Naui_JsonValue* root_arr = naui_json_array(&pattern_json);
	size_t track_count = naui_list_len(project->tracks);;
	for(size_t i = 0; i < track_count; i++)
	{
		Uph_MidiPattern* pattern = &project->midi_patterns[i];
		Naui_JsonValue* pattern_obj = naui_json_object(&pattern_json);
		naui_json_set_string(&pattern_json, pattern_obj, "name", pattern->name.data);
		
		// Naui_JsonValue* color_arr = naui_json_set_array(&pattern_json, pattern_obj, "color");
		// naui_json_push_int(&pattern_json, color_arr, pattern->color.a);
		// naui_json_push_int(&pattern_json, color_arr, pattern->color.r);
		// naui_json_push_int(&pattern_json, color_arr, pattern->color.g);
		// naui_json_push_int(&pattern_json, color_arr, pattern->color.b);
		// naui_json_push_array(&pattern_json, color_arr);

		// Implement this once paths are optimized
		// naui_json_set_string(&pattern_json, pattern_obj, "midi path", pattern->midi_path.data);
		// Call cmidi to convert and save midi data
	}

	return naui_json_write_file(root_arr, naui_path_join(save_path, NAUI_PATH("patterns.json")), true);
}

static bool uph_io_save_automation(const Uph_Project* project, const Naui_Path save_path)
{
	return false;
}
#pragma endregion

#pragma region Track Saving
static bool uph_io_save_tracks(const Uph_Project* project, const Naui_Path save_path)
{
	if(naui_path_exists(save_path) && !naui_path_is_directory(save_path))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to save. Destination is not a directory (%s)", save_path.data);
		return false;
	}

	

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