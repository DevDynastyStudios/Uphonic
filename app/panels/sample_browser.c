NAUI_PANEL(uph_sample_browser)

#define UPH_SAMPLE_BROWSER_ADD_FOLDER_DIALOG_KEY "uph_sample_browser_add_folder"
#define UPH_SAMPLE_BROWSER_MAX_NODES 500000u
#define UPH_SAMPLE_BROWSER_MAX_DEPTH 32
#define UPH_SAMPLE_BROWSER_ROW_HEIGHT 22.0f
#define UPH_SAMPLE_BROWSER_INDENT 14.0f
#define UPH_SAMPLE_BROWSER_WAVEFORM_HEIGHT 56.0f
#define UPH_SAMPLE_BROWSER_NONE UINT32_MAX

typedef uint8_t Uph_SampleBrowserNodeFlags;
enum
{
	UPH_SAMPLE_BROWSER_NODE_DIR = 1 << 0,
	UPH_SAMPLE_BROWSER_NODE_EXPANDED = 1 << 1,
	UPH_SAMPLE_BROWSER_NODE_ROOT = 1 << 2
};

typedef struct
{
	uint32_t path;
	uint32_t name;
	uint32_t parent;
	uint32_t first_child;
	uint32_t child_count;
	uint16_t depth;
	Uph_SampleBrowserNodeFlags flags;
}
Uph_SampleBrowserNode;

typedef struct
{
	Naui_List(Uph_SampleBrowserNode) nodes;
	char *pool;
	uint32_t pool_size;
	uint32_t pool_capacity;
	uint32_t root_count;
	uint32_t file_count;
	bool truncated;
}
Uph_SampleBrowserIndex;

typedef struct
{
	Naui_List(Naui_Path) folders;
	Uph_SampleBrowserIndex result;
	atomic_bool cancel;
}
Uph_SampleBrowserScan;

typedef struct
{
	Naui_Path path;
	Uph_PreviewSource source;
}
Uph_SampleBrowserLoad;

typedef struct
{
	char *name;
	bool is_directory;
}
Uph_SampleBrowserEntry;

static struct
{
	Uph_SampleBrowserIndex index;
	Uph_SampleBrowserScan *scan;
	Naui_JobHandle scan_job;
	bool rescan_requested;

	Naui_List(uint32_t) rows;
	bool rows_dirty;
	Naui_String filter;
	Naui_String applied_filter;
	uint32_t selected;
	uint32_t context_node;
	float scroll;
	bool scroll_to_selected;

	Uph_SampleBrowserLoad *load;
	Naui_JobHandle load_job;
	Naui_Path load_next;
	bool load_next_pending;
	bool load_cancelled;
	Naui_Path previewing;
}
s_sample_browser = {
	.selected = UPH_SAMPLE_BROWSER_NONE,
	.context_node = UPH_SAMPLE_BROWSER_NONE
};

#pragma region Index

static uint32_t _uph_sample_browser_pool_push(Uph_SampleBrowserIndex *index, const char *text, size_t length)
{
	const uint32_t needed = index->pool_size + (uint32_t)length + 1;
	if (needed > index->pool_capacity)
	{
		uint32_t capacity = index->pool_capacity ? index->pool_capacity : 4096;
		while (capacity < needed)
			capacity *= 2;

		char *grown = realloc(index->pool, capacity);
		if (!grown)
			return UPH_SAMPLE_BROWSER_NONE;

		index->pool = grown;
		index->pool_capacity = capacity;
	}

	const uint32_t offset = index->pool_size;
	memcpy(index->pool + offset, text, length);
	index->pool[offset + length] = '\0';
	index->pool_size = needed;
	return offset;
}

static void _uph_sample_browser_index_free(Uph_SampleBrowserIndex *index)
{
	naui_list_free(index->nodes);
	free(index->pool);
	*index = (Uph_SampleBrowserIndex){ 0 };
}

static const char *_uph_sample_browser_node_path(const Uph_SampleBrowserIndex *index, uint32_t node)
{
	return index->pool + index->nodes[node].path;
}

static const char *_uph_sample_browser_node_name(const Uph_SampleBrowserIndex *index, uint32_t node)
{
	return index->pool + index->nodes[node].name;
}

