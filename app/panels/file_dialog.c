#define UPH_FILE_DIALOG_WIDTH 760.0f
#define UPH_FILE_DIALOG_HEIGHT 480.0f

typedef struct
{
	Naui_Dialog *dialog;
	const char *error;
	float scroll;
	uint64_t instance;
}
Uph_FileDialogData;

NAUI_PANEL_WITH_DATA(uph_file_dialog, Uph_FileDialogData)

static Naui_Dialog *s_file_dialog_pending;
static Naui_Dialog *s_file_dialog_ending;
static bool s_file_dialog_recentering;
static char s_file_dialog_overwrite_key[NAUI_DIALOG_KEY_MAX];

typedef struct
{
	bool go_up;
	bool refresh;
	bool toggle_hidden;
	bool confirm;
	bool cancel;
	bool commit_path_text;
	bool has_navigate;
	Naui_Path navigate_to;
	int32_t clicked_row;
	int32_t activated_row;
}
Uph_FileDialogActions;

#pragma region Helpers

static void _uph_file_dialog_format_size(size_t size, char *out, size_t out_size)
{
	if (size < 1024)
		snprintf(out, out_size, "%u B", (unsigned)size);
	else if (size < 1024 * 1024)
		snprintf(out, out_size, "%.1f KB", (double)size / 1024.0);
	else if (size < 1024ull * 1024 * 1024)
		snprintf(out, out_size, "%.1f MB", (double)size / (1024.0 * 1024.0));
	else
		snprintf(out, out_size, "%.2f GB", (double)size / (1024.0 * 1024.0 * 1024.0));
}

static void _uph_file_dialog_filter_text(const Naui_Dialog *dialog, char *out, size_t out_size)
{
	out[0] = '\0';
	for (uint32_t i = 0; i < dialog->filter_count; i++)
	{
		const size_t used = strlen(out);
		snprintf(out + used, out_size - used, "%s%s", i ? ", " : "", dialog->filters[i]);
	}
}

static void _uph_file_dialog_overwrite_result(int32_t button, void *user_data)
{
	(void)user_data;
	if (button != 0)
		return;

	Naui_Dialog *dialog = naui_dialog_find(s_file_dialog_overwrite_key);
	if (dialog)
		naui_dialog_confirm(dialog, true);
}

static void _uph_file_dialog_handle_result(Uph_FileDialogData *data, Naui_DialogConfirmResult result)
{
	data->error = NULL;

	switch (result)
	{
		case NAUI_DIALOG_CONFIRM_DONE:
		case NAUI_DIALOG_CONFIRM_NAVIGATED:
			break;

		case NAUI_DIALOG_CONFIRM_NEEDS_OVERWRITE:
			snprintf(s_file_dialog_overwrite_key, sizeof(s_file_dialog_overwrite_key), "%s", data->dialog->key);
			uph_dialog_open(&(Uph_DialogConfig){
				.title = NAUI_TR("file_dialog.overwrite.title"),
				.message = NAUI_TR("file_dialog.overwrite.message"),
				.buttons = { NAUI_TR("file_dialog.overwrite.replace"), NAUI_TR("dialog.cancel") },
				.default_button = -1,
				.cancel_button = 1,
				.callback = _uph_file_dialog_overwrite_result
			});
			break;

		case NAUI_DIALOG_CONFIRM_NOTHING_SELECTED:
			data->error = NAUI_TR("file_dialog.error.nothing_selected");
			break;

		case NAUI_DIALOG_CONFIRM_INVALID_NAME:
			data->error = NAUI_TR("file_dialog.error.invalid_name");
			break;

		case NAUI_DIALOG_CONFIRM_NOT_FOUND:
			data->error = NAUI_TR("file_dialog.error.not_found");
			break;
	}
}

#pragma endregion

#pragma region UI

static void _uph_file_dialog_place(Uph_FileDialogData *data, Uph_FileDialogActions *actions, const char *label, const Naui_Path path, uint64_t index)
{
	if (!path.data[0] || !naui_path_is_directory(path))
		return;

	if (uph_ui_text_button(label, leaf_id_indexed("uph_file_dialog_place", data->instance + index)))
	{
		actions->navigate_to = path;
		actions->has_navigate = true;
	}
}

