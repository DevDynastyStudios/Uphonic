NAUI_PANEL(uph_sample_browser)

#define UPH_SAMPLE_BROWSER_ADD_FOLDER_DIALOG_KEY "uph_sample_browser_add_folder"
#define UPH_SAMPLE_BROWSER_MAX_NODES 500000u
#define UPH_SAMPLE_BROWSER_MAX_DEPTH 32
#define UPH_SAMPLE_BROWSER_ROW_HEIGHT 22
#define UPH_SAMPLE_BROWSER_INDENT 14
#define UPH_SAMPLE_BROWSER_CLICK_THRESHOLD 4
#define UPH_SAMPLE_BROWSER_WAVEFORM_HEIGHT 56
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

typedef struct
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
	uint32_t pressed_node;
	Naui_Vec2 press_mouse;
}
Uph_SampleBrowserState;

static Uph_SampleBrowserState uph__sample_browser_state = {
	.selected = UPH_SAMPLE_BROWSER_NONE,
	.context_node = UPH_SAMPLE_BROWSER_NONE,
	.pressed_node = UPH_SAMPLE_BROWSER_NONE
};

#pragma region Index

static uint32_t uph__sample_browser_pool_push(Uph_SampleBrowserIndex *index, const char *text, size_t length)
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

static void uph__sample_browser_index_free(Uph_SampleBrowserIndex *index)
{
	naui_list_free(index->nodes);
	free(index->pool);
	*index = (Uph_SampleBrowserIndex){ 0 };
}

static const char *uph__sample_browser_node_path(const Uph_SampleBrowserIndex *index, uint32_t node)
{
	return index->pool + index->nodes[node].path;
}

static const char *uph__sample_browser_node_name(const Uph_SampleBrowserIndex *index, uint32_t node)
{
	return index->pool + index->nodes[node].name;
}

static int uph__sample_browser_compare_nocase(const char *a, const char *b)
{
	while (*a && tolower((unsigned char)*a) == tolower((unsigned char)*b))
	{
		a++;
		b++;
	}

	return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static bool uph__sample_browser_is_audio(const char *name)
{
	const char *dot = strrchr(name, '.');
	if (!dot)
		return false;

	for (const char **ext = uph_audio_engine_extensions; *ext; ext++)
	{
		if (uph__sample_browser_compare_nocase(dot, *ext) == 0)
			return true;
	}

	return false;
}

static int uph__sample_browser_entry_compare(const void *a, const void *b)
{
	const Uph_SampleBrowserEntry *left = a;
	const Uph_SampleBrowserEntry *right = b;
	if (left->is_directory != right->is_directory)
		return left->is_directory ? -1 : 1;

	return uph__sample_browser_compare_nocase(left->name, right->name);
}

static bool uph__sample_browser_push_node(Uph_SampleBrowserIndex *index, const char *path, size_t path_length, size_t name_start, uint32_t parent, uint16_t depth, Uph_SampleBrowserNodeFlags flags)
{
	if ((uint32_t)naui_list_len(index->nodes) >= UPH_SAMPLE_BROWSER_MAX_NODES)
	{
		index->truncated = true;
		return false;
	}

	const uint32_t offset = uph__sample_browser_pool_push(index, path, path_length);
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

static void uph__sample_browser_scan_directory(Uph_SampleBrowserIndex *index, uint32_t node_index, Naui_List(uint32_t) *stack)
{
	const Naui_Path path = naui_path_from_cstr(uph__sample_browser_node_path(index, node_index));
	const uint16_t depth = index->nodes[node_index].depth;
	Naui_List(Uph_SampleBrowserEntry) entries = NULL;

	Naui_DirIterator it = naui_dir_iterator_open(path, NULL, NULL, false);
	while (naui_dir_iterator_valid(&it))
	{
		const Naui_StringView name = naui_file_filename(&it.entry.path);
		const bool usable = name.length > 0 && name.data[0] != '.' &&
			(it.entry.is_directory || uph__sample_browser_is_audio(name.data));

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
		qsort(entries, entry_count, sizeof(entries[0]), uph__sample_browser_entry_compare);

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
			if (uph__sample_browser_push_node(index, child, (size_t)written, name_start, node_index, (uint16_t)(depth + 1), flags))
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

static void uph__sample_browser_scan_job(void *data, char *err_buf, size_t err_size)
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
		uph__sample_browser_push_node(index, folder.data, length, name_start, UPH_SAMPLE_BROWSER_NONE, 0, UPH_SAMPLE_BROWSER_NODE_DIR | UPH_SAMPLE_BROWSER_NODE_ROOT);
	}

	index->root_count = (uint32_t)naui_list_len(index->nodes);

	Naui_List(uint32_t) stack = NULL;
	for (uint32_t i = index->root_count; i > 0; i--)
		naui_list_push(stack, i - 1);

	while (naui_list_len(stack) > 0 && !atomic_load_explicit(&scan->cancel, memory_order_relaxed))
	{
		const uint32_t node = naui_list_pop(stack);
		uph__sample_browser_scan_directory(index, node, &stack);
	}

	naui_list_free(stack);
}

static void uph__sample_browser_scan_free(Uph_SampleBrowserScan *scan)
{
	naui_list_free(scan->folders);
	uph__sample_browser_index_free(&scan->result);
	free(scan);
}

static void uph__sample_browser_cancel_scan(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	if (!state->scan)
		return;

	atomic_store(&state->scan->cancel, true);
	if (naui_job_is_valid(state->scan_job))
		naui_job_wait(state->scan_job);

	uph__sample_browser_scan_free(state->scan);
	state->scan = NULL;
	state->scan_job = NAUI_JOB_INVALID;
}

static void uph__sample_browser_start_scan(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	if (state->scan)
	{
		state->rescan_requested = true;
		return;
	}

	state->rescan_requested = false;

	Uph_SampleBrowserScan *scan = calloc(1, sizeof(Uph_SampleBrowserScan));
	if (!scan)
		return;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.settings.browser.folders); i++)
		naui_list_push(scan->folders, naui_path_normalize(uph_state.settings.browser.folders[i]));

	atomic_init(&scan->cancel, false);

	if (naui_job_submit(&state->scan_job, uph__sample_browser_scan_job, scan) != NAUI_JOB_SUBMIT_OK)
	{
		naui_log(NAUI_LOG_WARNING, "Sample browser: couldn't queue folder scan");
		uph__sample_browser_scan_free(scan);
		state->scan_job = NAUI_JOB_INVALID;
		return;
	}

	state->scan = scan;
}