static int _uph_sample_browser_compare_nocase(const char *a, const char *b)
{
	while (*a && tolower((unsigned char)*a) == tolower((unsigned char)*b))
	{
		a++;
		b++;
	}

	return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static bool _uph_sample_browser_is_audio(const char *name)
{
	const char *dot = strrchr(name, '.');
	if (!dot)
		return false;

	for (const char **ext = uph_audio_engine_extensions; *ext; ext++)
	{
		if (_uph_sample_browser_compare_nocase(dot, *ext) == 0)
			return true;
	}

	return false;
}

static int _uph_sample_browser_entry_compare(const void *a, const void *b)
{
	const Uph_SampleBrowserEntry *left = a;
	const Uph_SampleBrowserEntry *right = b;
	if (left->is_directory != right->is_directory)
		return left->is_directory ? -1 : 1;

	return _uph_sample_browser_compare_nocase(left->name, right->name);
}

static bool _uph_sample_browser_push_node(Uph_SampleBrowserIndex *index, const char *path, size_t path_length, size_t name_start, uint32_t parent, uint16_t depth, Uph_SampleBrowserNodeFlags flags)
{
	if ((uint32_t)naui_list_len(index->nodes) >= UPH_SAMPLE_BROWSER_MAX_NODES)
	{
		index->truncated = true;
		return false;
	}

	const uint32_t offset = _uph_sample_browser_pool_push(index, path, path_length);
	if (offset == UPH_SAMPLE_BROWSER_NONE)
		return false;

	Uph_SampleBrowserNode node = {
		.path = offset,
		.name = offset + (uint32_t)name_start,
		.parent = parent,
		.first_child = 0,
		.child_count = 0,
		.depth = depth,
		.flags = flags
	};
	naui_list_push(index->nodes, node);

	if (!(flags & UPH_SAMPLE_BROWSER_NODE_DIR))
		index->file_count++;

	return true;
}

static void _uph_sample_browser_scan_directory(Uph_SampleBrowserIndex *index, uint32_t node_index, Naui_List(uint32_t) *stack)
{
	const Naui_Path path = naui_path_from_cstr(_uph_sample_browser_node_path(index, node_index));
	const uint16_t depth = index->nodes[node_index].depth;
	Naui_List(Uph_SampleBrowserEntry) entries = NULL;

	Naui_DirIterator it = naui_dir_iterator_open(path, NULL, NULL, false);
	while (naui_dir_iterator_valid(&it))
	{
		const Naui_StringView name = naui_file_filename(&it.entry.path);
		const bool usable = name.length > 0 && name.data[0] != '.' &&
			(it.entry.is_directory || _uph_sample_browser_is_audio(name.data));

		if (usable)
		{
			char *copy = malloc(name.length + 1);
			if (copy)
			{
				memcpy(copy, name.data, name.length);
				copy[name.length] = '\0';
				Uph_SampleBrowserEntry entry = { .name = copy, .is_directory = it.entry.is_directory };
				naui_list_push(entries, entry);
			}
		}

		naui_dir_iterator_next(&it);
	}
	naui_dir_iterator_close(&it);

	const uint32_t entry_count = (uint32_t)naui_list_len(entries);
	if (entry_count > 1)
		qsort(entries, entry_count, sizeof(entries[0]), _uph_sample_browser_entry_compare);

	const size_t parent_length = strlen(path.data);
	const bool needs_separator = parent_length > 0 && path.data[parent_length - 1] != '/' && path.data[parent_length - 1] != '\\';
	const uint32_t first_child = (uint32_t)naui_list_len(index->nodes);
	uint32_t child_count = 0;

	for (uint32_t i = 0; i < entry_count; i++)
	{
		char child[NAUI_PATH_MAX];
		const int written = snprintf(child, sizeof(child), "%s%s%s", path.data, needs_separator ? (char[]){ NAUI_PATH_SEP, '\0' } : "", entries[i].name);
		if (written > 0 && (size_t)written < sizeof(child))
		{
			const size_t name_start = parent_length + (needs_separator ? 1 : 0);
			const Uph_SampleBrowserNodeFlags flags = entries[i].is_directory ? UPH_SAMPLE_BROWSER_NODE_DIR : 0;
			if (_uph_sample_browser_push_node(index, child, (size_t)written, name_start, node_index, (uint16_t)(depth + 1), flags))
				child_count++;
		}
		free(entries[i].name);
	}
	naui_list_free(entries);

	index->nodes[node_index].first_child = first_child;
	index->nodes[node_index].child_count = child_count;

	if (depth + 1 >= UPH_SAMPLE_BROWSER_MAX_DEPTH)
		return;

	for (uint32_t i = child_count; i > 0; i--)
	{
		const uint32_t child_index = first_child + i - 1;
		if (index->nodes[child_index].flags & UPH_SAMPLE_BROWSER_NODE_DIR)
			naui_list_push(*stack, child_index);
	}
}

static void _uph_sample_browser_scan_job(void *data, char *err_buf, size_t err_size)
{
	(void)err_buf;
	(void)err_size;
	Uph_SampleBrowserScan *scan = data;
	Uph_SampleBrowserIndex *index = &scan->result;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(scan->folders); i++)
	{
		const Naui_Path folder = scan->folders[i];
		const Naui_StringView name = naui_file_filename(&folder);
		const size_t length = strlen(folder.data);
		const size_t name_start = name.length > 0 ? (size_t)(name.data - folder.data) : 0;
		_uph_sample_browser_push_node(index, folder.data, length, name_start, UPH_SAMPLE_BROWSER_NONE, 0, UPH_SAMPLE_BROWSER_NODE_DIR | UPH_SAMPLE_BROWSER_NODE_ROOT);
	}

	index->root_count = (uint32_t)naui_list_len(index->nodes);

	Naui_List(uint32_t) stack = NULL;
	for (uint32_t i = index->root_count; i > 0; i--)
		naui_list_push(stack, i - 1);

	while (naui_list_len(stack) > 0 && !atomic_load_explicit(&scan->cancel, memory_order_relaxed))
	{
		const uint32_t node = naui_list_pop(stack);
		_uph_sample_browser_scan_directory(index, node, &stack);
	}

	naui_list_free(stack);
}

static void _uph_sample_browser_scan_free(Uph_SampleBrowserScan *scan)
{
	naui_list_free(scan->folders);
	_uph_sample_browser_index_free(&scan->result);
	free(scan);
}

