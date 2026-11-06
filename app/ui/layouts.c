#define UPH_LAYOUT_NAME_MAX 192
#define UPH_LAYOUT_MAX_COUNT 64

typedef struct
{
	char builtin_names[UPH_LAYOUT_MAX_COUNT][UPH_LAYOUT_NAME_MAX];
	uint32_t builtin_count;
	char names[UPH_LAYOUT_MAX_COUNT][UPH_LAYOUT_NAME_MAX];
	uint32_t count;
	char pending_delete[UPH_LAYOUT_NAME_MAX];
	bool menu_was_open;
}
Uph_LayoutsState;
static Uph_LayoutsState uph_layouts_state = { 0 };

static Naui_Path uph_layouts_path_in(const Naui_Path folder, const char *name)
{
	char file_name[UPH_LAYOUT_NAME_MAX + sizeof(UPH_LAYOUT_EXTENSION)];
	snprintf(file_name, sizeof(file_name), "%s%s", name, UPH_LAYOUT_EXTENSION);
	return naui_path_join(folder, NAUI_PATH(file_name));
}

static Naui_Path uph_layouts_path(const char *name)
{
	return uph_layouts_path_in(UPHONIC_LAYOUTS_FOLDER, name);
}

static Naui_Path uph_layouts_builtin_path(const char *name)
{
	return uph_layouts_path_in(UPHONIC_BUILTIN_LAYOUTS_FOLDER, name);
}

static bool uph_layouts_is_builtin_selection(const char *selection)
{
	return !strncmp(selection, UPH_LAYOUT_BUILTIN_PREFIX, sizeof(UPH_LAYOUT_BUILTIN_PREFIX) - 1);
}

static void uph_layouts_select(const char *name)
{
	uph_state.settings.general.layout = naui_string_from_cstr(name);
}

static void uph_layouts_scan(const Naui_Path folder, char names[UPH_LAYOUT_MAX_COUNT][UPH_LAYOUT_NAME_MAX], uint32_t *count)
{
	*count = 0;

	Naui_List(Naui_DirEntry) entries = naui_directory_filter(folder, NULL, NAUI_EXTENSIONS(UPH_LAYOUT_EXTENSION));
	for (size_t i = 0; i < (size_t)naui_list_len(entries) && *count < UPH_LAYOUT_MAX_COUNT; i++)
	{
		if (entries[i].is_directory)
			continue;

		const Naui_StringView stem = naui_file_stem(&entries[i].path);
		if (stem.length == 0 || stem.length >= UPH_LAYOUT_NAME_MAX || !naui_utf8_is_valid(stem.data, stem.length))
			continue;

		memcpy(names[*count], stem.data, stem.length);
		names[*count][stem.length] = '\0';
		(*count)++;
	}

	naui_directory_filter_free(entries);
}

static void uph_layouts_refresh(void)
{
	Uph_LayoutsState *state = &uph_layouts_state;
	uph_layouts_scan(UPHONIC_BUILTIN_LAYOUTS_FOLDER, state->builtin_names, &state->builtin_count);
	uph_layouts_scan(UPHONIC_LAYOUTS_FOLDER, state->names, &state->count);

	for (uint32_t i = 1; i < state->builtin_count; i++)
	{
		if (strcmp(state->builtin_names[i], UPH_LAYOUT_BUILTIN_DEFAULT))
			continue;

		char name[UPH_LAYOUT_NAME_MAX];
		memcpy(name, state->builtin_names[i], sizeof(name));
		memmove(state->builtin_names[1], state->builtin_names[0], (size_t)i * sizeof(state->builtin_names[0]));
		memcpy(state->builtin_names[0], name, sizeof(name));
		break;
	}
}