static uint32_t uph__sample_browser_find_path(const Uph_SampleBrowserIndex *index, const char *path)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(index->nodes); i++)
	{
		if (strcmp(uph__sample_browser_node_path(index, i), path) == 0)
			return i;
	}

	return UPH_SAMPLE_BROWSER_NONE;
}

static void uph__sample_browser_adopt_index(Uph_SampleBrowserIndex *fresh)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	Uph_SampleBrowserIndex *old = &state->index;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(old->nodes); i++)
	{
		if (!(old->nodes[i].flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED))
			continue;

		const uint32_t match = uph__sample_browser_find_path(fresh, uph__sample_browser_node_path(old, i));
		if (match != UPH_SAMPLE_BROWSER_NONE)
			fresh->nodes[match].flags |= UPH_SAMPLE_BROWSER_NODE_EXPANDED;
	}

	uint32_t selected = UPH_SAMPLE_BROWSER_NONE;
	if (state->selected < (uint32_t)naui_list_len(old->nodes))
		selected = uph__sample_browser_find_path(fresh, uph__sample_browser_node_path(old, state->selected));

	uph__sample_browser_index_free(old);
	*old = *fresh;
	*fresh = (Uph_SampleBrowserIndex){ 0 };

	state->selected = selected;
	state->context_node = UPH_SAMPLE_BROWSER_NONE;
	state->rows_dirty = true;
}

static void uph__sample_browser_poll_scan(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	Uph_SampleBrowserScan *scan = state->scan;
	if (!scan || !naui_job_is_done(state->scan_job))
		return;

	naui_job_release(state->scan_job);
	state->scan_job = NAUI_JOB_INVALID;

	if (scan->result.truncated)
		naui_log(NAUI_LOG_WARNING, "Sample browser: index truncated at %u entries", UPH_SAMPLE_BROWSER_MAX_NODES);

	uph__sample_browser_adopt_index(&scan->result);
	uph__sample_browser_scan_free(scan);
	state->scan = NULL;

	if (state->rescan_requested)
		uph__sample_browser_start_scan();
}