static void _uph_sample_browser_cancel_scan(void)
{
	if (!s_sample_browser.scan)
		return;

	atomic_store(&s_sample_browser.scan->cancel, true);
	if (naui_job_is_valid(s_sample_browser.scan_job))
		naui_job_wait(s_sample_browser.scan_job);

	_uph_sample_browser_scan_free(s_sample_browser.scan);
	s_sample_browser.scan = NULL;
	s_sample_browser.scan_job = NAUI_JOB_INVALID;
}

static void _uph_sample_browser_start_scan(void)
{
	if (s_sample_browser.scan)
	{
		s_sample_browser.rescan_requested = true;
		return;
	}

	s_sample_browser.rescan_requested = false;

	Uph_SampleBrowserScan *scan = calloc(1, sizeof(Uph_SampleBrowserScan));
	if (!scan)
		return;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.settings.browser.folders); i++)
		naui_list_push(scan->folders, naui_path_normalize(uph_state.settings.browser.folders[i]));

	atomic_init(&scan->cancel, false);

	if (naui_job_submit(&s_sample_browser.scan_job, _uph_sample_browser_scan_job, scan) != NAUI_JOB_SUBMIT_OK)
	{
		naui_log(NAUI_LOG_WARNING, "Sample browser: couldn't queue folder scan");
		_uph_sample_browser_scan_free(scan);
		s_sample_browser.scan_job = NAUI_JOB_INVALID;
		return;
	}

	s_sample_browser.scan = scan;
}

static uint32_t _uph_sample_browser_find_path(const Uph_SampleBrowserIndex *index, const char *path)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(index->nodes); i++)
	{
		if (strcmp(_uph_sample_browser_node_path(index, i), path) == 0)
			return i;
	}

	return UPH_SAMPLE_BROWSER_NONE;
}

static void _uph_sample_browser_adopt_index(Uph_SampleBrowserIndex *fresh)
{
	Uph_SampleBrowserIndex *old = &s_sample_browser.index;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(old->nodes); i++)
	{
		if (!(old->nodes[i].flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED))
			continue;

		const uint32_t match = _uph_sample_browser_find_path(fresh, _uph_sample_browser_node_path(old, i));
		if (match != UPH_SAMPLE_BROWSER_NONE)
			fresh->nodes[match].flags |= UPH_SAMPLE_BROWSER_NODE_EXPANDED;
	}

	uint32_t selected = UPH_SAMPLE_BROWSER_NONE;
	if (s_sample_browser.selected < (uint32_t)naui_list_len(old->nodes))
		selected = _uph_sample_browser_find_path(fresh, _uph_sample_browser_node_path(old, s_sample_browser.selected));

	_uph_sample_browser_index_free(old);
	*old = *fresh;
	*fresh = (Uph_SampleBrowserIndex){ 0 };

	s_sample_browser.selected = selected;
	s_sample_browser.context_node = UPH_SAMPLE_BROWSER_NONE;
	s_sample_browser.rows_dirty = true;
}

static void _uph_sample_browser_poll_scan(void)
{
	Uph_SampleBrowserScan *scan = s_sample_browser.scan;
	if (!scan || !naui_job_is_done(s_sample_browser.scan_job))
		return;

	naui_job_release(s_sample_browser.scan_job);
	s_sample_browser.scan_job = NAUI_JOB_INVALID;

	if (scan->result.truncated)
		naui_log(NAUI_LOG_WARNING, "Sample browser: index truncated at %u entries", UPH_SAMPLE_BROWSER_MAX_NODES);

	_uph_sample_browser_adopt_index(&scan->result);
	_uph_sample_browser_scan_free(scan);
	s_sample_browser.scan = NULL;

	if (s_sample_browser.rescan_requested)
		_uph_sample_browser_start_scan();
}

#pragma endregion

#pragma region Folders

static void _uph_sample_browser_add_folder(const Naui_Path folder)
{
	Uph_BrowserSettings *browser = &uph_state.settings.browser;
	const Naui_Path path = naui_path_normalize(folder);
	if (!naui_path_is_directory(path))
		return;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(browser->folders); i++)
	{
		if (strcmp(naui_path_normalize(browser->folders[i]).data, path.data) == 0)
			return;
	}

	naui_list_push(browser->folders, path);
	uph_settings_save();
	_uph_sample_browser_start_scan();
}

static void _uph_sample_browser_remove_folder(const char *path)
{
	Uph_BrowserSettings *browser = &uph_state.settings.browser;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(browser->folders); i++)
	{
		if (strcmp(naui_path_normalize(browser->folders[i]).data, path) != 0)
			continue;

		naui_list_remove(browser->folders, i);
		uph_settings_save();
		_uph_sample_browser_start_scan();
		return;
	}
}

static void _uph_sample_browser_folder_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	(void)user_data;
	if (is_cancelled || result->count == 0)
		return;

	_uph_sample_browser_add_folder(result->paths[0]);
}

static void _uph_sample_browser_open_folder_dialog(void)
{
	if (!naui_dialog_open_folder(UPH_SAMPLE_BROWSER_ADD_FOLDER_DIALOG_KEY, NULL, _uph_sample_browser_folder_picked, NULL))
		return;

	naui_dialog_title_set(UPH_SAMPLE_BROWSER_ADD_FOLDER_DIALOG_KEY, NAUI_TR("sample_browser.add_folder"));
}

#pragma endregion

#pragma region Preview

static void _uph_sample_browser_load_job(void *data, char *err_buf, size_t err_size)
{
	(void)err_buf;
	(void)err_size;
	Uph_SampleBrowserLoad *load = data;
	load->source = uph_audio_engine_preview_open(load->path);
}

