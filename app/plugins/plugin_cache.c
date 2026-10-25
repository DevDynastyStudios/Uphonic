typedef struct
{
	Naui_List(Naui_Path) folders;
	Naui_List(Uph_PluginInfo) known;
	Naui_List(Uph_PluginInfo) found;
	Naui_Mutex lock;
	uint32_t delivered;
	atomic_uint scanned;
	atomic_uint total;
	atomic_bool cancel;
}
Uph_PluginScan;

static Uph_PluginScan *s_scan;
static Naui_JobHandle s_scan_job;

static inline Naui_Path _uph_plugin_get_cache_file(void)
{
	return naui_path_join(naui_directory_get(NAUI_DIR_WORKING), NAUI_PATH("plugin_cache.json"));
}

static void _uph_plugin_cache_collect(Naui_List(Naui_Path) *files, const Naui_Path folder, const atomic_bool *cancel)
{
	static const char* plugin_extensions[] = {".clap", ".vst3"};
	static const size_t plugin_ext_count = sizeof(plugin_extensions) / sizeof(plugin_extensions[0]);

	Naui_DirIterator it = naui_dir_iterator_open(folder, "", NULL, true);
	while (naui_dir_iterator_valid(&it) && !atomic_load(cancel))
	{
		if (it.entry.is_directory)
			_uph_plugin_cache_collect(files, it.entry.path, cancel);

		for (size_t i = 0; i < plugin_ext_count; i++)
		{
			if (naui_string_view_equals_cstr(naui_file_extension(&it.entry.path), plugin_extensions[i], true))
			{
				naui_list_push(*files, it.entry.path);
				break;
			}
		}

		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);
}

static const Uph_PluginInfo *_uph_plugin_cache_find(const Naui_List(Uph_PluginInfo) list, const Naui_Path path)
{
	const Naui_Path wanted = naui_path_normalize(path);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
	{
		if (strcmp(naui_path_normalize(list[i].path).data, wanted.data) == 0)
			return &list[i];
	}

	return NULL;
}

static void _uph_plugin_cache_merge(Naui_List(Uph_PluginInfo) *info_list, const Uph_PluginInfo *info)
{
	const Naui_Path wanted = naui_path_normalize(info->path);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(*info_list); i++)
	{
		if (strcmp(naui_path_normalize((*info_list)[i].path).data, wanted.data) == 0)
		{
			(*info_list)[i] = *info;
			return;
		}
	}

	naui_list_push(*info_list, *info);
}

static void _uph_plugin_scan_publish(Uph_PluginScan *scan, const Uph_PluginInfo *info)
{
	naui_mutex_lock(scan->lock);
	naui_list_push(scan->found, *info);
	naui_mutex_unlock(scan->lock);
}

static void _uph_plugin_scan_deliver(Naui_List(Uph_PluginInfo) *info_list)
{
	Naui_List(Uph_PluginInfo) fresh = NULL;

	naui_mutex_lock(s_scan->lock);
	for (uint32_t i = s_scan->delivered; i < (uint32_t)naui_list_len(s_scan->found); i++)
		naui_list_push(fresh, s_scan->found[i]);

	s_scan->delivered = (uint32_t)naui_list_len(s_scan->found);
	naui_mutex_unlock(s_scan->lock);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(fresh); i++)
		_uph_plugin_cache_merge(info_list, &fresh[i]);

	naui_list_free(fresh);
}

static void _uph_plugin_cache_write(const Naui_List(Uph_PluginInfo) info_list)
{
	Naui_Json json = naui_json_result_create();
	Naui_JsonValue *root = naui_json_object(&json);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(info_list); i++)
	{
		const Uph_PluginInfo *info = &info_list[i];
		Naui_JsonValue *object = naui_json_set_object(&json, root, naui_path_normalize(info->path).data);

		naui_json_set_string(&json, object, "name", info->name.data);
		naui_json_set_string(&json, object, "vendor", info->vendor.data);
		naui_json_set_string(&json, object, "type", info->type == UPH_PLUGIN_EFFECT ? "FX" : "INST");
		naui_json_set_string(&json, object, "format", info->format == UPH_PLUGIN_VST3 ? "VST3" : "CLAP");
	}

	naui_json_write_file(root, _uph_plugin_get_cache_file(), true);
	naui_json_free(&json);
}