#pragma endregion

#pragma region Folders

static void uph__sample_browser_add_folder(const Naui_Path folder)
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
	uph__sample_browser_start_scan();
}

static void uph__sample_browser_remove_folder(const char *path)
{
	Uph_BrowserSettings *browser = &uph_state.settings.browser;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(browser->folders); i++)
	{
		if (strcmp(naui_path_normalize(browser->folders[i]).data, path) != 0)
			continue;

		naui_list_remove(browser->folders, i);
		uph_settings_save();
		uph__sample_browser_start_scan();
		return;
	}
}

static void uph__sample_browser_folder_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	(void)user_data;
	if (is_cancelled || result->count == 0)
		return;

	uph__sample_browser_add_folder(result->paths[0]);
}

static void uph__sample_browser_open_folder_dialog(void)
{
	if (!naui_dialog_open_folder(UPH_SAMPLE_BROWSER_ADD_FOLDER_DIALOG_KEY, NULL, uph__sample_browser_folder_picked, NULL))
		return;

	naui_dialog_title_set(UPH_SAMPLE_BROWSER_ADD_FOLDER_DIALOG_KEY, NAUI_TR("sample_browser.add_folder"));
}

#pragma endregion

#pragma region Preview

static void uph__sample_browser_load_job(void *data, char *err_buf, size_t err_size)
{
	(void)err_buf;
	(void)err_size;
	Uph_SampleBrowserLoad *load = data;
	load->source = uph_audio_engine_preview_open(load->path);
}

static void uph__sample_browser_start_load(const Naui_Path path)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	Uph_SampleBrowserLoad *load = calloc(1, sizeof(Uph_SampleBrowserLoad));
	if (!load)
		return;

	load->path = path;
	if (naui_job_submit(&state->load_job, uph__sample_browser_load_job, load) != NAUI_JOB_SUBMIT_OK)
	{
		free(load);
		state->load_job = NAUI_JOB_INVALID;
		return;
	}

	state->load = load;
}

static void uph__sample_browser_preview(const char *path)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	const Naui_Path target = naui_path_from_cstr(path);
	uph_audio_engine_preview_stop();

	if (state->load)
	{
		state->load_cancelled = false;
		state->load_next = target;
		state->load_next_pending = true;
		return;
	}

	Uph_PreviewInfo current;
	if (uph_audio_engine_preview_info(&current) && strcmp(current.path->data, target.data) == 0)
	{
		uph_audio_engine_preview_restart();
		return;
	}

	uph__sample_browser_start_load(target);
}

static void uph__sample_browser_poll_load(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	Uph_SampleBrowserLoad *load = state->load;
	if (!load || !naui_job_is_done(state->load_job))
		return;

	naui_job_release(state->load_job);
	state->load_job = NAUI_JOB_INVALID;
	state->load = NULL;

	const bool stale = state->load_next_pending;
	const bool cancelled = state->load_cancelled;
	state->load_cancelled = false;
	if (!stale && !cancelled && uph_audio_engine_preview_source_valid(&load->source))
	{
		uph_audio_engine_preview_play(load->source);
		state->previewing = load->path;
	}
	else
		uph_audio_engine_preview_close(&load->source);

	free(load);

	if (stale)
	{
		state->load_next_pending = false;
		uph__sample_browser_preview(state->load_next.data);
	}
}

static void uph__sample_browser_waveform_draw(Leaf_BoundingBox box, const Uph_PreviewInfo *info)
{
	if (!info->peaks || box.width <= 1.0f || box.height <= 1.0f)
		return;

	const uint64_t bin_count = info->peak_total;
	const uint64_t ready_count = info->peak_count < bin_count ? info->peak_count : bin_count;
	if (bin_count == 0 || ready_count == 0)
		return;

	const int32_t inset = NAUI_DPI(3);
	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	uph_ui_waveform_peaks(
		(Naui_Vec2){ box.x, box.y + inset },
		(Naui_Vec2){ box.width, box.height - inset * 2.0f },
		uph_state.colors.sample_browser_item_fg,
		info->peaks,
		ready_count,
		bin_count,
		1.0f
	);

	if (uph_audio_engine_preview_playing())
	{
		const float progress = (float)NAUI_CLAMP(uph_audio_engine_preview_progress(), 0.0, 1.0);
		naui_fill_rect((Naui_Vec2){ box.x + progress * box.width, box.y }, (Naui_Vec2){ NAUI_DPI(2), box.height }, uph_state.colors.panel_tab_accent, 0, NAUI_CORNER_NONE);
	}
	naui_pop_clip_rect();
}