static void _uph_sample_browser_start_load(const Naui_Path path)
{
	Uph_SampleBrowserLoad *load = calloc(1, sizeof(Uph_SampleBrowserLoad));
	if (!load)
		return;

	load->path = path;
	if (naui_job_submit(&s_sample_browser.load_job, _uph_sample_browser_load_job, load) != NAUI_JOB_SUBMIT_OK)
	{
		free(load);
		s_sample_browser.load_job = NAUI_JOB_INVALID;
		return;
	}

	s_sample_browser.load = load;
}

static void _uph_sample_browser_preview(const char *path)
{
	const Naui_Path target = naui_path_from_cstr(path);
	uph_audio_engine_preview_stop();

	if (s_sample_browser.load)
	{
		s_sample_browser.load_cancelled = false;
		s_sample_browser.load_next = target;
		s_sample_browser.load_next_pending = true;
		return;
	}

	Uph_PreviewInfo current;
	if (uph_audio_engine_preview_info(&current) && strcmp(current.path->data, target.data) == 0)
	{
		uph_audio_engine_preview_restart();
		return;
	}

	_uph_sample_browser_start_load(target);
}

static void _uph_sample_browser_poll_load(void)
{
	Uph_SampleBrowserLoad *load = s_sample_browser.load;
	if (!load || !naui_job_is_done(s_sample_browser.load_job))
		return;

	naui_job_release(s_sample_browser.load_job);
	s_sample_browser.load_job = NAUI_JOB_INVALID;
	s_sample_browser.load = NULL;

	const bool stale = s_sample_browser.load_next_pending;
	const bool cancelled = s_sample_browser.load_cancelled;
	s_sample_browser.load_cancelled = false;
	if (!stale && !cancelled && uph_audio_engine_preview_source_valid(&load->source))
	{
		uph_audio_engine_preview_play(load->source);
		s_sample_browser.previewing = load->path;
	}
	else
		uph_audio_engine_preview_close(&load->source);

	free(load);

	if (stale)
	{
		s_sample_browser.load_next_pending = false;
		_uph_sample_browser_preview(s_sample_browser.load_next.data);
	}
}

static void _uph_sample_browser_waveform_draw(Leaf_BoundingBox box, const Uph_PreviewInfo *info)
{
	if (!info->peaks || box.width <= 1.0f || box.height <= 1.0f)
		return;

	const uint64_t bin_count = info->peak_total;
	const uint64_t ready_count = info->peak_count < bin_count ? info->peak_count : bin_count;
	if (bin_count == 0 || ready_count == 0)
		return;

	const float inset = NAUI_DPI(3.0f);
	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	uph_ui_waveform_peaks(
		(Naui_Vec2){ box.x, box.y + inset },
		(Naui_Vec2){ box.width, box.height - inset * 2.0f },
		naui_theme_color("uph_resource_list_item_fg_color"),
		info->peaks,
		ready_count,
		bin_count,
		1.0f
	);

	if (uph_audio_engine_preview_playing())
	{
		const float progress = (float)NAUI_CLAMP(uph_audio_engine_preview_progress(), 0.0, 1.0);
		naui_fill_rect((Naui_Vec2){ box.x + progress * box.width, box.y }, (Naui_Vec2){ NAUI_DPI(2.0f), box.height }, naui_theme_color("naui_panel_tab_accent_color"), 0, NAUI_CORNER_NONE);
	}
	naui_pop_clip_rect();
}

#pragma endregion

#pragma region Project

bool uph_sample_browser_add_to_project(const Naui_Path path, Uph_ResourceIndex *out_index)
{
	const uint32_t before = (uint32_t)naui_list_len(uph_state.project.samples);
	if (!uph_project_add_file(&uph_state.project, path))
	{
		naui_log(NAUI_LOG_WARNING, "Couldn't add sample (%s)", path.data);
		return false;
	}

	const uint32_t after = (uint32_t)naui_list_len(uph_state.project.samples);
	Uph_ResourceIndex index;
	if (after > before)
		index = after - 1;
	else if (uph_state.shared.selected_resource.type == UPH_RESOURCE_SAMPLE && uph_state.shared.selected_resource.index < after)
		index = uph_state.shared.selected_resource.index;
	else
		return false;

	uph_state.shared.selected_resource = (Uph_SelectedResourceState){ .type = UPH_RESOURCE_SAMPLE, .index = index };
	uph_action_refresh_block_defaults();

	if (out_index)
		*out_index = index;

	return true;
}

#pragma endregion

#pragma region Drag

static void _uph_sample_browser_render_drag(const void *payload)
{
	const Naui_Path *path = payload;
	const Naui_StringView name = naui_file_filename(path);
	char label[NAUI_STRING_MAX_SIZE];
	snprintf(label, sizeof(label), "%.*s", (int)name.length, name.data);
	uph_drag_label(naui_asset_image("uph_icon_wave"), label, naui_theme_color("naui_panel_tab_accent_color"));
}

static void _uph_sample_browser_stop_on_playback(void)
{
	static bool was_playing;
	const bool playing = uph_state.shared.song_timeline_playing || uph_midi_preview_playing();
	if (playing && !was_playing)
	{
		uph_audio_engine_preview_stop();
		s_sample_browser.load_next_pending = false;
		if (s_sample_browser.load)
			s_sample_browser.load_cancelled = true;
	}

	was_playing = playing;
}

