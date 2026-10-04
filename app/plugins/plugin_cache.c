static void _uph_plugin_cache_scan_folder(Naui_List(Uph_PluginInfo) *info_list, const Naui_Path folder)
{
    static const char* plugin_extensions[] = {".clap", ".vst3"}; 
    static size_t plugin_ext_count = sizeof(plugin_extensions) / sizeof(plugin_extensions[0]);

	Naui_DirIterator it = naui_dir_iterator_open(folder, "", NULL, true);
	while (naui_dir_iterator_valid(&it))
	{
		if (it.entry.is_directory)
			_uph_plugin_cache_scan_folder(info_list, it.entry.path);

		for (size_t i = 0; i < plugin_ext_count; i++)
		{
			if (naui_string_view_equals_cstr(naui_file_extension(&it.entry.path), plugin_extensions[i], true))
			{
				Uph_PluginInfo info;
				if (uph_get_plugin_info(it.entry.path, &info))
					naui_list_push(*info_list, info);
				break;
			}
		}
		
		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);
}

static void _uph_plugin_cache_update(Naui_List(Uph_PluginInfo) *info_list, const Naui_Path path)
{
    Naui_Json json = naui_json_result_create();
    Naui_JsonValue *root = naui_json_object(&json);
    
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.settings.plugin.plugin_paths); i++)
		_uph_plugin_cache_scan_folder(info_list, uph_state.settings.plugin.plugin_paths[i]);

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(*info_list); i++)
    {
        const Uph_PluginInfo *info = &(*info_list)[i];
        Naui_JsonValue *object = naui_json_set_object(&json, root, info->path.data);

        naui_json_set_string(&json, object, "name", info->name.data);
        naui_json_set_string(&json, object, "vendor", info->vendor.data);
        naui_json_set_string(&json, object, "type", info->type == UPH_PLUGIN_EFFECT ? "FX" : "INST");
        naui_json_set_string(&json, object, "format", info->format == UPH_PLUGIN_VST3 ? "VST3" : "CLAP");
    }

    naui_json_write_file(
        root,
        naui_path_join(naui_directory_get(NAUI_DIR_WORKING), NAUI_PATH("plugin_cache.json")),
        true
    );

    naui_json_free(&json);
}

void uph_plugin_cache_get(Naui_List(Uph_PluginInfo) *info_list, bool refresh)
{
    const Naui_Path path = naui_path_join(naui_directory_get(NAUI_DIR_WORKING), NAUI_PATH("plugin_cache.json"));
    Naui_Json json = naui_json_parse_file(path);

    naui_list_clear(*info_list);

    if (json.error || refresh)
    {
        _uph_plugin_cache_update(info_list, path);
        return;
    }

    NAUI_JSON_FOREACH(json.root, key, val)
    {
        if (!key || val->type != NAUI_JSON_OBJECT)
            continue;

        Uph_PluginInfo info;
        memset(&info, 0, sizeof(info));

        int n = naui_json_copy_cstr(key, info.path.data, sizeof(info.path.data));
        if (n < 0)
            continue;

        naui_json_copy_string(naui_json_object_get(val, "name"),   &info.name);
        naui_json_copy_string(naui_json_object_get(val, "vendor"), &info.vendor);

        const char* type = naui_json_get_string(naui_json_object_get(val, "type"),   "");
        const char* format = naui_json_get_string(naui_json_object_get(val, "format"), "");

        info.type = strcmp(type, "FX") == 0 ? UPH_PLUGIN_EFFECT : UPH_PLUGIN_INSTRUMENT;
        info.format = strcmp(format, "VST3") == 0 ? UPH_PLUGIN_VST3   : UPH_PLUGIN_CLAP;

        naui_list_push(*info_list, info);
    }

    naui_json_free(&json);
}