static void _uph_file_dialog_sidebar(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	leaf({
		.size = {LEAF_SIZE_FIXED(NAUI_DPI(130.0f)), LEAF_SIZE_GROW},
		.direction = LEAF_DIRECTION_VERTICAL,
		.child_gap = NAUI_DPI(4.0f)
	})
	{
		_uph_file_dialog_place(data, actions, NAUI_TR("file_dialog.places.home"), naui_directory_get(NAUI_DIR_HOME), 1);
		_uph_file_dialog_place(data, actions, NAUI_TR("file_dialog.places.downloads"), naui_directory_get(NAUI_DIR_DOWNLOADS), 2);
		_uph_file_dialog_place(data, actions, NAUI_TR("file_dialog.places.projects"), UPHONIC_WORKSPACE_FOLDER, 3);
	}
}

static void _uph_file_dialog_row(Uph_FileDialogData *data, Uph_FileDialogActions *actions, const Naui_DirEntry *entry, int32_t index)
{
	const Naui_Dialog *dialog = data->dialog;
	const Leaf_ID row_id = leaf_id_indexed("uph_file_dialog_row", data->instance * 65599u + (uint64_t)index);
	const bool hovered = uph_ui_widget_hovered(row_id);
	const bool selected = dialog->selected_index == index;

	if (hovered)
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			actions->clicked_row = index;

		if (naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
			actions->activated_row = index;
	}

	const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
	const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

	Leaf_Color background = LEAF_COLOR_TRANSPARENT;
	if (selected)
		background = naui_theme_color("naui_widget_accent_color");
	else if (hovered)
		background = naui_theme_color("uph_ui_frame_hovered_bg_color");

	char name[NAUI_PATH_MAX + 2];
	snprintf(name, sizeof(name), "%s%s", naui_file_filename(&entry->path).data, entry->is_directory ? "/" : "");

	leaf({
		.id = row_id,
		.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 0.5f)),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.color = {background},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL)
	})
	{
		leaf({
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
			.clip_children = true
		})
		{
			leaf_text(name, { .color = {text_color}, .font_size = {font_size} });
		}

		if (!entry->is_directory)
		{
			char size_text[32];
			_uph_file_dialog_format_size(entry->size, size_text, sizeof(size_text));
			leaf_text(size_text, { .color = {text_color}, .font_size = {font_size} });
		}
	}
}

static void _uph_file_dialog_list(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	const Naui_Dialog *dialog = data->dialog;

	leaf({
		.size = {LEAF_SIZE_GROW, LEAF_SIZE_GROW},
		.color = {naui_theme_color("uph_ui_frame_bg_color")},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
		.clip_children = true
	})
	{
		Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(
			UPH_UI_SCROLL_DIRECTION_VERTICAL,
			&data->scroll,
			leaf_id_indexed("uph_file_dialog_scroll", data->instance)
		);

		const uint32_t count = (uint32_t)naui_list_len(dialog->entries);
		for (uint32_t i = 0; i < count; i++)
			_uph_file_dialog_row(data, actions, &dialog->entries[i], (int32_t)i);

		if (count == 0)
		{
			leaf_text(NAUI_TR("file_dialog.empty"), {
				.color = {naui_theme_color("uph_ui_text_color")},
				.font_size = {NAUI_DPI(naui_theme_float("uph_ui_font_size"))}
			});
		}

		uph_ui_end_scroll_container(&scroll_container);
	}
}

static void _uph_file_dialog_top_bar(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	Naui_Dialog *dialog = data->dialog;
	const Leaf_ID path_id = leaf_id_indexed("uph_file_dialog_path", data->instance);

	leaf({
		.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(6.0f)
	})
	{
		if (uph_ui_text_button(NAUI_TR("file_dialog.up"), leaf_id_indexed("uph_file_dialog_up", data->instance)))
			actions->go_up = true;

		if (uph_ui_text_button(NAUI_TR("file_dialog.refresh"), leaf_id_indexed("uph_file_dialog_refresh", data->instance)))
			actions->refresh = true;

		const bool was_active = uph_ui_textfield_active(path_id);
		uph_ui_textfield(&dialog->path_text, path_id, UPH_UI_TEXTFIELD_FLAGS_NONE, NULL);
		const bool is_active = uph_ui_textfield_active(path_id);

		const Naui_String shown = naui_string_from_cstr(dialog->current_dir.data);
		if (was_active && !is_active && strcmp(dialog->path_text.data, shown.data) != 0)
			actions->commit_path_text = true;

		if (uph_ui_text_toggle_button(NAUI_TR("file_dialog.hidden"), leaf_id_indexed("uph_file_dialog_hidden", data->instance), dialog->show_hidden))
			actions->toggle_hidden = true;
	}
}