#pragma endregion

#pragma region Project

bool uph_sample_browser_add_to_project(const Naui_Path path, Uph_SampleLoadedFn on_loaded, void *userdata)
{
	if (naui_path_exists(path) && !naui_path_is_directory(path) && uph_project_add_sample_file(&uph_state.project, path, on_loaded, userdata))
		return true;

	naui_log(NAUI_LOG_WARNING, "Couldn't add sample (%s)", path.data);
	return false;
}

#pragma endregion

#pragma region Drag

static void uph__sample_browser_render_drag(const void *payload)
{
	const Naui_Path *path = payload;
	const Naui_StringView name = naui_file_filename(path);
	char label[NAUI_STRING_MAX_SIZE];
	snprintf(label, sizeof(label), "%.*s", (int)name.length, name.data);
	uph_drag_label(uph_state.icons.wave, label, uph_state.colors.panel_tab_accent);
}

static void uph__sample_browser_stop_on_playback(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	static bool was_playing;
	const bool playing = uph_state.shared.song_timeline_playing || uph_midi_preview_playing();
	if (playing && !was_playing)
	{
		uph_audio_engine_preview_stop();
		state->load_next_pending = false;
		if (state->load)
			state->load_cancelled = true;
	}

	was_playing = playing;
}

void uph_sample_browser_end_frame(void)
{
	uph__sample_browser_stop_on_playback();
	uph__sample_browser_poll_scan();
	uph__sample_browser_poll_load();
}

#pragma endregion

#pragma region Rows

static bool uph__sample_browser_contains(const char *haystack, const char *needle, size_t needle_length)
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

static bool uph__sample_browser_matches(const char *name, const char *query)
{
	const char *word = query;
	while (*word)
	{
		while (*word == ' ')
			word++;

		const char *end = word;
		while (*end && *end != ' ')
			end++;

		if (end > word && !uph__sample_browser_contains(name, word, (size_t)(end - word)))
			return false;

		word = end;
	}

	return true;
}

static bool uph__sample_browser_searching(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	for (size_t i = 0; i < state->filter.length; i++)
	{
		if (state->filter.data[i] != ' ')
			return true;
	}

	return false;
}

static void uph__sample_browser_push_tree(uint32_t node)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	const Uph_SampleBrowserIndex *index = &state->index;
	naui_list_push(state->rows, node);

	const Uph_SampleBrowserNode *info = &index->nodes[node];
	if (!(info->flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED))
		return;

	for (uint32_t i = 0; i < info->child_count; i++)
		uph__sample_browser_push_tree(info->first_child + i);
}

static void uph__sample_browser_rebuild_rows(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	const Uph_SampleBrowserIndex *index = &state->index;
	naui_list_clear(state->rows);

	if (uph__sample_browser_searching())
	{
		for (uint32_t i = 0; i < (uint32_t)naui_list_len(index->nodes); i++)
		{
			if (!(index->nodes[i].flags & UPH_SAMPLE_BROWSER_NODE_DIR) && uph__sample_browser_matches(uph__sample_browser_node_name(index, i), state->filter.data))
				naui_list_push(state->rows, i);
		}
	}
	else
	{
		for (uint32_t i = 0; i < index->root_count; i++)
			uph__sample_browser_push_tree(i);
	}

	state->rows_dirty = false;
}

static int32_t uph__sample_browser_row_of(uint32_t node)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->rows); i++)
	{
		if (state->rows[i] == node)
			return (int32_t)i;
	}

	return -1;
}

static bool uph__sample_browser_is_dir(uint32_t node)
{
	return node < (uint32_t)naui_list_len(uph__sample_browser_state.index.nodes) && (uph__sample_browser_state.index.nodes[node].flags & UPH_SAMPLE_BROWSER_NODE_DIR);
}

