#define NAUI_MAX_DIALOGS 4
#define NAUI_DIALOG_MAX_REMEMBERED 16
#define NAUI_DIALOG_MODE_COUNT 3

static Naui_Dialog s_dialogs[NAUI_MAX_DIALOGS];
static const Naui_DialogRenderer *s_dialog_renderer;

typedef struct
{
	char key[NAUI_DIALOG_KEY_MAX];
	Naui_Path directory;
}
Naui_DialogRememberedDir;

static Naui_DialogRememberedDir s_remembered_by_key[NAUI_DIALOG_MAX_REMEMBERED];
static uint32_t s_remembered_by_key_count;
static uint32_t s_remembered_by_key_next;
static Naui_Path s_remembered_by_mode[NAUI_DIALOG_MODE_COUNT];
static Naui_Path s_default_path;

#pragma region Helpers

static void _naui_dialog_copy_text(char *dest, size_t dest_size, const char *source)
{
	snprintf(dest, dest_size, "%s", source ? source : "");
}

static bool _naui_dialog_is_separator(char c)
{
	return c == '/' || c == '\\';
}

static bool _naui_dialog_is_folder(const Naui_Path path)
{
	return path.data[0] && naui_path_exists(path) && naui_path_is_directory(path);
}

static int _naui_dialog_compare_names(const char *a, const char *b)
{
	for (;; a++, b++)
	{
		const int ca = tolower((unsigned char)*a);
		const int cb = tolower((unsigned char)*b);
		if (ca != cb)
			return ca - cb;

		if (ca == 0)
			return 0;
	}
}

static const char *_naui_dialog_entry_name(const Naui_DirEntry *entry)
{
	return naui_file_filename(&entry->path).data;
}

static int _naui_dialog_compare_entries(const void *left, const void *right)
{
	const Naui_DirEntry *a = (const Naui_DirEntry*)left;
	const Naui_DirEntry *b = (const Naui_DirEntry*)right;
	if (a->is_directory != b->is_directory)
		return a->is_directory ? -1 : 1;

	return _naui_dialog_compare_names(_naui_dialog_entry_name(a), _naui_dialog_entry_name(b));
}

static const char *_naui_dialog_extension(const char *file_name)
{
	const char *dot = strrchr(file_name, '.');
	return (dot && dot != file_name) ? dot : "";
}