void uph_sample_browser_end_frame(void)
{
	_uph_sample_browser_stop_on_playback();
	_uph_sample_browser_poll_scan();
	_uph_sample_browser_poll_load();
}

#pragma endregion

#pragma region Rows

static bool _uph_sample_browser_contains(const char *haystack, const char *needle, size_t needle_length)
{
	if (needle_length == 0)
		return true;

	for (const char *start = haystack; *start; start++)
	{
		size_t i = 0;
		while (i < needle_length && start[i] && tolower((unsigned char)start[i]) == tolower((unsigned char)needle[i]))
			i++;

		if (i == needle_length)
			return true;
	}

	return false;
}

static bool _uph_sample_browser_matches(const char *name, const char *query)
{
	const char *word = query;
	while (*word)
	{
		while (*word == ' ')
			word++;

		const char *end = word;
		while (*end && *end != ' ')
			end++;

		if (end > word && !_uph_sample_browser_contains(name, word, (size_t)(end - word)))
			return false;

		word = end;
	}

	return true;
}

static bool _uph_sample_browser_searching(void)
{
	for (size_t i = 0; i < s_sample_browser.filter.length; i++)
	{
		if (s_sample_browser.filter.data[i] != ' ')
			return true;
	}

	return false;
}

static void _uph_sample_browser_push_tree(uint32_t node)
{
	const Uph_SampleBrowserIndex *index = &s_sample_browser.index;
	naui_list_push(s_sample_browser.rows, node);

	const Uph_SampleBrowserNode *info = &index->nodes[node];
	if (!(info->flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED))
		return;

	for (uint32_t i = 0; i < info->child_count; i++)
		_uph_sample_browser_push_tree(info->first_child + i);
}

static void _uph_sample_browser_rebuild_rows(void)
{
	const Uph_SampleBrowserIndex *index = &s_sample_browser.index;
	naui_list_clear(s_sample_browser.rows);

	if (_uph_sample_browser_searching())
	{
		for (uint32_t i = 0; i < (uint32_t)naui_list_len(index->nodes); i++)
		{
			if (!(index->nodes[i].flags & UPH_SAMPLE_BROWSER_NODE_DIR) && _uph_sample_browser_matches(_uph_sample_browser_node_name(index, i), s_sample_browser.filter.data))
				naui_list_push(s_sample_browser.rows, i);
		}
	}
	else
	{
		for (uint32_t i = 0; i < index->root_count; i++)
			_uph_sample_browser_push_tree(i);
	}

	s_sample_browser.rows_dirty = false;
}

static int32_t _uph_sample_browser_row_of(uint32_t node)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(s_sample_browser.rows); i++)
	{
		if (s_sample_browser.rows[i] == node)
			return (int32_t)i;
	}

	return -1;
}

static bool _uph_sample_browser_is_dir(uint32_t node)
{
	return node < (uint32_t)naui_list_len(s_sample_browser.index.nodes) && (s_sample_browser.index.nodes[node].flags & UPH_SAMPLE_BROWSER_NODE_DIR);
}

static void _uph_sample_browser_set_expanded(uint32_t node, bool expanded)
{
	Uph_SampleBrowserNode *info = &s_sample_browser.index.nodes[node];
	const bool was_expanded = (info->flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED) != 0;
	if (was_expanded == expanded)
		return;

	info->flags ^= UPH_SAMPLE_BROWSER_NODE_EXPANDED;
	s_sample_browser.rows_dirty = true;
}

static void _uph_sample_browser_select(uint32_t node, bool preview)
{
	s_sample_browser.selected = node;
	if (preview && !_uph_sample_browser_is_dir(node) && uph_state.settings.browser.auto_preview)
		_uph_sample_browser_preview(_uph_sample_browser_node_path(&s_sample_browser.index, node));
}

static void _uph_sample_browser_activate(uint32_t node)
{
	if (_uph_sample_browser_is_dir(node))
	{
		_uph_sample_browser_set_expanded(node, !(s_sample_browser.index.nodes[node].flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED));
		return;
	}

	uph_sample_browser_add_to_project(naui_path_from_cstr(_uph_sample_browser_node_path(&s_sample_browser.index, node)), NULL);
}

static void _uph_sample_browser_relative_folder(uint32_t node, char *out, size_t out_size)
{
	const Uph_SampleBrowserIndex *index = &s_sample_browser.index;
	out[0] = '\0';

	const uint32_t parent = index->nodes[node].parent;
	if (parent == UPH_SAMPLE_BROWSER_NONE)
		return;

	uint32_t root = parent;
	while (index->nodes[root].parent != UPH_SAMPLE_BROWSER_NONE)
		root = index->nodes[root].parent;

	const char *parent_path = _uph_sample_browser_node_path(index, parent);
	const char *root_name = _uph_sample_browser_node_name(index, root);
	const size_t root_offset = (size_t)(root_name - _uph_sample_browser_node_path(index, root));
	snprintf(out, out_size, "%s", parent_path + root_offset);
}

