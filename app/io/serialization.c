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

static void uph_io_remove_stale_pattern_files(const Naui_Path patterns_dir, const size_t pattern_count)
{
	Naui_List(Naui_DirEntry) entries = naui_directory_filter(patterns_dir, NULL, NAUI_EXTENSIONS(".mid"));
	for (size_t i = 0; i < (size_t)naui_list_len(entries); i++)
	{
		if (entries[i].is_directory)
			continue;

		const Naui_StringView name = naui_file_filename(&entries[i].path);
		size_t index = 0;
		int consumed = 0;
		if (sscanf(name.data, "pattern_%zu.mid%n", &index, &consumed) == 1 && name.data[consumed] == '\0' && index >= pattern_count)
			naui_file_delete(entries[i].path);
	}

	naui_directory_filter_free(entries);
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

	size_t pattern_count = naui_list_len(project->midi_patterns);
	for (size_t i = 0; i < pattern_count; i++)
	{
		const Uph_MidiPattern* pattern = &project->midi_patterns[i];
		char filename[64];
		snprintf(filename, sizeof(filename), "pattern_%04zu.mid", i);
		Naui_Path midi_path = naui_path_join(save_path, naui_path_from_cstr(filename));
		cmidi_file_t* midi_file = uph_midi_convert_to_midi(pattern);

		if (!midi_file)
		{
			naui_log(NAUI_LOG_ERROR, "Failed to convert pattern %zu to MIDI format", i);
		}
		else
		{
			if (!cmidi_file_save(midi_file, midi_path.data))
				naui_log(NAUI_LOG_ERROR, "Failed to save MIDI file %s", midi_path.data);

			cmidi_file_free(midi_file);
		}

		Naui_JsonValue* pattern_obj = naui_json_push_object(&json, root);
		naui_json_set_string(&json, pattern_obj, "name", pattern->name.data);
		naui_json_set_string(&json, pattern_obj, "midi_file", filename);
	}
		
	bool written = naui_json_write_file(root, naui_path_join(save_path, NAUI_PATH(UPH_IO_FILE_PATTERNS)), true);
	if (!written)
		naui_log(NAUI_LOG_ERROR, "Failed to save patterns JSON metadata: %s", save_path.data);

	uph_io_remove_stale_pattern_files(save_path, pattern_count);

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
static double uph_io_float_json(const float value)
{
	char text[32];
	snprintf(text, sizeof(text), "%.7g", (double)value);
	return strtod(text, NULL);
}

static bool uph_io_save_editor_settings(const Uph_State* state, const Naui_Path save_path)
{
	const Uph_Settings* settings = &state->settings;

	Naui_Json json = naui_json_result_create();
	Naui_JsonValue* root = naui_json_object(&json);
	naui_json_set_int(&json, root, "version", UPH_IO_FORMAT_VERSION);

	uph_io_save_editor_settings_general(&json, naui_json_set_object(&json, root, "general"), &settings->general);
	uph_io_save_editor_settings_audio(&json, naui_json_set_object(&json, root, "audio"), &settings->audio);
	uph_io_save_editor_settings_ui(&json, naui_json_set_object(&json, root, "ui"), &settings->ui);
	uph_io_save_editor_settings_midi(&json, naui_json_set_object(&json, root, "midi"), &settings->midi);
	uph_io_save_editor_settings_plugin(&json, naui_json_set_object(&json, root, "plugin"), &settings->plugin);

	char temp_name[NAUI_PATH_MAX];
	snprintf(temp_name, sizeof(temp_name), "%s.tmp", save_path.data);
	const Naui_Path temp_path = naui_path_from_cstr(temp_name);

	bool written = naui_json_write_file(root, temp_path, true) && naui_file_rename(temp_path, save_path);
	if (!written)
	{
		naui_log(NAUI_LOG_ERROR, "Failed to save settings: %s", save_path.data);
		naui_file_delete(temp_path);
	}

	naui_json_free(&json);
	return written;
}

static void uph_io_save_editor_settings_general(Naui_Json* json, Naui_JsonValue* object, const Uph_GeneralSettings* general)
{
	naui_json_set_string(json, object, "theme", general->theme.data);
	naui_json_set_string(json, object, "language_code", general->language_code.data);
	naui_json_set_string(json, object, "region_code", general->region_code.data);
	naui_json_set_number(json, object, "ui_scale", uph_io_float_json(general->ui_scale));
	naui_json_set_int(json, object, "autosave_timer", general->autosave_timer);
	naui_json_set_int(json, object, "undo_history_limit", (int)general->undo_history_limit);
	naui_json_set_bool(json, object, "confirm_on_exit", general->confirm_on_exit);
	naui_json_set_bool(json, object, "confirm_on_delete", general->confirm_on_delete);
	naui_json_set_bool(json, object, "copy_resources", general->copy_resources);
}

static void uph_io_save_editor_settings_audio(Naui_Json* json, Naui_JsonValue* object, const Uph_AudioSettings* audio)
{
	naui_json_set_string(json, object, "output_device", audio->output_device.data);
	naui_json_set_string(json, object, "input_device", audio->input_device.data);
	naui_json_set_int(json, object, "sample_rate", (int)audio->sample_rate);
	naui_json_set_int(json, object, "buffer_size", (int)audio->buffer_size);
	naui_json_set_int(json, object, "channels", (int)audio->channels);
	naui_json_set_number(json, object, "monitoring_gain", uph_io_float_json(audio->monitoring_gain));
	naui_json_set_number(json, object, "vu_meter_hold_time", uph_io_float_json(audio->vu_meter_hold_time));
	naui_json_set_number(json, object, "vu_meter_decay_time", uph_io_float_json(audio->vu_meter_decay_time));
	naui_json_set_bool(json, object, "exclusive_mode", audio->exclusive_mode);
	naui_json_set_bool(json, object, "software_monitoring", audio->software_monitoring);
	naui_json_set_bool(json, object, "auto_restart_on_device_change", audio->auto_restart_on_device_change);
	naui_json_set_bool(json, object, "underrun_protection", audio->underrun_protection);
}

static void uph_io_save_vec2(Naui_Json* json, Naui_JsonValue* object, const char* key, const Naui_Vec2 value)
{
	Naui_JsonValue* array = naui_json_set_array(json, object, key);
	naui_json_push_number(json, array, uph_io_float_json(value.x));
	naui_json_push_number(json, array, uph_io_float_json(value.y));
}

static void uph_io_save_editor_settings_ui(Naui_Json* json, Naui_JsonValue* object, const Uph_UISettings* ui)
{
	uph_io_save_vec2(json, object, "scroll_sensitivity", ui->scroll_sensitivity);
	uph_io_save_vec2(json, object, "zoom_sensitivity", ui->zoom_sensitivity);
	naui_json_set_int(json, object, "default_snap", (int)ui->default_snap);
	naui_json_set_number(json, object, "playhead_lock_position", uph_io_float_json(ui->playhead_lock_position));
	naui_json_set_bool(json, object, "follow_playhead", ui->follow_playhead);
	naui_json_set_bool(json, object, "show_tooltips", ui->show_tooltips);
}

static void uph_io_save_editor_settings_midi(Naui_Json* json, Naui_JsonValue* object, const Uph_MIDISettings* midi)
{
	naui_json_set_string(json, object, "input_device", midi->input_device.data);
	naui_json_set_string(json, object, "output_device", midi->output_device.data);
	naui_json_set_bool(json, object, "record_quantize", midi->record_quantize);
	naui_json_set_int(json, object, "record_quantize_grid", (int)midi->record_quantize_grid);
	naui_json_set_int(json, object, "count_in_bars", (int)midi->count_in_bars);
	naui_json_set_int(json, object, "grid_division", (int)midi->grid_division);
	naui_json_set_int(json, object, "default_velocity", midi->default_velocity);
	naui_json_set_int(json, object, "default_channel", midi->default_channel);
	naui_json_set_bool(json, object, "filter_by_channel", midi->filter_by_channel);
	naui_json_set_bool(json, object, "midi_thru_enabled", midi->midi_thru_enabled);
	naui_json_set_bool(json, object, "record_over_dub", midi->record_over_dub);
}

static void uph_io_save_editor_settings_plugin(Naui_Json* json, Naui_JsonValue* object, const Uph_PluginSettings* plugin)
{
	Naui_JsonValue* paths = naui_json_set_array(json, object, "plugin_paths");
	for (size_t i = 0; i < (size_t)naui_list_len(plugin->plugin_paths); i++)
		naui_json_push_string(json, paths, plugin->plugin_paths[i].data);

	naui_json_set_bool(json, object, "sandbox_plugins", plugin->sandbox_plugins);
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
	const Naui_Path pattern_file = naui_path_join(load_path, NAUI_PATH(UPH_IO_FILE_PATTERNS));
	Naui_Json json = naui_json_parse_file(pattern_file);
	if (!json.root)
	{
		naui_log(NAUI_LOG_ERROR, "Failed to parse %s", pattern_file.data);
		return false;
	}

	const Naui_JsonValue* root = json.root;
	if (root->type != NAUI_JSON_ARRAY)
	{
		naui_log(NAUI_LOG_ERROR, "%s is in an improper JSON format.", naui_file_filename(&pattern_file).data);
		naui_json_free(&json);
		return false;
	}

	NAUI_JSON_FOREACH(root, key, value)
	{
		Uph_MidiPattern pattern = { 0 };
		const Naui_JsonValue* jname = NULL;

		if (value && value->type == NAUI_JSON_OBJECT)
		{
			jname = naui_json_object_get(value, "name");
			const Naui_JsonValue* jfile = naui_json_object_get(value, "midi_file");
			if (jfile && jfile->type == NAUI_JSON_STRING)
			{
				Naui_Path midi_path = naui_path_join(load_path, naui_path_from_cstr(jfile->string.data));
				pattern = uph_midi_convert_to_pattern(midi_path);
			}
			else
				naui_log(NAUI_LOG_WARNING, "Found malformed file name in patterns, loading it as an empty pattern");
		}
		else
			naui_log(NAUI_LOG_WARNING, "Found malformed pattern entry, loading it as an empty pattern");

		pattern.name = naui_string_from_cstr(naui_json_get_string(jname, NAUI_TR("patterns.default.name")));
		naui_list_push(project->midi_patterns, pattern);
	}

	naui_json_free(&json);
	return true;
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
		naui_log(NAUI_LOG_ERROR, "%s is in an improper JSON format.", naui_file_filename(&sample_file).data);
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

static void uph_io_read_bool(const Naui_JsonValue* object, const char* key, bool* value)
{
	*value = naui_json_get_bool(naui_json_object_get(object, key), *value);
}

static void uph_io_read_float(const Naui_JsonValue* object, const char* key, float* value)
{
	*value = (float)naui_json_get_number(naui_json_object_get(object, key), *value);
}

static void uph_io_read_int32(const Naui_JsonValue* object, const char* key, int32_t* value)
{
	*value = naui_json_get_int(naui_json_object_get(object, key), *value);
}

static void uph_io_read_uint32(const Naui_JsonValue* object, const char* key, uint32_t* value)
{
	const double number = naui_json_get_number(naui_json_object_get(object, key), (double)*value);
	*value = number < 0.0 ? 0 : (number > (double)UINT32_MAX ? UINT32_MAX : (uint32_t)number);
}

static void uph_io_read_uint8(const Naui_JsonValue* object, const char* key, uint8_t* value)
{
	const double number = naui_json_get_number(naui_json_object_get(object, key), (double)*value);
	*value = number < 0.0 ? 0 : (number > (double)UINT8_MAX ? UINT8_MAX : (uint8_t)number);
}

static void uph_io_read_string(const Naui_JsonValue* object, const char* key, Naui_String* value)
{
	const char* text = naui_json_get_string(naui_json_object_get(object, key), NULL);
	if (text)
		*value = naui_string_from_cstr(text);
}

static void uph_io_read_vec2(const Naui_JsonValue* object, const char* key, Naui_Vec2* value)
{
	const Naui_JsonValue* array = naui_json_object_get(object, key);
	if (!array || array->type != NAUI_JSON_ARRAY)
		return;

	value->x = (float)naui_json_get_number(naui_json_array_get(array, 0), value->x);
	value->y = (float)naui_json_get_number(naui_json_array_get(array, 1), value->y);
}

static void uph_io_load_editor_settings_general(Uph_GeneralSettings* general, const Naui_JsonValue* object)
{
	uph_io_read_string(object, "theme", &general->theme);
	uph_io_read_string(object, "language_code", &general->language_code);
	uph_io_read_string(object, "region_code", &general->region_code);
	uph_io_read_float(object, "ui_scale", &general->ui_scale);
	uph_io_read_int32(object, "autosave_timer", &general->autosave_timer);
	uph_io_read_uint32(object, "undo_history_limit", &general->undo_history_limit);
	uph_io_read_bool(object, "confirm_on_exit", &general->confirm_on_exit);
	uph_io_read_bool(object, "confirm_on_delete", &general->confirm_on_delete);
	uph_io_read_bool(object, "copy_resources", &general->copy_resources);

	naui_load_theme(general->theme.data);
	// naui_load_font(0, )
}

static void uph_io_load_editor_settings_audio(Uph_AudioSettings* audio, const Naui_JsonValue* object)
{
	uph_io_read_string(object, "output_device", &audio->output_device);
	uph_io_read_string(object, "input_device", &audio->input_device);
	uph_io_read_uint32(object, "sample_rate", &audio->sample_rate);
	uph_io_read_uint32(object, "buffer_size", &audio->buffer_size);
	uph_io_read_uint32(object, "channels", &audio->channels);
	uph_io_read_float(object, "monitoring_gain", &audio->monitoring_gain);
	uph_io_read_float(object, "vu_meter_hold_time", &audio->vu_meter_hold_time);
	uph_io_read_float(object, "vu_meter_decay_time", &audio->vu_meter_decay_time);
	uph_io_read_bool(object, "exclusive_mode", &audio->exclusive_mode);
	uph_io_read_bool(object, "software_monitoring", &audio->software_monitoring);
	uph_io_read_bool(object, "auto_restart_on_device_change", &audio->auto_restart_on_device_change);
	uph_io_read_bool(object, "underrun_protection", &audio->underrun_protection);
}

static void uph_io_load_editor_settings_ui(Uph_UISettings* ui, const Naui_JsonValue* object)
{
	uph_io_read_vec2(object, "scroll_sensitivity", &ui->scroll_sensitivity);
	uph_io_read_vec2(object, "zoom_sensitivity", &ui->zoom_sensitivity);
	uph_io_read_uint32(object, "default_snap", &ui->default_snap);
	uph_io_read_float(object, "playhead_lock_position", &ui->playhead_lock_position);
	uph_io_read_bool(object, "follow_playhead", &ui->follow_playhead);
	uph_io_read_bool(object, "show_tooltips", &ui->show_tooltips);
}

static void uph_io_load_editor_settings_midi(Uph_MIDISettings* midi, const Naui_JsonValue* object)
{
	uph_io_read_string(object, "input_device", &midi->input_device);
	uph_io_read_string(object, "output_device", &midi->output_device);
	uph_io_read_bool(object, "record_quantize", &midi->record_quantize);
	uph_io_read_uint32(object, "record_quantize_grid", &midi->record_quantize_grid);
	uph_io_read_uint32(object, "count_in_bars", &midi->count_in_bars);
	uph_io_read_uint32(object, "grid_division", &midi->grid_division);
	uph_io_read_uint8(object, "default_velocity", &midi->default_velocity);
	uph_io_read_uint8(object, "default_channel", &midi->default_channel);
	uph_io_read_bool(object, "filter_by_channel", &midi->filter_by_channel);
	uph_io_read_bool(object, "midi_thru_enabled", &midi->midi_thru_enabled);
	uph_io_read_bool(object, "record_over_dub", &midi->record_over_dub);
}

static void uph_io_load_editor_settings_plugin(Uph_PluginSettings* plugin, const Naui_JsonValue* object)
{
	uph_io_read_bool(object, "sandbox_plugins", &plugin->sandbox_plugins);
	const Naui_JsonValue* paths = naui_json_object_get(object, "plugin_paths");
	if (!paths || paths->type != NAUI_JSON_ARRAY)
		return;

	naui_list_clear(plugin->plugin_paths);
	NAUI_JSON_FOREACH(paths, key, value)
	{
		const char* text = naui_json_get_string(value, NULL);
		if (text && text[0])
			naui_list_push(plugin->plugin_paths, naui_path_from_cstr(text));
	}
}

static bool uph_io_load_editor_settings(Uph_State* state, const Naui_Path load_path)
{
	Naui_Json json = naui_json_parse_file(load_path);
	if (!json.root || json.root->type != NAUI_JSON_OBJECT)
	{
		naui_log(NAUI_LOG_WARNING, "Settings file can't be read, keeping it as .bad and using defaults: %s", load_path.data);

		if (json.root)
			naui_json_free(&json);

		char bad_name[NAUI_PATH_MAX];
		snprintf(bad_name, sizeof(bad_name), "%s.bad", load_path.data);
		naui_file_rename(load_path, naui_path_from_cstr(bad_name));
		return false;
	}

	const Naui_JsonValue* root = json.root;
	if (naui_json_get_int(naui_json_object_get(root, "version"), 0) > UPH_IO_FORMAT_VERSION)
		naui_log(NAUI_LOG_WARNING, "Settings were saved by a newer version, unknown values are ignored");

	Uph_Settings* settings = &state->settings;
	const Naui_JsonValue* section = NULL;

	if ((section = naui_json_object_get(root, "general")))
		uph_io_load_editor_settings_general(&settings->general, section);

	if ((section = naui_json_object_get(root, "audio")))
		uph_io_load_editor_settings_audio(&settings->audio, section);

	if ((section = naui_json_object_get(root, "ui")))
		uph_io_load_editor_settings_ui(&settings->ui, section);

	if ((section = naui_json_object_get(root, "midi")))
		uph_io_load_editor_settings_midi(&settings->midi, section);

	if ((section = naui_json_object_get(root, "plugin")))
		uph_io_load_editor_settings_plugin(&settings->plugin, section);

	naui_json_free(&json);
	return true;
}
#pragma endregion

#pragma region Misc Loads
static bool uph_io_load_scales(Uph_State* state, const Naui_Path scales_dir)
{
	// Pass the scales directory, it will contain every json file for all types of scales
	return false;
}

#pragma endregion