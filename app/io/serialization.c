#pragma region Project Saving
static bool uph_io_save_project(const Uph_Project* project, const Naui_Path save_path)
{
	if (!naui_path_exists(save_path))
		naui_directory_create(save_path);

	const Naui_Path patterns_dir = naui_path_join(save_path, NAUI_PATH(UPH_IO_FOLDER_PATTERNS));
	const Naui_Path samples_dir = naui_path_join(save_path, NAUI_PATH(UPH_IO_FOLDER_SAMPLES));
	const Naui_Path tracks_dir = naui_path_join(save_path, NAUI_PATH(UPH_IO_FOLDER_TRACKS));
	naui_directory_create(patterns_dir);
	naui_directory_create(samples_dir);
	naui_directory_create(tracks_dir);

	bool saved = true;
	saved &= uph_io_save_settings(project, save_path);
	saved &= uph_io_save_patterns(project, patterns_dir);
	saved &= uph_io_save_samples(project, samples_dir);
	// Automation Save
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

	Naui_Json json = naui_json_result_create();
	Naui_JsonValue* root = naui_json_object(&json);

	Uph_Version version = project->project_version; 
	Naui_JsonValue* version_obj = naui_json_set_array(&json, root, "version");
	naui_json_push_int(&json, version_obj, version.major);
	naui_json_push_int(&json, version_obj, version.minor);
	naui_json_push_int(&json, version_obj, version.patch);
	
	naui_json_set_int(&json, root, "time_created", project->time_created);
	naui_json_set_int(&json, root, "last_accessed", project->last_accessed);
	naui_json_set_int(&json, root, "last_modified", project->last_modified);

	naui_json_set_string(&json, root, "projectName", project->title.data);
	naui_json_set_number(&json, root, "bpm", project->bpm);

	Naui_JsonValue* time_sig_obj = naui_json_set_array(&json, root, "time_sig");
	naui_json_push_int(&json, time_sig_obj, project->time_signature.numerator);
	naui_json_push_int(&json, time_sig_obj, project->time_signature.denominator);

	bool written = naui_json_write_file(root, naui_path_join(save_path, NAUI_PATH(UPH_IO_FILE_PROJECT)), true);
	naui_json_free(&json);
	if (!written)
		naui_log(NAUI_LOG_ERROR, "Failed to save project: %s", save_path.data);

	return written;
}

static bool uph_io_save_patterns(const Uph_Project* project, const Naui_Path save_path)
{
	if(naui_path_exists(save_path) && !naui_path_is_directory(save_path))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to save. Destination is not a directory (%s)", save_path.data);
		return false;
	}

	Naui_Json json = naui_json_result_create();
	Naui_JsonValue* root = naui_json_array(&json);
	size_t track_count = naui_list_len(project->tracks);;
	for(size_t i = 0; i < track_count; i++)
	{
		Uph_MidiPattern* pattern = &project->midi_patterns[i];
		Naui_JsonValue* pattern_obj = naui_json_object(&json);
		naui_json_set_string(&json, pattern_obj, "name", pattern->name.data);
		
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

	bool written = naui_json_write_file(root, naui_path_join(save_path, NAUI_PATH(UPH_IO_FILE_PATTERNS)), true);
	naui_json_free(&json);
	if (!written)
		naui_log(NAUI_LOG_ERROR, "Failed to save patterns: %s", save_path.data);

	return written;
}

static bool uph_io_save_samples(const Uph_Project* project, const Naui_Path save_path)
{
	if(naui_path_exists(save_path) && !naui_path_is_directory(save_path))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to save. Destination is not a directory (%s)", save_path.data);
		return false;
	}

	Naui_Json json = naui_json_result_create();
	Naui_JsonValue* root = naui_json_array(&json);

	size_t sample_count = naui_list_len(project->sample_data);
	for(size_t i = 0; i < sample_count; i++)
	{
		Uph_Sample* sample = &project->samples[i];
		Uph_SampleData sample_data = project->sample_data[i];
		Naui_JsonValue* sample_obj = naui_json_push_object(&json, root);
		naui_json_set_string(&json, sample_obj, "name", sample->name.data);
		naui_json_set_string(&json, sample_obj, "path", sample_data.file_path.data);
		naui_json_set_number(&json, sample_obj, "time_scale", sample->time_scale);
	}

	bool written = naui_json_write_file(root, naui_path_join(save_path, NAUI_PATH(UPH_IO_FILE_SAMPLES)), true); 
	naui_json_free(&json);
	if (!written)
		naui_log(NAUI_LOG_ERROR, "Failed to save samples: %s", save_path.data);

	return written;
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

static bool uph_io_save_track(const Uph_Track* track, const Naui_Path save_dir)
{
	return false;
}

static bool uph_io_save_track_meta(const Uph_Track* track, const Naui_Path save_dir)
{
	return false;
}

static bool uph_io_save_track_blocks(const Uph_Track* track, const Naui_Path save_dir)
{
	return false;
}

static bool uph_io_save_track_automation(const Uph_Track* track, const Naui_Path save_dir)
{
	return false;
}
#pragma endregion

#pragma region Editor Saving
static bool uph_io_save_editor_settings(const Uph_State* state, const Naui_Path save_path)
{
	return false;
}

static bool uph_io_save_editor_settings_general(const Uph_Settings* settings, const Naui_Path save_dir)
{
	return false;
}

static bool uph_io_save_editor_settings_audio(const Uph_Settings* settings, const Naui_Path save_dir)
{
	return false;
}