static void uph__sample_browser_set_expanded(uint32_t node, bool expanded)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	Uph_SampleBrowserNode *info = &state->index.nodes[node];
	const bool was_expanded = (info->flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED) != 0;
	if (was_expanded == expanded)
		return;

	info->flags ^= UPH_SAMPLE_BROWSER_NODE_EXPANDED;
	state->rows_dirty = true;
}

static void uph__sample_browser_select(uint32_t node, bool preview)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	state->selected = node;
	if (preview && !uph__sample_browser_is_dir(node) && uph_state.settings.browser.auto_preview)
		uph__sample_browser_preview(uph__sample_browser_node_path(&state->index, node));
}

static void uph__sample_browser_activate(uint32_t node)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	if (uph__sample_browser_is_dir(node))
	{
		uph__sample_browser_set_expanded(node, !(state->index.nodes[node].flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED));
		return;
	}

	uph_sample_browser_add_to_project(naui_path_from_cstr(uph__sample_browser_node_path(&state->index, node)), NULL, NULL);
}

static void uph__sample_browser_relative_folder(uint32_t node, char *out, size_t out_size)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	const Uph_SampleBrowserIndex *index = &state->index;
	out[0] = '\0';

	const uint32_t parent = index->nodes[node].parent;
	if (parent == UPH_SAMPLE_BROWSER_NONE)
		return;

	uint32_t root = parent;
	while (index->nodes[root].parent != UPH_SAMPLE_BROWSER_NONE)
		root = index->nodes[root].parent;

	const char *parent_path = uph__sample_browser_node_path(index, parent);
	const char *root_name = uph__sample_browser_node_name(index, root);
	const size_t root_offset = (size_t)(root_name - uph__sample_browser_node_path(index, root));
	snprintf(out, out_size, "%s", parent_path + root_offset);
}

static void uph__sample_browser_row(uint32_t node, Uph_UIMenuID context_menu, bool searching, float row_height)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	const Uph_SampleBrowserIndex *index = &state->index;
	const Uph_SampleBrowserNode *info = &index->nodes[node];
	const bool is_dir = (info->flags & UPH_SAMPLE_BROWSER_NODE_DIR) != 0;
	const bool expanded = (info->flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED) != 0;
	const bool selected = state->selected == node;
	const bool previewing = !is_dir && strcmp(state->previewing.data, uph__sample_browser_node_path(index, node)) == 0 && uph_audio_engine_preview_playing();
	const Leaf_ID row_id = leaf_id_indexed("uph_sample_browser_row", node);
	const bool hovered = uph_ui_widget_hovered(row_id);

	if (hovered)
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		{
			if (is_dir)
			{
				state->selected = node;
				uph__sample_browser_set_expanded(node, !expanded);
			}
			else
			{
				uph__sample_browser_select(node, false);
				state->pressed_node = node;
				state->press_mouse = (Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() };
				const Naui_Path path = naui_path_from_cstr(uph__sample_browser_node_path(index, node));
				uph_drag_begin(UPH_DRAG_TYPE_SAMPLE_FILE, &path, sizeof(path), uph__sample_browser_render_drag);
			}
		}
		else if (naui_mouse_released(NAUI_MOUSE_LEFT) && state->pressed_node == node)
		{
			const float dx = (float)naui_mouse_x() - state->press_mouse.x;
			const float dy = (float)naui_mouse_y() - state->press_mouse.y;
			const int32_t threshold = NAUI_DPI(UPH_SAMPLE_BROWSER_CLICK_THRESHOLD);
			if (dx * dx + dy * dy < threshold * threshold)
				uph__sample_browser_select(node, true);
			state->pressed_node = UPH_SAMPLE_BROWSER_NONE;
		}
		else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
		{
			state->selected = node;
			state->context_node = node;
			uph_ui_open_context_menu(context_menu);
		}
	}

	Leaf_Color background = LEAF_COLOR_TRANSPARENT;
	if (selected)
		background = uph_color_mix(uph_state.colors.ui_frame_bg, uph_state.colors.ui_selection, 0.3f);
	else if (hovered)
		background = uph_state.colors.ui_frame_hovered_bg;

	const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const int32_t icon_size = NAUI_DPI(14);
	const float indent = searching ? 0.0f : NAUI_DPI(UPH_SAMPLE_BROWSER_INDENT) * (float)info->depth;
	const Leaf_Color text_color = uph_state.colors.ui_text;
	const Leaf_Color muted_color = uph_state.colors.ui_text_disabled;
	const Leaf_Color icon_color = previewing ? uph_state.colors.panel_tab_accent : text_color;

	leaf({
		.id = row_id,
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(row_height) },
		.padding = { .left = (int32_t)(NAUI_DPI(6) + indent), .right = (int32_t)NAUI_DPI(6), .top = 0, .bottom = 0 },
		.color = background,
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.child_gap = NAUI_DPI(4),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.clip_children = true
	})
	{
		if (!searching)
		{
			leaf({
				.image = (void*)(expanded ? uph_state.icons.arrow_down : uph_state.icons.arrow_right),
				.size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
				.color = is_dir && info->child_count > 0 ? muted_color : LEAF_COLOR_TRANSPARENT
			});
		}

		leaf({
			.image = (void*)(is_dir ? uph_state.icons.folder : uph_state.icons.wave),
			.size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
			.color = is_dir ? muted_color : icon_color
		});

		leaf_text(uph__sample_browser_node_name(index, node), {
			.color = text_color,
			.font_size = LEAF_SIZE_FIXED(font_size)
		});

		if (searching)
		{
			char folder[NAUI_PATH_MAX];
			uph__sample_browser_relative_folder(node, folder, sizeof(folder));
			leaf_text(folder, {
				.color = muted_color,
				.font_size = LEAF_SIZE_FIXED(font_size)
			});
		}
	}
}

