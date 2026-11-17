typedef struct
{
	uint32_t scanned;
	uint32_t total;
	bool active;
}
Uph_PluginScanProgress;

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

typedef struct
{
	Uph_PluginScan *scan;
	Naui_JobHandle job;
}
Uph_PluginScanState;

static Uph_PluginScanState uph__plugin_scan_state;

static inline Naui_Path uph__plugin_get_cache_file(void)
{
	return naui_path_join(UPHONIC_FOLDER, NAUI_PATH("plugin_cache.json"));
}

#define UPH_PLUGIN_SCAN_MAX_DEPTH 16

static const char *const uph__plugin_cache_format_names[UPH_PLUGIN_FORMAT_COUNT] = {
	[UPH_PLUGIN_CLAP] = "CLAP",
	[UPH_PLUGIN_VST3] = "VST3",
	[UPH_PLUGIN_VST2] = "VST2"
};

static bool uph__plugin_cache_files_contain(const Naui_List(Naui_Path) files, const Naui_Path path)
{
	for (size_t i = 0; i < naui_list_len(files); i++)
	{
		if (uph__module_path_equal(files[i], path))
			return true;
	}

	return false;
}

static void uph__plugin_cache_collect(Naui_List(Naui_Path) *files, const Naui_Path folder, const atomic_bool *cancel, uint32_t depth)
{
	Naui_DirIterator it = naui_dir_iterator_open(folder, "", NULL, false);
	while (naui_dir_iterator_valid(&it) && !atomic_load(cancel))
	{
		if (uph_plugin_format_from_path(it.entry.path, NULL))
		{
			if (!uph__plugin_cache_files_contain(*files, it.entry.path))
				naui_list_push(*files, it.entry.path);
		}
		else if (it.entry.is_directory && depth < UPH_PLUGIN_SCAN_MAX_DEPTH)
			uph__plugin_cache_collect(files, it.entry.path, cancel, depth + 1);

		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);
}

static const Uph_PluginInfo *uph__plugin_cache_find(const Naui_List(Uph_PluginInfo) list, const Naui_Path path)
{
	const Naui_Path wanted = naui_path_normalize(path);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
	{
		if (strcmp(naui_path_normalize(list[i].path).data, wanted.data) == 0)
			return &list[i];
	}

	return NULL;
}

static void uph__plugin_cache_merge(Naui_List(Uph_PluginInfo) *info_list, const Uph_PluginInfo *info)
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

static void uph__plugin_scan_publish(Uph_PluginScan *scan, const Uph_PluginInfo *info)
{
	naui_mutex_lock(scan->lock);
	naui_list_push(scan->found, *info);
	naui_mutex_unlock(scan->lock);
}

static void uph__plugin_scan_deliver(Naui_List(Uph_PluginInfo) *info_list)
{
	Uph_PluginScanState *state = &uph__plugin_scan_state;
	Naui_List(Uph_PluginInfo) fresh = NULL;

	naui_mutex_lock(state->scan->lock);
	for (uint32_t i = state->scan->delivered; i < (uint32_t)naui_list_len(state->scan->found); i++)
		naui_list_push(fresh, state->scan->found[i]);

	state->scan->delivered = (uint32_t)naui_list_len(state->scan->found);
	naui_mutex_unlock(state->scan->lock);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(fresh); i++)
		uph__plugin_cache_merge(info_list, &fresh[i]);

	naui_list_free(fresh);
}

static void uph__plugin_cache_write(const Naui_List(Uph_PluginInfo) info_list)
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
		naui_json_set_string(&json, object, "format", uph__plugin_cache_format_names[info->format]);
	}

	naui_directories_create(UPHONIC_FOLDER);
	naui_json_write_file(root, uph__plugin_get_cache_file(), true);
	naui_json_free(&json);
}