static bool uph_layouts_load_builtin(const char *name)
{
	const Naui_Path path = uph_layouts_builtin_path(name);
	if (!naui_panel_layout_load(path.data))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to load built-in layout: %s", path.data);
		return false;
	}

	char selection[UPH_LAYOUT_NAME_MAX + sizeof(UPH_LAYOUT_BUILTIN_PREFIX)];
	snprintf(selection, sizeof(selection), "%s%s", UPH_LAYOUT_BUILTIN_PREFIX, name);
	uph_layouts_select(selection);
	return true;
}

void uph_layouts_apply_default(void)
{
	uph_layouts_load_builtin(UPH_LAYOUT_BUILTIN_DEFAULT);
}

bool uph_layouts_load(const char *name)
{
	const Naui_Path path = uph_layouts_path(name);
	if (naui_panel_layout_load(path.data))
	{
		uph_layouts_select(name);
		return true;
	}

	naui_log(NAUI_LOG_ERROR, "Failed to load layout: %s", path.data);
	return false;
}

void uph_layouts_apply_saved(void)
{
	const char *name = uph_state.settings.general.layout.data;
	if (uph_layouts_is_builtin_selection(name))
	{
		const char *builtin = name + sizeof(UPH_LAYOUT_BUILTIN_PREFIX) - 1;
		if (naui_path_exists(uph_layouts_builtin_path(builtin)) && uph_layouts_load_builtin(builtin))
			return;
	}
	else if (name[0] && naui_path_exists(uph_layouts_path(name)) && uph_layouts_load(name))
		return;

	uph_layouts_apply_default();
}

static Naui_String uph_layouts_save_name;

static void _uph_open_save_layout_dialog_ex(const char *message);

static bool _uph_layout_name_valid(const char *name)
{
	const size_t length = strlen(name);
	if (length == 0 || length >= UPH_LAYOUT_NAME_MAX || !strcmp(name, ".") || !strcmp(name, ".."))
		return false;

	if (!naui_utf8_is_valid(name, length))
		return false;

	for (size_t i = 0; i < length; i++)
	{
		const unsigned char c = (unsigned char)name[i];
		if (c < 0x20 || strchr("<>:\"/\\|?*", c))
			return false;
	}

	return name[length - 1] != '.';
}

static void _uph_layout_name_trim(char *name)
{
	size_t start = 0;
	while (name[start] && isspace((unsigned char)name[start]))
		start++;

	size_t length = strlen(name + start);
	memmove(name, name + start, length + 1);
	while (length > 0 && isspace((unsigned char)name[length - 1]))
		name[--length] = '\0';
}

static void _uph_save_layout_write(const char *name)
{
	const Naui_Path folder = UPHONIC_LAYOUTS_FOLDER;
	if (!naui_path_exists(folder) && !naui_directories_create(folder))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to create the layouts folder: %s", folder.data);
		return;
	}

	const Naui_Path path = uph_layouts_path(name);
	if (!naui_panel_layout_save(path.data))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to save layout: %s", path.data);
		return;
	}

	uph_layouts_refresh();
	uph_layouts_select(name);
}

static void _uph_overwrite_layout_result(int32_t button, void *user_data)
{
	(void)user_data;
	if (button == 0)
		_uph_save_layout_write(uph_layouts_save_name.data);
	else
		_uph_open_save_layout_dialog_ex(NULL);
}

static void _uph_save_layout_result(int32_t button, void *user_data)
{
	(void)user_data;
	if (button != 0)
		return;

	char name[NAUI_STRING_MAX_SIZE];
	snprintf(name, sizeof(name), "%s", uph_layouts_save_name.data);
	_uph_layout_name_trim(name);
	name[naui_utf8_clip_length(name, strlen(name), UPH_LAYOUT_NAME_MAX - 1)] = '\0';
	_uph_layout_name_trim(name);
	uph_layouts_save_name = naui_string_from_cstr(name);

	if (!_uph_layout_name_valid(name))
	{
		_uph_open_save_layout_dialog_ex(NAUI_TR("file_dialog.error.invalid_name"));
		return;
	}

	if (naui_path_exists(uph_layouts_path(name)))
	{
		uph_dialog_open(&(Uph_DialogConfig){
			.title = NAUI_TR("menu.layout.popup.override"),
			.message = uph_layouts_save_name.data,
			.buttons = { NAUI_TR("menu.layout.popup.overwrite"), NAUI_TR("menu.layout.popup.cancel") },
			.default_button = 0,
			.cancel_button = 1,
			.callback = _uph_overwrite_layout_result
		});
		return;
	}

	_uph_save_layout_write(name);
}