static bool _naui_dialog_valid_file_name(const char *name)
{
	if (!name[0] || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return false;

	for (const char *c = name; *c; c++)
	{
		if ((unsigned char)*c < 0x20 || strchr("/\\:*?\"<>|", *c))
			return false;
	}

	const char last = name[strlen(name) - 1];
	return last != ' ' && last != '.';
}

static void _naui_dialog_set_text(Naui_String *string, const char *text)
{
	*string = naui_string_from_cstr(text ? text : "");
}

static const char *_naui_dialog_trimmed_name(const Naui_Dialog *dialog, char *out, size_t out_size)
{
	const char *start = dialog->name.data;
	while (*start == ' ')
		start++;

	_naui_dialog_copy_text(out, out_size, start);
	size_t length = strlen(out);
	while (length > 0 && out[length - 1] == ' ')
		out[--length] = '\0';

	return out;
}

#pragma endregion

#pragma region Remembered Folders

static void _naui_dialog_remember(const char *key, Naui_DialogMode mode, const Naui_Path directory)
{
	if ((int)mode >= 0 && mode < NAUI_DIALOG_MODE_COUNT)
		s_remembered_by_mode[mode] = directory;

	for (uint32_t i = 0; i < s_remembered_by_key_count; i++)
	{
		if (strcmp(s_remembered_by_key[i].key, key) == 0)
		{
			s_remembered_by_key[i].directory = directory;
			return;
		}
	}

	uint32_t slot;
	if (s_remembered_by_key_count < NAUI_DIALOG_MAX_REMEMBERED)
		slot = s_remembered_by_key_count++;
	else
	{
		slot = s_remembered_by_key_next;
		s_remembered_by_key_next = (s_remembered_by_key_next + 1) % NAUI_DIALOG_MAX_REMEMBERED;
	}

	_naui_dialog_copy_text(s_remembered_by_key[slot].key, NAUI_DIALOG_KEY_MAX, key);
	s_remembered_by_key[slot].directory = directory;
}

static Naui_Path _naui_dialog_pick_start_dir(const char *key, Naui_DialogMode mode)
{
	if (s_default_path.data[0])
	{
		const Naui_Path path = s_default_path;
		s_default_path = (Naui_Path){0};
		if (_naui_dialog_is_folder(path))
			return path;
	}

	for (uint32_t i = 0; i < s_remembered_by_key_count; i++)
	{
		if (strcmp(s_remembered_by_key[i].key, key) == 0 && _naui_dialog_is_folder(s_remembered_by_key[i].directory))
			return s_remembered_by_key[i].directory;
	}

	if ((int)mode >= 0 && mode < NAUI_DIALOG_MODE_COUNT && _naui_dialog_is_folder(s_remembered_by_mode[mode]))
		return s_remembered_by_mode[mode];

	return naui_directory_get(NAUI_DIR_WORKING);
}

#pragma endregion

#pragma region Listing

bool naui_dialog_matches_filter(const Naui_Dialog *dialog, const char *file_name)
{
	if (dialog->filter_count == 0)
		return true;

	const char *extension = _naui_dialog_extension(file_name);
	for (uint32_t i = 0; i < dialog->filter_count; i++)
	{
		if (_naui_dialog_compare_names(extension, dialog->filters[i]) == 0)
			return true;
	}

	return false;
}

void naui_dialog_refresh(Naui_Dialog *dialog)
{
	Naui_Path previous_selection = {0};
	if (dialog->selected_index >= 0 && dialog->selected_index < (int32_t)naui_list_len(dialog->entries))
		previous_selection = dialog->entries[dialog->selected_index].path;

	naui_list_clear(dialog->entries);
	dialog->selected_index = -1;

	Naui_DirIterator iterator = naui_dir_iterator_open(dialog->current_dir, NULL, NULL, false);
	for (; naui_dir_iterator_valid(&iterator); naui_dir_iterator_next(&iterator))
	{
		const Naui_DirEntry entry = iterator.entry;
		if (!dialog->show_hidden && naui_file_is_hidden(&entry.path))
			continue;

		if (!entry.is_directory)
		{
			if (dialog->mode == NAUI_DIALOG_OPEN_FOLDER)
				continue;

			if (!naui_dialog_matches_filter(dialog, _naui_dialog_entry_name(&entry)))
				continue;
		}

		naui_list_push(dialog->entries, entry);
	}
	naui_dir_iterator_close(&iterator);

	const size_t count = naui_list_len(dialog->entries);
	if (count > 1)
		qsort(dialog->entries, count, sizeof(Naui_DirEntry), _naui_dialog_compare_entries);

	if (previous_selection.data[0])
	{
		for (size_t i = 0; i < count; i++)
		{
			if (strcmp(dialog->entries[i].path.data, previous_selection.data) == 0)
			{
				dialog->selected_index = (int32_t)i;
				break;
			}
		}
	}
}

void naui_dialog_set_show_hidden(Naui_Dialog *dialog, bool show_hidden)
{
	dialog->show_hidden = show_hidden;
	naui_dialog_refresh(dialog);
}

#pragma endregion

#pragma region Navigate & Select

bool naui_dialog_navigate(Naui_Dialog *dialog, const Naui_Path folder)
{
	if (!_naui_dialog_is_folder(folder))
		return false;

	dialog->current_dir = naui_path_normalize(folder);
	_naui_dialog_set_text(&dialog->path_text, dialog->current_dir.data);
	dialog->selected_index = -1;

	if (dialog->mode != NAUI_DIALOG_SAVE_FILE)
		_naui_dialog_set_text(&dialog->name, "");

	naui_dialog_refresh(dialog);
	return true;
}

bool naui_dialog_navigate_up(Naui_Dialog *dialog)
{
	const Naui_Path parent = naui_path_parent(dialog->current_dir);
	if (!parent.data[0] || strcmp(parent.data, dialog->current_dir.data) == 0)
		return false;

	return naui_dialog_navigate(dialog, parent);
}

void naui_dialog_select(Naui_Dialog *dialog, int32_t entry_index)
{
	if (entry_index < 0 || entry_index >= (int32_t)naui_list_len(dialog->entries))
	{
		dialog->selected_index = -1;
		return;
	}

	dialog->selected_index = entry_index;
	const Naui_DirEntry *entry = &dialog->entries[entry_index];
	if (!entry->is_directory || dialog->mode == NAUI_DIALOG_OPEN_FOLDER)
		_naui_dialog_set_text(&dialog->name, _naui_dialog_entry_name(entry));
}

bool naui_dialog_commit_path_text(Naui_Dialog *dialog)
{
	char text[NAUI_STRING_MAX_SIZE];
	_naui_dialog_copy_text(text, sizeof(text), dialog->path_text.data);

	size_t length = strlen(text);
	while (length > 1 && _naui_dialog_is_separator(text[length - 1]))
	{
		text[--length] = '\0';
	}

	const Naui_Path typed = naui_path_normalize(naui_path_from_cstr(text));
	if (length > 0 && naui_path_exists(typed))
	{
		if (naui_path_is_directory(typed))
			return naui_dialog_navigate(dialog, typed);

		if (dialog->mode != NAUI_DIALOG_OPEN_FOLDER && naui_dialog_navigate(dialog, naui_path_parent(typed)))
		{
			for (size_t i = 0; i < naui_list_len(dialog->entries); i++)
			{
				if (strcmp(dialog->entries[i].path.data, typed.data) == 0)
				{
					naui_dialog_select(dialog, (int32_t)i);
					break;
				}
			}

			return true;
		}
	}

	_naui_dialog_set_text(&dialog->path_text, dialog->current_dir.data);
	return false;
}

#pragma endregion

#pragma region Finishing

static void _naui_dialog_finish(Naui_Dialog* dialog, const Naui_Path* picked, bool cancelled)
{
	Naui_DialogResult result = {
		.paths = picked,
		.count = picked ? 1u : 0u,
		.directory = dialog->current_dir
	};
	const Naui_DialogCallback callback = dialog->callback;
	void *user_data = dialog->user_data;

	if (!cancelled)
		_naui_dialog_remember(dialog->key, dialog->mode, dialog->current_dir);

	dialog->state &= ~NAUI_DIALOG_STATE_OPEN;
	if (s_dialog_renderer && s_dialog_renderer->end)
		s_dialog_renderer->end(dialog);

	naui_list_free(dialog->entries);
	memset(dialog, 0, sizeof(*dialog));

	if (callback)
		callback(&result, user_data, cancelled);
}

void naui_dialog_cancel(Naui_Dialog* dialog)
{
	_naui_dialog_finish(dialog, NULL, true);
}

Naui_DialogConfirmResult naui_dialog_confirm(Naui_Dialog *dialog, bool allow_overwrite)
{
	char name[NAUI_STRING_MAX_SIZE];
	_naui_dialog_trimmed_name(dialog, name, sizeof(name));

	if (dialog->mode == NAUI_DIALOG_OPEN_FOLDER)
	{
		Naui_Path target = dialog->current_dir;
		if (dialog->selected_index >= 0 && dialog->selected_index < (int32_t)naui_list_len(dialog->entries))
			target = dialog->entries[dialog->selected_index].path;

		_naui_dialog_finish(dialog, &target, false);
		return NAUI_DIALOG_CONFIRM_DONE;
	}

	if (dialog->mode == NAUI_DIALOG_OPEN_FILE)
	{
		if (!name[0])
		{
			if (dialog->selected_index >= 0 && dialog->selected_index < (int32_t)naui_list_len(dialog->entries) &&
				dialog->entries[dialog->selected_index].is_directory)
			{
				naui_dialog_navigate(dialog, dialog->entries[dialog->selected_index].path);
				return NAUI_DIALOG_CONFIRM_NAVIGATED;
			}
			return NAUI_DIALOG_CONFIRM_NOTHING_SELECTED;
		}

		if (!_naui_dialog_valid_file_name(name))
			return NAUI_DIALOG_CONFIRM_INVALID_NAME;

		const Naui_Path target = naui_path_normalize(naui_path_join(dialog->current_dir, naui_path_from_cstr(name)));
		if (!naui_path_exists(target))
			return NAUI_DIALOG_CONFIRM_NOT_FOUND;

		if (naui_path_is_directory(target))
		{
			naui_dialog_navigate(dialog, target);
			return NAUI_DIALOG_CONFIRM_NAVIGATED;
		}

		_naui_dialog_finish(dialog, &target, false);
		return NAUI_DIALOG_CONFIRM_DONE;
	}

	if (!name[0])
		return NAUI_DIALOG_CONFIRM_NOTHING_SELECTED;

	if (!_naui_dialog_valid_file_name(name))
		return NAUI_DIALOG_CONFIRM_INVALID_NAME;

	if (dialog->filter_count > 0 && !naui_dialog_matches_filter(dialog, name))
	{
		const size_t length = strlen(name);
		if (length + strlen(dialog->filters[0]) >= sizeof(name))
			return NAUI_DIALOG_CONFIRM_INVALID_NAME;

		memcpy(name + length, dialog->filters[0], strlen(dialog->filters[0]) + 1);
	}

	const Naui_Path target = naui_path_normalize(naui_path_join(dialog->current_dir, naui_path_from_cstr(name)));
	if (naui_path_exists(target))
	{
		if (naui_path_is_directory(target))
			return NAUI_DIALOG_CONFIRM_INVALID_NAME;

		if (!allow_overwrite)
			return NAUI_DIALOG_CONFIRM_NEEDS_OVERWRITE;
	}

	_naui_dialog_finish(dialog, &target, false);
	return NAUI_DIALOG_CONFIRM_DONE;
}

Naui_DialogConfirmResult naui_dialog_activate(Naui_Dialog *dialog, int32_t entry_index)
{
	if (entry_index < 0 || entry_index >= (int32_t)naui_list_len(dialog->entries))
		return NAUI_DIALOG_CONFIRM_NOTHING_SELECTED;

	const Naui_DirEntry entry = dialog->entries[entry_index];
	if (entry.is_directory)
	{
		naui_dialog_navigate(dialog, entry.path);
		return NAUI_DIALOG_CONFIRM_NAVIGATED;
	}

	naui_dialog_select(dialog, entry_index);
	return naui_dialog_confirm(dialog, false);
}

#pragma endregion

#pragma region Opening

static Naui_Dialog *_naui_dialog_open(Naui_DialogMode mode, const char *key, const char **filters, Naui_DialogCallback callback, void *user_data, const char *title, const char *confirm_label)
{
	if (!key || !key[0] || !s_dialog_renderer || !s_dialog_renderer->begin)
	{
		naui_log(NAUI_LOG_ERROR, "Can't open dialog: no key or no dialog renderer registered");
		return NULL;
	}

	if (naui_dialog_find(key))
		return NULL;

	Naui_Dialog *dialog = NULL;
	for (uint32_t i = 0; i < NAUI_MAX_DIALOGS; i++)
	{
		if (!s_dialogs[i]._in_use)
		{
			dialog = &s_dialogs[i];
			break;
		}
	}

	if (!dialog)
	{
		naui_log(NAUI_LOG_WARNING, "Can't open dialog \"%s\": too many dialogs are open", key);
		return NULL;
	}

	memset(dialog, 0, sizeof(*dialog));
	dialog->_in_use = true;
	dialog->mode = mode;
	dialog->state = NAUI_DIALOG_STATE_OPEN;
	dialog->selected_index = -1;
	dialog->callback = callback;
	dialog->user_data = user_data;
	_naui_dialog_copy_text(dialog->key, sizeof(dialog->key), key);
	_naui_dialog_copy_text(dialog->title, sizeof(dialog->title), title);
	_naui_dialog_copy_text(dialog->confirm_label, sizeof(dialog->confirm_label), confirm_label);

	for (uint32_t i = 0; filters && filters[i] && dialog->filter_count < NAUI_DIALOG_MAX_FILTERS; i++)
		_naui_dialog_copy_text(dialog->filters[dialog->filter_count++], NAUI_DIALOG_FILTER_MAX, filters[i]);

	naui_dialog_navigate(dialog, _naui_dialog_pick_start_dir(key, mode));
	if (!dialog->current_dir.data[0])
	{
		naui_list_free(dialog->entries);
		memset(dialog, 0, sizeof(*dialog));
		return NULL;
	}

	if (!s_dialog_renderer->begin(dialog))
	{
		naui_log(NAUI_LOG_ERROR, "Dialog renderer couldn't show \"%s\"", key);
		naui_list_free(dialog->entries);
		memset(dialog, 0, sizeof(*dialog));
		return NULL;
	}

	return dialog;
}

bool naui_dialog_open_file(const char *key, const char **filters, Naui_DialogCallback callback, void *user_data)
{
	return _naui_dialog_open(NAUI_DIALOG_OPEN_FILE, key, filters, callback, user_data, "Open File", "Open") != NULL;
}

bool naui_dialog_save_file(const char *key, const char **filters, Naui_DialogCallback callback, void *user_data)
{
	return _naui_dialog_open(NAUI_DIALOG_SAVE_FILE, key, filters, callback, user_data, "Save File", "Save") != NULL;
}

bool naui_dialog_open_folder(const char *key, const char **filters, Naui_DialogCallback callback, void *user_data)
{
	return _naui_dialog_open(NAUI_DIALOG_OPEN_FOLDER, key, filters, callback, user_data, "Select Folder", "Select") != NULL;
}

Naui_Dialog *naui_dialog_find(const char *key)
{
	for (uint32_t i = 0; key && i < NAUI_MAX_DIALOGS; i++)
	{
		if (s_dialogs[i]._in_use && strcmp(s_dialogs[i].key, key) == 0)
			return &s_dialogs[i];
	}

	return NULL;
}

void naui_dialog_confirm_label_set(const char *key, const char *confirm_label)
{
	Naui_Dialog *dialog = naui_dialog_find(key);
	if (dialog)
		_naui_dialog_copy_text(dialog->confirm_label, sizeof(dialog->confirm_label), confirm_label);
}

void naui_dialog_title_set(const char *key, const char *title)
{
	Naui_Dialog *dialog = naui_dialog_find(key);
	if (dialog)
		_naui_dialog_copy_text(dialog->title, sizeof(dialog->title), title);
}

void naui_dialog_name_set(const char *key, const char *name)
{
	Naui_Dialog *dialog = naui_dialog_find(key);
	if (dialog)
		_naui_dialog_set_text(&dialog->name, name);
}

bool naui_dialog_default_path_set(const Naui_Path path)
{
	if (!_naui_dialog_is_folder(path))
		return false;

	s_default_path = path;
	return true;
}

void naui_dialog_set_renderer(const Naui_DialogRenderer *renderer)
{
	s_dialog_renderer = renderer;
}

#pragma endregion