static void _uph_plugin_scan_job(void *data, char *err_buf, size_t err_size)
{
	(void)err_buf;
	(void)err_size;
	Uph_PluginScan *scan = data;

	Naui_List(Naui_Path) files = NULL;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(scan->folders); i++)
		_uph_plugin_cache_collect(&files, scan->folders[i], &scan->cancel);

	atomic_store(&scan->total, (uint32_t)naui_list_len(files));

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(files) && !atomic_load(&scan->cancel); i++)
	{
		if (uph_plugin_module_scan_begin(files[i]))
		{
			Uph_PluginInfo info;
			if (uph_get_plugin_info(files[i], &info))
				_uph_plugin_scan_publish(scan, &info);

			uph_plugin_module_scan_end();
		}
		else
		{
			const Uph_PluginInfo *known = _uph_plugin_cache_find(scan->known, files[i]);
			if (known)
				_uph_plugin_scan_publish(scan, known);
		}

		atomic_fetch_add(&scan->scanned, 1);
	}

	naui_list_free(files);

	if (!atomic_load(&scan->cancel))
		_uph_plugin_cache_write(scan->found);
}

static void _uph_plugin_scan_free(void)
{
	naui_list_free(s_scan->folders);
	naui_list_free(s_scan->known);
	naui_list_free(s_scan->found);
	naui_mutex_destroy(s_scan->lock);
	free(s_scan);
	s_scan = NULL;
	s_scan_job = NAUI_JOB_INVALID;
}

bool uph_plugin_cache_read(Naui_List(Uph_PluginInfo) *info_list)
{
	naui_list_clear(*info_list);

	Naui_Json json = naui_json_parse_file(_uph_plugin_get_cache_file());
	if (json.error)
		return false;

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
	return true;
}

void uph_plugin_scan_start(const Naui_List(Uph_PluginInfo) known)
{
	if (s_scan)
		return;

	s_scan = (Uph_PluginScan*)calloc(1, sizeof(*s_scan));
	s_scan->lock = naui_mutex_create();
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.settings.plugin.plugin_paths); i++)
		naui_list_push(s_scan->folders, uph_state.settings.plugin.plugin_paths[i]);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(known); i++)
		naui_list_push(s_scan->known, known[i]);

	if (naui_job_submit(&s_scan_job, _uph_plugin_scan_job, s_scan) != NAUI_JOB_SUBMIT_OK)
	{
		s_scan_job = NAUI_JOB_INVALID;
		_uph_plugin_scan_job(s_scan, NULL, 0);
	}
}

bool uph_plugin_scan_poll(Naui_List(Uph_PluginInfo) *info_list)
{
	if (!s_scan)
		return false;

	_uph_plugin_scan_deliver(info_list);

	const bool ran_inline = !naui_job_is_valid(s_scan_job);
	if (!ran_inline && !naui_job_is_done(s_scan_job))
		return false;

	const bool succeeded = ran_inline || naui_job_status(s_scan_job) == NAUI_JOB_DONE;
	if (!ran_inline)
		naui_job_release(s_scan_job);

	if (succeeded)
	{
		_uph_plugin_scan_deliver(info_list);

		for (uint32_t i = (uint32_t)naui_list_len(*info_list); i > 0; i--)
		{
			if (!_uph_plugin_cache_find(s_scan->found, (*info_list)[i - 1].path))
				naui_list_remove(*info_list, i - 1);
		}
	}

	_uph_plugin_scan_free();
	return true;
}

Uph_PluginScanProgress uph_plugin_scan_progress(void)
{
	Uph_PluginScanProgress progress = { 0 };
	if (!s_scan)
		return progress;

	progress.active = true;
	progress.scanned = atomic_load(&s_scan->scanned);
	progress.total = atomic_load(&s_scan->total);
	return progress;
}

void uph_plugin_scan_shutdown(void)
{
	if (!s_scan)
		return;

	atomic_store(&s_scan->cancel, true);
	if (naui_job_is_valid(s_scan_job))
		naui_job_wait(s_scan_job);

	_uph_plugin_scan_free();
}