static void _uph_sample_browser_row(uint32_t node, Uph_UIMenuID context_menu, bool searching, float row_height)
{
	const Uph_SampleBrowserIndex *index = &s_sample_browser.index;
	const Uph_SampleBrowserNode *info = &index->nodes[node];
	const bool is_dir = (info->flags & UPH_SAMPLE_BROWSER_NODE_DIR) != 0;
	const bool expanded = (info->flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED) != 0;
	const Leaf_ID row_id = leaf_id_indexed("uph_sample_browser_row", node);
	const bool hovered = uph_ui_widget_hovered(row_id);
	const bool selected = s_sample_browser.selected == node;
	const bool previewing = !is_dir && strcmp(s_sample_browser.previewing.data, _uph_sample_browser_node_path(index, node)) == 0 && uph_audio_engine_preview_playing();

	if (hovered)
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		{
			if (is_dir)
			{
				s_sample_browser.selected = node;
				_uph_sample_browser_set_expanded(node, !expanded);
			}
			else
			{
				_uph_sample_browser_select(node, true);
				const Naui_Path path = naui_path_from_cstr(_uph_sample_browser_node_path(index, node));
				uph_drag_begin(UPH_DRAG_TYPE_SAMPLE_FILE, &path, sizeof(path), _uph_sample_browser_render_drag);
			}
		}
		else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
		{
			s_sample_browser.selected = node;
			s_sample_browser.context_node = node;
			uph_ui_open_context_menu(context_menu);
		}
	}

	Leaf_Color background = LEAF_COLOR_TRANSPARENT;
	if (selected)
		background = uph_color_mix(naui_theme_color("uph_ui_frame_bg_color"), naui_theme_color("uph_ui_selection_color"), 0.3f);
	else if (hovered)
		background = naui_theme_color("uph_ui_frame_hovered_bg_color");

	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const float icon_size = NAUI_DPI(14.0f);
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	const Leaf_Color muted_color = naui_theme_color("uph_ui_text_disabled_color");
	const Leaf_Color icon_color = previewing ? naui_theme_color("naui_panel_tab_accent_color") : text_color;
	const float indent = searching ? 0.0f : NAUI_DPI(UPH_SAMPLE_BROWSER_INDENT) * (float)info->depth;

	leaf({
		.id = row_id,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(row_height)},
		.padding = {(int32_t)(NAUI_DPI(6.0f) + indent), (int32_t)NAUI_DPI(6.0f), 0, 0},
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(4.0f),
		.color = {background},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.clip_children = true
	})
	{
		if (!searching)
		{
			leaf({
				.size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
				.color = is_dir && info->child_count > 0 ? muted_color : LEAF_COLOR_TRANSPARENT,
				.image = (void*)naui_asset_image(expanded ? "uph_icon_arrow_down" : "uph_icon_arrow_right")
			});
		}

		leaf({
			.size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
			.color = is_dir ? muted_color : icon_color,
			.image = (void*)naui_asset_image(is_dir ? "uph_icon_folder" : "uph_icon_wave")
		});

		leaf_text(_uph_sample_browser_node_name(index, node), {
			.font_size = {font_size},
			.color = {text_color}
		});

		if (searching)
		{
			char folder[NAUI_PATH_MAX];
			_uph_sample_browser_relative_folder(node, folder, sizeof(folder));
			leaf_text(folder, {
				.font_size = {font_size},
				.color = {muted_color}
			});
		}
	}
}

#pragma endregion

#pragma region Panel

static void uph_sample_browser_on_attach(void)
{
	s_sample_browser.rows_dirty = true;
	if (naui_list_len(uph_state.settings.browser.folders) > 0)
		_uph_sample_browser_start_scan();
}

static void uph_sample_browser_on_detach(void)
{

}

static void uph_sample_browser_on_open(void)
{

}

static void uph_sample_browser_on_close(void)
{
	uph_audio_engine_preview_stop();
}

void uph_sample_browser_shutdown(void)
{
	_uph_sample_browser_cancel_scan();

	if (s_sample_browser.load)
	{
		if (naui_job_is_valid(s_sample_browser.load_job))
			naui_job_wait(s_sample_browser.load_job);

		uph_audio_engine_preview_close(&s_sample_browser.load->source);
		free(s_sample_browser.load);
		s_sample_browser.load = NULL;
		s_sample_browser.load_job = NAUI_JOB_INVALID;
	}

	_uph_sample_browser_index_free(&s_sample_browser.index);
	naui_list_free(s_sample_browser.rows);
	s_sample_browser.rows = NULL;
}

static void _uph_sample_browser_toolbar(void)
{
	const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
	const float button_size = NAUI_DPI(16.0f);
	const Leaf_Color tint = naui_theme_color("uph_ui_text_color");

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(4.0f)
	})
	{
		uph_ui_textfield(&s_sample_browser.filter, leaf_id("uph_sample_browser_search"), UPH_UI_TEXTFIELD_FLAG_SEARCH, NAUI_TR("sample_browser.search"));

		if (uph_ui_image_toggle_button(naui_asset_image("uph_icon_headphones"), leaf_id("uph_sample_browser_auto_preview"), (Naui_Vec2){ button_size, button_size }, tint, uph_state.settings.browser.auto_preview))
		{
			uph_state.settings.browser.auto_preview = !uph_state.settings.browser.auto_preview;
			if (!uph_state.settings.browser.auto_preview)
				uph_audio_engine_preview_stop();
		}

		if (uph_ui_image_button(naui_asset_image("uph_icon_reload"), leaf_id("uph_sample_browser_rescan"), (Naui_Vec2){ button_size, button_size }, tint))
			_uph_sample_browser_start_scan();

		if (uph_ui_image_button(naui_asset_image("uph_icon_plus"), leaf_id("uph_sample_browser_add_folder"), (Naui_Vec2){ button_size, button_size }, tint))
			_uph_sample_browser_open_folder_dialog();
	}
}

