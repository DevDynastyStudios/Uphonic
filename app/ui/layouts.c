#define UPH_SAVE_LAYOUT_DIALOG_KEY "_uph_save_layout"
#define UPH_LAYOUT_NAME_MAX 64
#define UPH_LAYOUT_MAX_COUNT 64

typedef struct
{
	char names[UPH_LAYOUT_MAX_COUNT][UPH_LAYOUT_NAME_MAX];
	uint32_t count;
	char pending_delete[UPH_LAYOUT_NAME_MAX];
	bool menu_was_open;
}
Uph_LayoutsState;
static Uph_LayoutsState uph_layouts_state = { 0 };

static Naui_Path uph_layouts_path(const char *name)
{
	char file_name[UPH_LAYOUT_NAME_MAX + sizeof(UPH_LAYOUT_EXTENSION)];
	snprintf(file_name, sizeof(file_name), "%s%s", name, UPH_LAYOUT_EXTENSION);
	return naui_path_join(UPHONIC_LAYOUTS_FOLDER, NAUI_PATH(file_name));
}

static void uph_layouts_select(const char *name)
{
	uph_state.settings.general.layout = naui_string_from_cstr(name);
}

static void uph_layouts_select_default(void)
{
	uph_layouts_select("");
	uph_layouts_apply_default();
}

static void uph_layouts_refresh(void)
{
	Uph_LayoutsState *state = &uph_layouts_state;
	state->count = 0;

	Naui_List(Naui_DirEntry) entries = naui_directory_filter(UPHONIC_LAYOUTS_FOLDER, NULL, NAUI_EXTENSIONS(UPH_LAYOUT_EXTENSION));
	for (size_t i = 0; i < (size_t)naui_list_len(entries) && state->count < UPH_LAYOUT_MAX_COUNT; i++)
	{
		if (entries[i].is_directory)
			continue;

		const Naui_StringView stem = naui_file_stem(&entries[i].path);
		if (stem.length == 0 || stem.length >= UPH_LAYOUT_NAME_MAX)
			continue;

		memcpy(state->names[state->count], stem.data, stem.length);
		state->names[state->count][stem.length] = '\0';
		state->count++;
	}

	naui_directory_filter_free(entries);
}

void uph_layouts_apply_default(void)
{
	const char uph_default_layout[] =
		"{"
			"\"version\": 1,"
			"\"viewport\": {"
				"\"kind\": \"split\", \"axis\": \"vertical\", \"ratio\": 0.55,"
				"\"child0\": {"
					"\"kind\": \"split\", \"axis\": \"horizontal\", \"ratio\": 0.8,"
					"\"child0\": { \"kind\": \"panel\", \"type\": \"uph_song_timeline\" },"
					"\"child1\": { \"kind\": \"panel\", \"type\": \"uph_resource_list\" }"
				"},"
				"\"child1\": {"
					"\"kind\": \"split\", \"axis\": \"horizontal\", \"ratio\": 0.65,"
					"\"child0\": { \"kind\": \"panel\", \"type\": \"uph_resource_editor\" },"
					"\"child1\": { \"kind\": \"panel\", \"type\": \"uph_mixer\" }"
				"}"
			"},"
			"\"floating\": []"
		"}";
	naui_panel_layout_load_from_memory(uph_default_layout, sizeof(uph_default_layout) - 1);
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
	if (name[0] && naui_path_exists(uph_layouts_path(name)) && uph_layouts_load(name))
		return;

	uph_layouts_select_default();
}

static void _uph_save_layout_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	(void)user_data;
	if (is_cancelled || result->count == 0)
		return;

	const Naui_Path saved_path = result->paths[0];
	if (!naui_panel_layout_save(saved_path.data))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to save layout: %s", saved_path.data);
		return;
	}

	uph_layouts_refresh();

	char name[UPH_LAYOUT_NAME_MAX];
	const Naui_StringView stem = naui_file_stem(&saved_path);
	if (stem.length == 0 || stem.length >= sizeof(name))
		return;

	memcpy(name, stem.data, stem.length);
	name[stem.length] = '\0';

	const Naui_Path expected = naui_path_normalize(uph_layouts_path(name));
	const Naui_Path actual = naui_path_normalize(saved_path);
	if (!strcmp(expected.data, actual.data))
		uph_layouts_select(name);
}

static void _uph_open_save_layout_dialog(void)
{
	if (naui_dialog_find(UPH_SAVE_LAYOUT_DIALOG_KEY))
		return;

	const Naui_Path folder = UPHONIC_LAYOUTS_FOLDER;
	if (!naui_path_exists(folder) && !naui_directories_create(folder))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to create the layouts folder: %s", folder.data);
		return;
	}

	naui_dialog_default_path_set(folder);
	if (!naui_dialog_save_file(UPH_SAVE_LAYOUT_DIALOG_KEY, NAUI_EXTENSIONS(UPH_LAYOUT_EXTENSION), _uph_save_layout_picked, NULL))
		return;

	naui_dialog_title_set(UPH_SAVE_LAYOUT_DIALOG_KEY, NAUI_TR("menu.layout.popup.save"));
	naui_dialog_confirm_label_set(UPH_SAVE_LAYOUT_DIALOG_KEY, NAUI_TR("menu.layout.popup.save_btn"));
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
	if (uph_ui_menu_item(layout_menu, NAUI_TR("menu.layout.default"), leaf_id("uph_layout_menu_default")))
		uph_layouts_select_default();

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