#pragma endregion

#pragma region Panel

static void uph_sample_browser_on_attach(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	state->rows_dirty = true;
	if (naui_list_len(uph_state.settings.browser.folders) > 0)
		uph__sample_browser_start_scan();
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
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	uph__sample_browser_cancel_scan();

	if (state->load)
	{
		if (naui_job_is_valid(state->load_job))
			naui_job_wait(state->load_job);

		uph_audio_engine_preview_close(&state->load->source);
		free(state->load);
		state->load = NULL;
		state->load_job = NAUI_JOB_INVALID;
	}

	uph__sample_browser_index_free(&state->index);
	naui_list_free(state->rows);
	state->rows = NULL;
}

static void uph__sample_browser_toolbar(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
	const int32_t button_size = NAUI_DPI(16);
	const Leaf_Color tint = uph_state.colors.ui_text;

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(4),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		uph_ui_textfield(&state->filter, leaf_id("uph_sample_browser_search"), UPH_UI_TEXTFIELD_FLAG_SEARCH, NAUI_TR("sample_browser.search"));

		if (uph_ui_image_toggle_button(uph_state.icons.headphones, leaf_id("uph_sample_browser_auto_preview"), (Naui_Vec2){ button_size, button_size }, tint, uph_state.settings.browser.auto_preview))
		{
			uph_state.settings.browser.auto_preview = !uph_state.settings.browser.auto_preview;
			if (!uph_state.settings.browser.auto_preview)
				uph_audio_engine_preview_stop();
		}

		if (uph_ui_image_button(uph_state.icons.reload, leaf_id("uph_sample_browser_rescan"), (Naui_Vec2){ button_size, button_size }, tint))
			uph__sample_browser_start_scan();

		if (uph_ui_image_button(uph_state.icons.plus, leaf_id("uph_sample_browser_add_folder"), (Naui_Vec2){ button_size, button_size }, tint))
			uph__sample_browser_open_folder_dialog();
	}
}