static bool uph_io_save_editor_settings_midi(const Uph_Settings* settings, const Naui_Path save_dir)
{
	return false;
}

static bool uph_io_save_editor_settings_timeline(const Uph_Settings* settings, const Naui_Path save_dir)
{
	return false;
}

static bool uph_io_save_editor_settings_plugin(const Uph_Settings* settings, const Naui_Path save_dir)
{
	return false;
}
#pragma endregion

#pragma region Project Loading
static bool uph_io_load_project(Uph_Project* project, const Naui_Path load_path)
{
	if (!naui_path_exists(load_path))
	{
		naui_log(NAUI_LOG_ERROR, "No path exists: %s", load_path.data);
		return false;
	}

	const Naui_Path patterns_dir = naui_path_join(load_path, NAUI_PATH(UPH_IO_FOLDER_PATTERNS));
	const Naui_Path samples_dir = naui_path_join(load_path, NAUI_PATH(UPH_IO_FOLDER_SAMPLES));
	const Naui_Path tracks_dir = naui_path_join(load_path, NAUI_PATH(UPH_IO_FOLDER_TRACKS));
	naui_directory_create(patterns_dir);
	naui_directory_create(samples_dir);
	naui_directory_create(tracks_dir);

	bool loaded = true;
	loaded &= uph_io_load_settings(project, load_path);
	loaded &= uph_io_load_patterns(project, patterns_dir);
	loaded &= uph_io_load_samples(project, samples_dir);
	// Automation Load
	loaded &= uph_io_load_tracks(project, tracks_dir);
	return loaded;
}

static bool uph_io_load_settings(Uph_Project* project, const Naui_Path load_path)
{
	return false;
}

static bool uph_io_load_patterns(Uph_Project* project, const Naui_Path load_path)
{
	return false;
}

static bool uph_io_load_samples(Uph_Project* project, const Naui_Path load_path)
{
	if(naui_path_exists(load_path) && !naui_path_is_directory(load_path))
		return true;

	const Naui_Path sample_file = naui_path_join(load_path, NAUI_PATH(UPH_IO_FILE_SAMPLES));
	Naui_Json json = naui_json_parse_file(sample_file);
	if(!json.root)
	{
		naui_log(NAUI_LOG_ERROR, "Failed to parse %s", sample_file.data);
		return false;
	}

	const Naui_JsonValue* root = json.root;
	if(root->type != NAUI_JSON_ARRAY)
	{
		naui_log(NAUI_LOG_ERROR, "%s.json is in an improper format.");
		naui_json_free(&json);
		return false;
	}

	size_t sample_counter = 0;
	naui_list_clear(project->samples);
	naui_list_clear(project->sample_data);
	NAUI_JSON_FOREACH(root, key, value)
	{
		if(!value || value->type != NAUI_JSON_OBJECT)
			continue;

		const Naui_JsonValue* jvalue_path = naui_json_object_get(value, "path");
		if(!jvalue_path || jvalue_path->type != NAUI_JSON_STRING)
		{
			naui_log(NAUI_LOG_WARNING, "Found malformed path in sample data, skipping...");
			continue;
		}

		Naui_Path sample_path = naui_path_from_cstr(jvalue_path->string.data);
		if(!uph_resources_add_sample_from_file(sample_path))
		{
			naui_log(NAUI_LOG_ERROR, "Resource Engine failed to load sample: %s", sample_path.data);
			continue;
		}

		Uph_Sample* sample = &project->samples[sample_counter++];
		Naui_JsonValue* name = naui_json_object_get(value, "name");
		Naui_JsonValue* time_scale = naui_json_object_get(value, "time_scale");
		Naui_StringView file_name = naui_file_filename(&sample_path);

		const char* file_name_cstr = naui_json_get_string(jvalue_path, NAUI_TR("samples.default.name"));
		sample->name = naui_string_from_cstr(naui_json_get_string(name, file_name_cstr));
		sample->time_scale = naui_json_get_number(time_scale, 0);
	}

	// Check file directory to make sure there are no unseen samples in folder.

	naui_json_free(&json);
	return true;
}
#pragma endregion

#pragma region Track Loading
static bool uph_io_load_tracks(Uph_Project* project, const Naui_Path load_path)
{
	return false;
}

static bool uph_io_load_track(Uph_Track* track, const Naui_Path load_dir)
{
	return false;
}

static bool uph_io_load_track_meta(Uph_Track* track, const Naui_Path load_dir)
{
	return false;
}

static bool uph_io_load_track_blocks(Uph_Track* track, const Naui_Path load_dir)
{
	return false;
}

static bool uph_io_load_track_automation(Uph_Track* track, const Naui_Path load_dir)
{
	return false;
}
#pragma endregion

#pragma region Editor Loading
static bool uph_io_load_editor_settings(Uph_State* state, const Naui_Path load_path)
{
	return false;
}

static bool uph_io_load_editor_settings_general(Uph_Settings* settings, const Naui_Path load_dir)
{
	return false;
}

static bool uph_io_load_editor_settings_audio(Uph_Settings* settings, const Naui_Path load_dir)
{
	return false;
}

static bool uph_io_load_editor_settings_midi(Uph_Settings* settings, const Naui_Path load_dir)
{
	return false;
}

static bool uph_io_load_editor_settings_timeline(Uph_Settings* settings, const Naui_Path load_dir)
{
	return false;
}

static bool uph_io_load_editor_settings_plugin(Uph_Settings* settings, const Naui_Path load_dir)
{
	return false;
}
#pragma endregion