static void _uph_open_save_layout_dialog_ex(const char *message)
{
	uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("menu.layout.popup.save"),
		.message = message,
		.buttons = { NAUI_TR("menu.layout.popup.save_btn"), NAUI_TR("menu.layout.popup.cancel") },
		.default_button = 0,
		.cancel_button = 1,
		.callback = _uph_save_layout_result,
		.input = &uph_layouts_save_name,
		.input_placeholder = NAUI_TR("file_dialog.name")
	});
}

static void _uph_open_save_layout_dialog(void)
{
	if (uph_dialog_is_open())
		return;

	const char *current = uph_state.settings.general.layout.data;
	uph_layouts_save_name = naui_string_from_cstr(uph_layouts_is_builtin_selection(current) ? "" : current);
	_uph_open_save_layout_dialog_ex(NULL);
}

static void _uph_delete_layout_result(int32_t button, void *user_data)
{
	(void)user_data;
	if (button != 0)
		return;

	const Naui_Path path = uph_layouts_path(uph_layouts_state.pending_delete);
	if (!naui_file_delete(path))
		naui_log(NAUI_LOG_ERROR, "Failed to delete layout: %s", path.data);
	else if (!strcmp(uph_state.settings.general.layout.data, uph_layouts_state.pending_delete))
		uph_layouts_select("");

	uph_layouts_refresh();
}

static void _uph_confirm_delete_layout(const char *name)
{
	snprintf(uph_layouts_state.pending_delete, sizeof(uph_layouts_state.pending_delete), "%s", name);

	uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("menu.layout.delete"),
		.message = uph_layouts_state.pending_delete,
		.buttons = { NAUI_TR("menu.layout.delete"), NAUI_TR("menu.layout.popup.cancel") },
		.default_button = 1,
		.cancel_button = 1,
		.callback = _uph_delete_layout_result
	});
}

void uph_layouts_render_menu(void)
{
	Uph_LayoutsState *state = &uph_layouts_state;

	const bool menu_open = uph_ui_any_menu_open();
	if (menu_open && !state->menu_was_open)
		uph_layouts_refresh();
	state->menu_was_open = menu_open;

	Uph_UIMenuID layout_menu = uph_ui_menu(NAUI_TR("menu.layout"), leaf_id("uph_layout_menu"));
	for (uint32_t i = 0; i < state->builtin_count; i++)
	{
		if (uph_ui_menu_item(layout_menu, NAUI_TR(state->builtin_names[i]), leaf_id_indexed("uph_layout_menu_builtin_item", i)))
			uph_layouts_load_builtin(state->builtin_names[i]);
	}

	for (uint32_t i = 0; i < state->count; i++)
	{
		if (uph_ui_menu_item(layout_menu, state->names[i], leaf_id_indexed("uph_layout_menu_item", i)))
			uph_layouts_load(state->names[i]);
	}

	if (uph_ui_menu_item(layout_menu, NAUI_TR("menu.layout.save"), leaf_id("uph_layout_menu_save")))
		_uph_open_save_layout_dialog();

	if (state->count == 0)
		return;

	Uph_UIMenuID delete_menu = uph_ui_submenu(layout_menu, NAUI_TR("menu.layout.delete"), leaf_id("uph_layout_menu_delete"));
	for (uint32_t i = 0; i < state->count; i++)
	{
		if (uph_ui_menu_item(delete_menu, state->names[i], leaf_id_indexed("uph_layout_menu_delete_item", i)))
			_uph_confirm_delete_layout(state->names[i]);
	}
}