static void uph__plugin_scan_job(void *data, char *err_buf, size_t err_size)
{
	(void)err_buf;
	(void)err_size;
	Uph_PluginScan *scan = data;

	Naui_List(Naui_Path) files = NULL;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(scan->folders); i++)
		uph__plugin_cache_collect(&files, scan->folders[i], &scan->cancel, 0);

	atomic_store(&scan->total, (uint32_t)naui_list_len(files));

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(files) && !atomic_load(&scan->cancel); i++)
	{
		if (uph_plugin_module_scan_begin(files[i]))
		{
			Uph_PluginInfo info;
			if (uph_get_plugin_info(files[i], &info))
				uph__plugin_scan_publish(scan, &info);

			uph_plugin_module_scan_end();
		}
		else
		{
			const Uph_PluginInfo *known = uph__plugin_cache_find(scan->known, files[i]);
			if (known)
				uph__plugin_scan_publish(scan, known);
		}

		atomic_fetch_add(&scan->scanned, 1);
	}

	naui_list_free(files);

	if (!atomic_load(&scan->cancel))
		uph__plugin_cache_write(scan->found);
}

static void uph__plugin_scan_free(void)
{
	Uph_PluginScanState *state = &uph__plugin_scan_state;
	naui_list_free(state->scan->folders);
	naui_list_free(state->scan->known);
	naui_list_free(state->scan->found);
	naui_mutex_destroy(state->scan->lock);
	free(state->scan);
	state->scan = NULL;
	state->job = NAUI_JOB_INVALID;
}

bool uph_plugin_cache_read(Naui_List(Uph_PluginInfo) *info_list)
{
	naui_list_clear(*info_list);

	Naui_Json json = naui_json_parse_file(uph__plugin_get_cache_file());
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
		info.format = UPH_PLUGIN_CLAP;
		for (Uph_PluginFormat f = 0; f < UPH_PLUGIN_FORMAT_COUNT; f++)
		{
			if (strcmp(format, uph__plugin_cache_format_names[f]) == 0)
				info.format = f;
		}

		naui_list_push(*info_list, info);
	}

	naui_json_free(&json);
	return true;
}

void uph_plugin_scan_start(const Naui_List(Uph_PluginInfo) known)
{
	Uph_PluginScanState *state = &uph__plugin_scan_state;
	if (state->scan)
		return;

	state->scan = (Uph_PluginScan*)calloc(1, sizeof(*state->scan));
	state->scan->lock = naui_mutex_create();
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.settings.plugin.plugin_paths); i++)
		naui_list_push(state->scan->folders, uph_state.settings.plugin.plugin_paths[i]);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(known); i++)
		naui_list_push(state->scan->known, known[i]);

	if (naui_job_submit(&state->job, uph__plugin_scan_job, state->scan) != NAUI_JOB_SUBMIT_OK)
	{
		state->job = NAUI_JOB_INVALID;
		uph__plugin_scan_job(state->scan, NULL, 0);
	}
}

bool uph_plugin_scan_poll(Naui_List(Uph_PluginInfo) *info_list)
{
	Uph_PluginScanState *state = &uph__plugin_scan_state;
	if (!state->scan)
		return false;

	uph__plugin_scan_deliver(info_list);

	const bool ran_inline = !naui_job_is_valid(state->job);
	if (!ran_inline && !naui_job_is_done(state->job))
		return false;

	const bool succeeded = ran_inline || naui_job_status(state->job) == NAUI_JOB_DONE;
	if (!ran_inline)
		naui_job_release(state->job);

	if (succeeded)
	{
		uph__plugin_scan_deliver(info_list);

		for (uint32_t i = (uint32_t)naui_list_len(*info_list); i > 0; i--)
		{
			if (!uph__plugin_cache_find(state->scan->found, (*info_list)[i - 1].path))
				naui_list_remove(*info_list, i - 1);
		}
	}

	uph__plugin_scan_free();
	return true;
}

Uph_PluginScanProgress uph_plugin_scan_progress(void)
{
	Uph_PluginScanState *state = &uph__plugin_scan_state;
	Uph_PluginScanProgress progress = { 0 };
	if (!state->scan)
		return progress;

	progress.active = true;
	progress.scanned = atomic_load(&state->scan->scanned);
	progress.total = atomic_load(&state->scan->total);
	return progress;
}

void uph_plugin_scan_shutdown(void)
{
	Uph_PluginScanState *state = &uph__plugin_scan_state;
	if (!state->scan)
		return;

	atomic_store(&state->scan->cancel, true);
	if (naui_job_is_valid(state->job))
		naui_job_wait(state->job);

	uph__plugin_scan_free();
}