static void uph__sample_browser_preview_strip(void)
{
	Uph_PreviewInfo info;
	if (!uph_audio_engine_preview_info(&info))
		return;

	const Naui_StringView name = naui_file_filename(info.path);
	const uint32_t rate = info.original_sample_rate ? info.original_sample_rate : uph_state.settings.audio.sample_rate;
	const double seconds = uph_frames_to_seconds(info.frame_count, uph_state.settings.audio.sample_rate);
	const char *channels = info.channel_type == UPH_SAMPLE_STEREO ? "Stereo" : "Mono";
	const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
	const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);

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
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 0.5f)),
		.child_gap = NAUI_DPI(4)
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(8),
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.clip_children = true
		})
		{
			leaf({ .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT }, .clip_children = true })
				leaf_text(title, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED(font_size) });
			leaf_text(details, { .color = uph_state.colors.ui_text_disabled, .font_size = LEAF_SIZE_FIXED(font_size) });
		}

		leaf({
			.id = waveform_id,
			.custom_draw = (Leaf_CustomDrawFn)uph__sample_browser_waveform_draw,
			.custom_draw_data = LEAF_DATA_SLICE(info),
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SAMPLE_BROWSER_WAVEFORM_HEIGHT)) },
			.color = uph_state.colors.sample_browser_item_bg,
			.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL)
		});
	}
}

static void uph__sample_browser_empty_state(void)
{
	const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
		.padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x * 2)),
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(8)
	})
	{
		leaf_text(NAUI_TR("sample_browser.empty"), {
			.color = uph_state.colors.ui_text_disabled,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE)),
			.alignment = LEAF_TEXT_ALIGN_CENTER,
			.wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
		});

		if (uph_ui_text_button(NAUI_TR("sample_browser.add_folder"), leaf_id("uph_sample_browser_empty_add")))
			uph__sample_browser_open_folder_dialog();
	}
}

static void uph__sample_browser_status(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
	char status[128];
	if (state->scan)
		snprintf(status, sizeof(status), "%s", NAUI_TR("sample_browser.indexing"));
	else if (uph__sample_browser_searching())
		snprintf(status, sizeof(status), "%u / %u", (uint32_t)naui_list_len(state->rows), state->index.file_count);
	else
		snprintf(status, sizeof(status), "%u", state->index.file_count);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 0.5f)),
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_TOP,
			.color = uph_state.colors.ui_frame_border
		},
		.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf_text(status, { .color = uph_state.colors.ui_text_disabled, .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE)) });
	}
}

static void uph__sample_browser_keyboard(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	if (naui_panel_focused() != uph_state.panels.sample_browser || uph_ui_any_menu_open())
		return;

	const int32_t row_count = (int32_t)naui_list_len(state->rows);
	if (row_count == 0)
		return;

	const uint32_t node = state->selected;
	const int32_t current = uph__sample_browser_row_of(state->selected);
	const bool searching = uph__sample_browser_searching();

	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;
	const bool down = naui_keybind_pressed(keybinds->browser_down);
	if (down || naui_keybind_pressed(keybinds->browser_up))
	{
		int32_t next = current < 0 ? 0 : current + (down ? 1 : -1);
		next = NAUI_CLAMP(next, 0, row_count - 1);
		uph__sample_browser_select(state->rows[next], true);
		state->scroll_to_selected = true;
	}
	else if (current < 0)
		return;
	else if (naui_keybind_pressed(keybinds->browser_expand) && !searching && uph__sample_browser_is_dir(node))
		uph__sample_browser_set_expanded(node, true);
	else if (naui_keybind_pressed(keybinds->browser_collapse) && !searching)
	{
		const Uph_SampleBrowserNode *info = &state->index.nodes[node];
		if (uph__sample_browser_is_dir(node) && (info->flags & UPH_SAMPLE_BROWSER_NODE_EXPANDED))
			uph__sample_browser_set_expanded(node, false);
		else if (info->parent != UPH_SAMPLE_BROWSER_NONE)
		{
			state->selected = info->parent;
			state->scroll_to_selected = true;
		}
	}
	else if (naui_keybind_pressed(keybinds->browser_activate))
		uph__sample_browser_activate(node);
	else if (naui_keybind_pressed(keybinds->browser_preview) && !uph__sample_browser_is_dir(node))
	{
		if (uph_audio_engine_preview_playing())
			uph_audio_engine_preview_stop();
		else
			uph__sample_browser_preview(uph__sample_browser_node_path(&state->index, node));
	}
}