static void _uph_sample_browser_preview_strip(void)
{
	Uph_PreviewInfo info;
	if (!uph_audio_engine_preview_info(&info))
		return;

	const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const Naui_StringView name = naui_file_filename(info.path);
	const uint32_t rate = info.original_sample_rate ? info.original_sample_rate : uph_state.settings.audio.sample_rate;
	const double seconds = uph_frames_to_seconds(info.frame_count, uph_state.settings.audio.sample_rate);
	const char *channels = info.channel_type == UPH_SAMPLE_STEREO ? "Stereo" : "Mono";

	char title[NAUI_STRING_MAX_SIZE];
	snprintf(title, sizeof(title), "%.*s", (int)name.length, name.data);
	char details[64];
	if (seconds >= 60.0)
		snprintf(details, sizeof(details), "%d:%02d  %s  %u Hz", (int)(seconds / 60.0), (int)fmod(seconds, 60.0), channels, rate);
	else
		snprintf(details, sizeof(details), "%.2fs  %s  %u Hz", seconds, channels, rate);

	const Leaf_ID waveform_id = leaf_id("uph_sample_browser_waveform");
	if (uph_ui_widget_hovered(waveform_id))
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		{
			if (uph_audio_engine_preview_playing())
				uph_audio_engine_preview_stop();
			else
				uph_audio_engine_preview_restart();
		}
	}

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 0.5f)),
		.child_gap = NAUI_DPI(4.0f)
	})
	{
		leaf({
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(8.0f),
			.clip_children = true
		})
		{
			leaf({ .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT}, .clip_children = true })
				leaf_text(title, { .font_size = {font_size}, .color = {naui_theme_color("uph_ui_text_color")} });
			leaf_text(details, { .font_size = {font_size}, .color = {naui_theme_color("uph_ui_text_disabled_color")} });
		}

		leaf({
			.id = waveform_id,
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_SAMPLE_BROWSER_WAVEFORM_HEIGHT))},
			.color = {naui_theme_color("uph_resource_list_item_bg_color")},
			.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
			.custom_draw = (Leaf_CustomDrawFn)_uph_sample_browser_waveform_draw,
			.custom_draw_data = LEAF_DATA_SLICE(info)
		});
	}
}

static void _uph_sample_browser_empty_state(void)
{
	const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
		.padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x * 2.0f)),
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(8.0f)
	})
	{
		leaf_text(NAUI_TR("sample_browser.empty"), {
			.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE)},
			.color = {naui_theme_color("uph_ui_text_disabled_color")},
			.alignment = LEAF_TEXT_ALIGN_CENTER,
			.wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
		});

		if (uph_ui_text_button(NAUI_TR("sample_browser.add_folder"), leaf_id("uph_sample_browser_empty_add")))
			_uph_sample_browser_open_folder_dialog();
	}
}

static void _uph_sample_browser_status(void)
{
	const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
	char status[128];
	if (s_sample_browser.scan)
		snprintf(status, sizeof(status), "%s", NAUI_TR("sample_browser.indexing"));
	else if (_uph_sample_browser_searching())
		snprintf(status, sizeof(status), "%u / %u", (uint32_t)naui_list_len(s_sample_browser.rows), s_sample_browser.index.file_count);
	else
		snprintf(status, sizeof(status), "%u", s_sample_browser.index.file_count);

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 0.5f)),
		.border = {
			.width = 1,
			.color = {naui_theme_color("uph_ui_frame_border")},
			.sides = LEAF_SIDE_TOP
		},
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
	})
	{
		leaf_text(status, { .font_size = {NAUI_DPI(UPH_UI_FONT_SIZE)}, .color = {naui_theme_color("uph_ui_text_disabled_color")} });
	}
}

static void _uph_sample_browser_keyboard(void)
{
	if (naui_panel_focused() != naui_panel_current() || uph_ui_textfield_active(leaf_id("uph_sample_browser_search")) || uph_ui_any_menu_open() || uph_dialog_blocks_input())
		return;

	const int32_t row_count = (int32_t)naui_list_len(s_sample_browser.rows);
	if (row_count == 0)
		return;

	const int32_t current = _uph_sample_browser_row_of(s_sample_browser.selected);
	const uint32_t node = s_sample_browser.selected;
	const bool searching = _uph_sample_browser_searching();

	if (naui_key_pressed(NAUI_KEY_DOWN) || naui_key_pressed(NAUI_KEY_UP))
	{
		int32_t next = current < 0 ? 0 : current + (naui_key_pressed(NAUI_KEY_DOWN) ? 1 : -1);
		next = NAUI_CLAMP(next, 0, row_count - 1);
		_uph_sample_browser_select(s_sample_browser.rows[next], true);
		s_sample_browser.scroll_to_selected = true;
	}
	else if (current < 0)
		return;
	else if (naui_key_pressed(NAUI_KEY_RIGHT) && !searching && _uph_sample_browser_is_dir(node))
		_uph_sample_browser_set_expanded(node, true);
	else if (naui_key_pressed(NAUI_KEY_LEFT) && !searching)
	{
		const Uph_SampleBrowserNode *info = &s_sample_browser.index.nodes[node];
		if (_uph_sample_browser_is_dir(node) && (info->flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED))
			_uph_sample_browser_set_expanded(node, false);
		else if (info->parent != UPH_SAMPLE_BROWSER_NONE)
		{
			s_sample_browser.selected = info->parent;
			s_sample_browser.scroll_to_selected = true;
		}
	}
	else if (naui_key_pressed(NAUI_KEY_ENTER))
		_uph_sample_browser_activate(node);
	else if (naui_key_pressed(NAUI_KEY_SPACE) && !_uph_sample_browser_is_dir(node))
	{
		if (uph_audio_engine_preview_playing())
			uph_audio_engine_preview_stop();
		else
			_uph_sample_browser_preview(_uph_sample_browser_node_path(&s_sample_browser.index, node));
	}
}