static void _uph_file_dialog_footer(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	Naui_Dialog *dialog = data->dialog;
	const Leaf_ID name_id = leaf_id_indexed("uph_file_dialog_name", data->instance);
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));

	if (data->error)
		leaf_text(data->error, { .color = {text_color}, .font_size = {font_size} });

	leaf({
		.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(6.0f)
	})
	{
		const bool was_active = uph_ui_textfield_active(name_id);
		uph_ui_textfield(&dialog->name, name_id, UPH_UI_TEXTFIELD_FLAGS_NONE, NAUI_TR("file_dialog.name"));
		const bool is_active = uph_ui_textfield_active(name_id);

		if (was_active && !is_active && naui_key_pressed(NAUI_KEY_ENTER))
			actions->confirm = true;

		char filters[NAUI_DIALOG_MAX_FILTERS * (NAUI_DIALOG_FILTER_MAX + 2)];
		_uph_file_dialog_filter_text(dialog, filters, sizeof(filters));
		if (filters[0])
			leaf_text(filters, { .color = {text_color}, .font_size = {font_size} });

		if (uph_ui_text_button(dialog->confirm_label, leaf_id_indexed("uph_file_dialog_confirm", data->instance)))
			actions->confirm = true;

		if (uph_ui_text_button(NAUI_TR("dialog.cancel"), leaf_id_indexed("uph_file_dialog_cancel", data->instance)))
			actions->cancel = true;
	}
}

static void _uph_file_dialog_apply(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	Naui_Dialog *dialog = data->dialog;

	if (actions->cancel)
	{
		naui_dialog_cancel(dialog);
		return;
	}

	bool list_changed = false;
	if (actions->commit_path_text)
	{
		data->error = naui_dialog_commit_path_text(dialog) ? NULL : NAUI_TR("file_dialog.error.not_found");
		list_changed = true;
	}

	if (actions->has_navigate)
	{
		data->error = NULL;
		naui_dialog_navigate(dialog, actions->navigate_to);
		list_changed = true;
	}

	if (actions->go_up && naui_dialog_navigate_up(dialog))
	{
		data->error = NULL;
		list_changed = true;
	}

	if (actions->refresh)
	{
		naui_dialog_refresh(dialog);
		list_changed = true;
	}

	if (actions->toggle_hidden)
	{
		naui_dialog_set_show_hidden(dialog, !dialog->show_hidden);
		list_changed = true;
	}

	if (list_changed)
		data->scroll = 0.0f;
	else
	{
		if (actions->clicked_row >= 0)
		{
			data->error = NULL;
			naui_dialog_select(dialog, actions->clicked_row);
		}

		if (actions->activated_row >= 0)
		{
			const Naui_DialogConfirmResult result = naui_dialog_activate(dialog, actions->activated_row);
			if (result == NAUI_DIALOG_CONFIRM_NAVIGATED)
				data->scroll = 0.0f;

			_uph_file_dialog_handle_result(data, result);
			return;
		}
	}

	if (actions->confirm)
		_uph_file_dialog_handle_result(data, naui_dialog_confirm(dialog, false));
}

#pragma endregion

#pragma region Panel

static void uph_file_dialog_on_attach(void)
{
	Uph_FileDialogData *data = (Uph_FileDialogData*)naui_current_panel_data();
	memset(data, 0, sizeof(*data));
	data->dialog = s_file_dialog_pending;
	data->instance = (uint64_t)(uintptr_t)s_file_dialog_pending;
	s_file_dialog_pending = NULL;

	const Naui_PanelID self = naui_current_panel();
	naui_panel_set_title(self, data->dialog->title);
	naui_panel_set_size(self, (Naui_Vec2){ NAUI_DPI(UPH_FILE_DIALOG_WIDTH), NAUI_DPI(UPH_FILE_DIALOG_HEIGHT) });
	naui_panel_set_min_size(self, (Naui_Vec2){ NAUI_DPI(480.0f), NAUI_DPI(300.0f) });
	naui_panel_enable_flags(self, NAUI_PANEL_FLAG_NO_DOCK);
}