static void uph__sample_browser_context_menu_items(Uph_UIMenuID context_menu)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;
	const uint32_t node = state->context_node;
	if (node >= (uint32_t)naui_list_len(state->index.nodes))
		return;

	const Uph_SampleBrowserNode info = state->index.nodes[node];
	const char *path = uph__sample_browser_node_path(&state->index, node);

	if (!(info.flags & UPH_SAMPLE_BROWSER_NODE_DIR))
	{
		if (uph_ui_menu_item(context_menu, NAUI_TR("sample_browser.add_to_project"), leaf_id("uph_sample_browser_menu_add")))
			uph_sample_browser_add_to_project(naui_path_from_cstr(path), NULL, NULL);

		if (uph_ui_menu_item(context_menu, NAUI_TR("sample_browser.preview"), leaf_id("uph_sample_browser_menu_preview")))
			uph__sample_browser_preview(path);
	}

	if (info.flags & UPH_SAMPLE_BROWSER_NODE_ROOT)
	{
		if (uph_ui_menu_item(context_menu, NAUI_TR("sample_browser.remove_folder"), leaf_id("uph_sample_browser_menu_remove")))
			uph__sample_browser_remove_folder(path);
	}

	if (uph_ui_menu_item(context_menu, NAUI_TR("sample_browser.rescan"), leaf_id("uph_sample_browser_menu_rescan")))
		uph__sample_browser_start_scan();
}

static void uph_sample_browser_on_update(void)
{
	Uph_SampleBrowserState *state = &uph__sample_browser_state;

	if (!naui_strings_equal(state->filter, state->applied_filter, true))
	{
		state->applied_filter = state->filter;
		state->rows_dirty = true;
		state->scroll = 0.0f;
	}

	if (state->rows_dirty)
		uph__sample_browser_rebuild_rows();

	uph__sample_browser_keyboard();

	if (state->rows_dirty)
		uph__sample_browser_rebuild_rows();

	Uph_UIMenuID context_menu = uph_ui_context_menu(leaf_id("uph_sample_browser_context_menu"));

	uph__sample_browser_toolbar();
	uph__sample_browser_preview_strip();

	if (naui_list_len(uph_state.settings.browser.folders) == 0)
	{
		uph__sample_browser_empty_state();
		uph__sample_browser_context_menu_items(context_menu);
		return;
	}

	const bool searching = uph__sample_browser_searching();
	const uint32_t row_count = (uint32_t)naui_list_len(state->rows);
	const Leaf_ID scroll_id = leaf_id("uph_sample_browser_scrollbar");
	const int32_t row_height = NAUI_DPI(UPH_SAMPLE_BROWSER_ROW_HEIGHT);

	const Leaf_BoundingBox view_box = leaf_get_bounding_box((Leaf_ID){ .value = scroll_id.value + 2 });
	const float view_height = view_box.height > 0.0f ? view_box.height : row_height * 40.0f;

	if (state->scroll_to_selected)
	{
		const int32_t row = uph__sample_browser_row_of(state->selected);
		if (row >= 0)
		{
			const float top = (float)row * row_height;
			if (top < state->scroll)
				state->scroll = top;
			else if (top + row_height > state->scroll + view_height)
				state->scroll = top + row_height - view_height;
		}
		state->scroll_to_selected = false;
	}

	Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(UPH_UI_SCROLL_DIRECTION_VERTICAL, &state->scroll, scroll_id);
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(4), 0)
	})
	{
		uint32_t first = (uint32_t)(state->scroll / row_height);
		uint32_t last = first + (uint32_t)(view_height / row_height) + 2;
		if (first > row_count)
			first = row_count;
		if (last > row_count)
			last = row_count;

		if (first > 0)
			leaf({ .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED((float)first * row_height) } });

		for (uint32_t i = first; i < last; i++)
			uph__sample_browser_row(state->rows[i], context_menu, searching, row_height);

		if (last < row_count)
			leaf({ .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED((float)(row_count - last) * row_height) } });

		if (row_count == 0 && !state->scan)
		{
			leaf({
				.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
				.padding = LEAF_PADDING_ALL(NAUI_DPI(8)),
				.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
			})
			{
				leaf_text(NAUI_TR(searching ? "sample_browser.no_matches" : "sample_browser.no_samples"), {
					.color = uph_state.colors.ui_text_disabled,
					.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
				});
			}
		}
	}
	uph_ui_end_scroll_container(&scroll_container);

	uph__sample_browser_status();
	uph__sample_browser_context_menu_items(context_menu);
}

#pragma endregion