static void _uph_sample_browser_context_menu_items(Uph_UIMenuID context_menu)
{
	const uint32_t node = s_sample_browser.context_node;
	if (node >= (uint32_t)naui_list_len(s_sample_browser.index.nodes))
		return;

	const Uph_SampleBrowserNode info = s_sample_browser.index.nodes[node];
	const char *path = _uph_sample_browser_node_path(&s_sample_browser.index, node);

	if (!(info.flags & UPH_SAMPLE_BROWSER_NODE_DIR))
	{
		if (uph_ui_menu_item(context_menu, NAUI_TR("sample_browser.add_to_project"), leaf_id("uph_sample_browser_menu_add")))
			uph_sample_browser_add_to_project(naui_path_from_cstr(path), NULL);

		if (uph_ui_menu_item(context_menu, NAUI_TR("sample_browser.preview"), leaf_id("uph_sample_browser_menu_preview")))
			_uph_sample_browser_preview(path);
	}

	if (info.flags & UPH_SAMPLE_BROWSER_NODE_ROOT)
	{
		if (uph_ui_menu_item(context_menu, NAUI_TR("sample_browser.remove_folder"), leaf_id("uph_sample_browser_menu_remove")))
			_uph_sample_browser_remove_folder(path);
	}

	if (uph_ui_menu_item(context_menu, NAUI_TR("sample_browser.rescan"), leaf_id("uph_sample_browser_menu_rescan")))
		_uph_sample_browser_start_scan();
}

static void uph_sample_browser_on_update(void)
{
	naui_panel_set_title(naui_panel_current(), NAUI_TR("sample_browser.title"));

	if (!naui_strings_equal(s_sample_browser.filter, s_sample_browser.applied_filter, true))
	{
		s_sample_browser.applied_filter = s_sample_browser.filter;
		s_sample_browser.rows_dirty = true;
		s_sample_browser.scroll = 0.0f;
	}

	if (s_sample_browser.rows_dirty)
		_uph_sample_browser_rebuild_rows();

	_uph_sample_browser_keyboard();

	if (s_sample_browser.rows_dirty)
		_uph_sample_browser_rebuild_rows();

	Uph_UIMenuID context_menu = uph_ui_context_menu(leaf_id("uph_sample_browser_context_menu"));

	_uph_sample_browser_toolbar();
	_uph_sample_browser_preview_strip();

	if (naui_list_len(uph_state.settings.browser.folders) == 0)
	{
		_uph_sample_browser_empty_state();
		_uph_sample_browser_context_menu_items(context_menu);
		return;
	}

	const bool searching = _uph_sample_browser_searching();
	const float row_height = NAUI_DPI(UPH_SAMPLE_BROWSER_ROW_HEIGHT);
	const uint32_t row_count = (uint32_t)naui_list_len(s_sample_browser.rows);
	const Leaf_ID scroll_id = leaf_id("uph_sample_browser_scrollbar");

	const Leaf_BoundingBox view_box = leaf_get_bounding_box((Leaf_ID){ .value = scroll_id.value + 2 });
	const float view_height = view_box.height > 0.0f ? view_box.height : row_height * 40.0f;

	if (s_sample_browser.scroll_to_selected)
	{
		const int32_t row = _uph_sample_browser_row_of(s_sample_browser.selected);
		if (row >= 0)
		{
			const float top = (float)row * row_height;
			if (top < s_sample_browser.scroll)
				s_sample_browser.scroll = top;
			else if (top + row_height > s_sample_browser.scroll + view_height)
				s_sample_browser.scroll = top + row_height - view_height;
		}
		s_sample_browser.scroll_to_selected = false;
	}

	Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(UPH_UI_SCROLL_DIRECTION_VERTICAL, &s_sample_browser.scroll, scroll_id);
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(4.0f), 0)
	})
	{
		uint32_t first = (uint32_t)(s_sample_browser.scroll / row_height);
		uint32_t last = first + (uint32_t)(view_height / row_height) + 2;
		if (first > row_count)
			first = row_count;
		if (last > row_count)
			last = row_count;

		if (first > 0)
			leaf({ .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED((float)first * row_height)} });

		for (uint32_t i = first; i < last; i++)
			_uph_sample_browser_row(s_sample_browser.rows[i], context_menu, searching, row_height);

		if (last < row_count)
			leaf({ .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED((float)(row_count - last) * row_height)} });

		if (row_count == 0 && !s_sample_browser.scan)
		{
			leaf({
				.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
				.padding = LEAF_PADDING_ALL(NAUI_DPI(8.0f)),
				.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
			})
			{
				leaf_text(NAUI_TR(searching ? "sample_browser.no_matches" : "sample_browser.no_samples"), {
					.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE)},
					.color = {naui_theme_color("uph_ui_text_disabled_color")}
				});
			}
		}
	}
	uph_ui_end_scroll_container(&scroll_container);

	_uph_sample_browser_status();
	_uph_sample_browser_context_menu_items(context_menu);
}

#pragma endregion