static void uph_file_dialog_on_detach(void)
{

}

static void uph_file_dialog_on_open(void)
{

}

static void uph_file_dialog_on_close(void)
{
	if (s_file_dialog_recentering)
		return;

	Uph_FileDialogData *data = (Uph_FileDialogData*)naui_current_panel_data();
	Naui_Dialog *dialog = data->dialog;
	data->dialog = NULL;

	if (dialog && dialog != s_file_dialog_ending)
		naui_dialog_cancel(dialog);
}

static void uph_file_dialog_on_update(void)
{
	Uph_FileDialogData *data = (Uph_FileDialogData*)naui_current_panel_data();
	if (!data->dialog)
		return;

	Uph_FileDialogActions actions = { .clicked_row = -1, .activated_row = -1 };

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
		.direction = LEAF_DIRECTION_VERTICAL,
		.padding = LEAF_PADDING_ALL(NAUI_DPI(8.0f)),
		.child_gap = NAUI_DPI(8.0f)
	})
	{
		_uph_file_dialog_top_bar(data, &actions);

		leaf({
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_GROW},
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.child_gap = NAUI_DPI(8.0f)
		})
		{
			_uph_file_dialog_sidebar(data, &actions);
			_uph_file_dialog_list(data, &actions);
		}

		_uph_file_dialog_footer(data, &actions);
	}

	const bool typing = uph_ui_textfield_active(leaf_id_indexed("uph_file_dialog_path", data->instance)) || uph_ui_textfield_active(leaf_id_indexed("uph_file_dialog_name", data->instance));
	if (typing)
		data->dialog->state |= NAUI_DIALOG_STATE_EDITING;
	else
		data->dialog->state &= ~NAUI_DIALOG_STATE_EDITING;

	_uph_file_dialog_apply(data, &actions);
}

#pragma endregion

#pragma region Renderer

static Naui_PanelID s_file_dialog_panels[NAUI_MAX_DIALOGS];
static Naui_Dialog *s_file_dialog_panel_owners[NAUI_MAX_DIALOGS];

static bool uph_file_dialog_renderer_begin(Naui_Dialog *dialog)
{
	uint32_t slot = 0;
	while (slot < NAUI_MAX_DIALOGS && s_file_dialog_panel_owners[slot])
		slot++;

	if (slot == NAUI_MAX_DIALOGS)
		return false;

	const char *titles[] = { NAUI_TR("file_dialog.open_file.title"), NAUI_TR("file_dialog.save_file.title"), NAUI_TR("file_dialog.open_folder.title") };
	const char *labels[] = { NAUI_TR("file_dialog.open"), NAUI_TR("file_dialog.save"), NAUI_TR("file_dialog.select") };
	snprintf(dialog->title, sizeof(dialog->title), "%s", titles[dialog->mode]);
	snprintf(dialog->confirm_label, sizeof(dialog->confirm_label), "%s", labels[dialog->mode]);

	s_file_dialog_pending = dialog;
	const Naui_PanelID panel = NAUI_ATTACH_PANEL(uph_file_dialog);
	s_file_dialog_pending = NULL;
	if (!panel)
		return false;

	s_file_dialog_panels[slot] = panel;
	s_file_dialog_panel_owners[slot] = dialog;
	s_file_dialog_recentering = true;
	naui_close_panel(panel);
	s_file_dialog_recentering = false;
	naui_open_panel(panel);
	return true;
}

static void uph_file_dialog_renderer_end(Naui_Dialog *dialog)
{
	for (uint32_t i = 0; i < NAUI_MAX_DIALOGS; i++)
	{
		if (s_file_dialog_panel_owners[i] != dialog)
			continue;

		const Naui_PanelID panel = s_file_dialog_panels[i];
		s_file_dialog_panel_owners[i] = NULL;
		s_file_dialog_panels[i] = 0;

		s_file_dialog_ending = dialog;
		naui_close_panel(panel);
		s_file_dialog_ending = NULL;
		naui_detach_panel(panel);
		return;
	}
}

static const Naui_DialogRenderer s_uph_file_dialog_renderer = {
	.begin = uph_file_dialog_renderer_begin,
	.end = uph_file_dialog_renderer_end
};

void uph_file_dialog_register(void)
{
	naui_dialog_set_renderer(&s_uph_file_dialog_renderer);
}

#pragma endregion
