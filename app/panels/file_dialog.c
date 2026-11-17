#define UPH_FILE_DIALOG_WIDTH 760
#define UPH_FILE_DIALOG_HEIGHT 480
#define UPH_FILE_DIALOG_SIDEBAR_WIDTH 150
#define UPH_FILE_DIALOG_ROW_HEIGHT 24
#define UPH_FILE_DIALOG_ICON_SIZE 14
#define UPH_FILE_DIALOG_SIZE_COLUMN 80

typedef struct
{
	Naui_Dialog *dialog;
	const char *error_key;
	float scroll;
	uint64_t instance;
}
Uph_FileDialogData;

NAUI_PANEL_WITH_DATA(uph_file_dialog, Uph_FileDialogData)

typedef struct
{
	Naui_Dialog *pending;
	Naui_Dialog *ending;
	bool recentering;
	char overwrite_key[NAUI_DIALOG_KEY_MAX];
	Naui_PanelID panels[NAUI_MAX_DIALOGS];
	Naui_Dialog *panel_owners[NAUI_MAX_DIALOGS];
}
Uph_FileDialogState;

static Uph_FileDialogState uph__file_dialog_state;

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

static void uph__file_dialog_format_size(size_t size, char *out, size_t out_size)
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

static void uph__file_dialog_filter_text(const Naui_Dialog *dialog, char *out, size_t out_size)
{
	out[0] = '\0';
	for (uint32_t i = 0; i < dialog->filter_count; i++)
	{
		const size_t used = strlen(out);
		snprintf(out + used, out_size - used, "%s%s", i ? ", " : "", dialog->filters[i]);
	}
}

static void uph__file_dialog_overwrite_result(int32_t button, void *user_data)
{
	Uph_FileDialogState *state = &uph__file_dialog_state;
	(void)user_data;
	if (button != 0)
		return;

	Naui_Dialog *dialog = naui_dialog_find(state->overwrite_key);
	if (dialog)
		naui_dialog_confirm(dialog, true);
}

static void uph__file_dialog_handle_result(Uph_FileDialogData *data, Naui_DialogConfirmResult result)
{
	Uph_FileDialogState *state = &uph__file_dialog_state;
	data->error_key = NULL;

	switch (result)
	{
		case NAUI_DIALOG_CONFIRM_DONE:
		case NAUI_DIALOG_CONFIRM_NAVIGATED:
			break;

		case NAUI_DIALOG_CONFIRM_NEEDS_OVERWRITE:
			snprintf(state->overwrite_key, sizeof(state->overwrite_key), "%s", data->dialog->key);
			uph_dialog_open(&(Uph_DialogConfig){
				.title = NAUI_TR("file_dialog.overwrite.title"),
				.message = NAUI_TR("file_dialog.overwrite.message"),
				.buttons = { NAUI_TR("file_dialog.overwrite.replace"), NAUI_TR("dialog.cancel") },
				.default_button = -1,
				.cancel_button = 1,
				.callback = uph__file_dialog_overwrite_result
			});
			break;

		case NAUI_DIALOG_CONFIRM_NOTHING_SELECTED:
			data->error_key = "file_dialog.error.nothing_selected";
			break;

		case NAUI_DIALOG_CONFIRM_INVALID_NAME:
			data->error_key = "file_dialog.error.invalid_name";
			break;

		case NAUI_DIALOG_CONFIRM_NOT_FOUND:
			data->error_key = "file_dialog.error.not_found";
			break;
	}
}

#pragma endregion

#pragma region UI

static void uph__file_dialog_place(Uph_FileDialogData *data, Uph_FileDialogActions *actions, const char *label, const Naui_Path path, uint64_t index)
{
	if (!path.data[0] || !naui_path_is_directory(path))
		return;

	const Leaf_ID id = leaf_id_indexed("uph_file_dialog_place", data->instance + index);
	const bool hovered = uph_ui_widget_hovered(id);
	const bool current = strcmp(data->dialog->current_dir.data, path.data) == 0;
	const int32_t icon_size = NAUI_DPI(UPH_FILE_DIALOG_ICON_SIZE);

	if (hovered)
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		{
			actions->navigate_to = path;
			actions->has_navigate = true;
		}
	}

	Leaf_Color background = LEAF_COLOR_TRANSPARENT;
	if (current)
		background = uph_color_mix(uph_state.colors.ui_well_bg, uph_state.colors.widget_accent, 0.35f);
	else if (hovered)
		background = uph_state.colors.ui_frame_hovered_bg;

	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_FILE_DIALOG_ROW_HEIGHT)) },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(8), 0),
		.color = background,
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.child_gap = NAUI_DPI(8),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.clip_children = true
	})
	{
		leaf({
			.image = (void*)uph_state.icons.folder,
			.size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
			.color = current ? uph_state.colors.ui_text : uph_state.colors.ui_text_disabled
		});

		leaf_text(label, {
			.color = uph_state.colors.ui_text,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
		});
	}
}

static void uph__file_dialog_caption(const char *text)
{
	leaf_text(text, {
		.color = uph_state.colors.ui_text_disabled,
		.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE - 2))
	});
}

static void uph__file_dialog_sidebar(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	leaf({
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_FILE_DIALOG_SIDEBAR_WIDTH)), .height = LEAF_SIZE_GROW },
		.padding = LEAF_PADDING_ALL(NAUI_DPI(6)),
		.color = uph_state.colors.ui_well_bg,
		.border = { .width = 1, .sides = LEAF_SIDE_ALL, .color = uph_state.colors.ui_frame_border },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.child_gap = NAUI_DPI(2),
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
			.padding = { .left = NAUI_DPI(6), .right = NAUI_DPI(6), .top = NAUI_DPI(2), .bottom = NAUI_DPI(4) }
		})
		{
			uph__file_dialog_caption(NAUI_TR("file_dialog.places"));
		}

		uph__file_dialog_place(data, actions, NAUI_TR("file_dialog.places.home"), naui_directory_get(NAUI_DIR_HOME), 1);
		uph__file_dialog_place(data, actions, NAUI_TR("file_dialog.places.downloads"), naui_directory_get(NAUI_DIR_DOWNLOADS), 2);
		uph__file_dialog_place(data, actions, NAUI_TR("file_dialog.places.projects"), UPHONIC_WORKSPACE_FOLDER, 3);
	}
}

static void uph__file_dialog_row(Uph_FileDialogData *data, Uph_FileDialogActions *actions, const Naui_DirEntry *entry, int32_t index)
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

	const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const int32_t icon_size = NAUI_DPI(UPH_FILE_DIALOG_ICON_SIZE);
	const Leaf_Color text_color = uph_state.colors.ui_text;
	const Leaf_Color muted_color = uph_state.colors.ui_text_disabled;

	Leaf_Color background = LEAF_COLOR_TRANSPARENT;
	if (selected)
		background = uph_color_mix(uph_state.colors.ui_well_bg, uph_state.colors.widget_accent, 0.5f);
	else if (hovered)
		background = uph_state.colors.ui_frame_hovered_bg;

	leaf({
		.id = row_id,
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_FILE_DIALOG_ROW_HEIGHT)) },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(8), 0),
		.color = background,
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.child_gap = NAUI_DPI(8),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({
			.image = (void*)(entry->is_directory ? uph_state.icons.folder : uph_state.icons.file),
			.size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
			.color = entry->is_directory || selected ? text_color : muted_color
		});

		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
			.clip_children = true
		})
		{
			leaf_text(naui_file_filename(&entry->path).data, { .color = text_color, .font_size = LEAF_SIZE_FIXED(font_size) });
		}

		leaf({
			.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_FILE_DIALOG_SIZE_COLUMN)), .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER }
		})
		{
			if (!entry->is_directory)
			{
				char size_text[32];
				uph__file_dialog_format_size(entry->size, size_text, sizeof(size_text));
				leaf_text(size_text, { .color = selected ? text_color : muted_color, .font_size = LEAF_SIZE_FIXED(font_size) });
			}
		}
	}
}

static void uph__file_dialog_list_header(void)
{
	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
		.padding = { .left = NAUI_DPI(14 + UPH_FILE_DIALOG_ICON_SIZE + 8), .right = NAUI_DPI(14), .top = NAUI_DPI(6), .bottom = NAUI_DPI(6) },
		.border = { .width = 1, .sides = LEAF_SIDE_BOTTOM, .color = uph_state.colors.ui_frame_border },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(8),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({ .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT } })
		{
			uph__file_dialog_caption(NAUI_TR("file_dialog.name"));
		}

		leaf({
			.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_FILE_DIALOG_SIZE_COLUMN)), .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER }
		})
		{
			uph__file_dialog_caption(NAUI_TR("file_dialog.size"));
		}
	}
}

static void uph__file_dialog_list(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	const Naui_Dialog *dialog = data->dialog;
	const uint32_t count = (uint32_t)naui_list_len(dialog->entries);

	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
		.color = uph_state.colors.ui_well_bg,
		.border = { .width = 1, .sides = LEAF_SIDE_ALL, .color = uph_state.colors.ui_frame_border },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.direction = LEAF_DIRECTION_VERTICAL,
		.clip_children = true
	})
	{
		uph__file_dialog_list_header();

		if (count == 0)
		{
			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
				.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
			})
			{
				leaf_text(NAUI_TR("file_dialog.empty"), {
					.color = uph_state.colors.ui_text_disabled,
					.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
				});
			}
		}
		else
		{
			Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(
				UPH_UI_SCROLL_DIRECTION_VERTICAL,
				&data->scroll,
				leaf_id_indexed("uph_file_dialog_scroll", data->instance)
			);

			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
				.padding = LEAF_PADDING_ALL(NAUI_DPI(6)),
				.child_gap = NAUI_DPI(1),
				.direction = LEAF_DIRECTION_VERTICAL
			})
			{
				for (uint32_t i = 0; i < count; i++)
					uph__file_dialog_row(data, actions, &dialog->entries[i], (int32_t)i);
			}

			uph_ui_end_scroll_container(&scroll_container);
		}
	}
}

static void uph__file_dialog_top_bar(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	Naui_Dialog *dialog = data->dialog;
	const Leaf_ID path_id = leaf_id_indexed("uph_file_dialog_path", data->instance);

	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(6),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		if (uph_ui_text_button(NAUI_TR("file_dialog.up"), leaf_id_indexed("uph_file_dialog_up", data->instance)))
			actions->go_up = true;

		const float icon_size = NAUI_DPI(UPH_FILE_DIALOG_ICON_SIZE);
		if (uph_ui_image_button(uph_state.icons.reload, leaf_id_indexed("uph_file_dialog_refresh", data->instance), (Naui_Vec2){ icon_size, icon_size }, uph_state.colors.ui_text))
			actions->refresh = true;

		const bool was_active = uph_ui_textfield_active(path_id);
		uph_ui_textfield(&dialog->path_text, path_id, UPH_UI_TEXTFIELD_FLAG_NONE, NULL);
		const bool is_active = uph_ui_textfield_active(path_id);

		const Naui_String shown = naui_string_from_cstr(dialog->current_dir.data);
		if (was_active && !is_active && strcmp(dialog->path_text.data, shown.data) != 0)
			actions->commit_path_text = true;

		if (uph_ui_text_toggle_button(NAUI_TR("file_dialog.hidden"), leaf_id_indexed("uph_file_dialog_hidden", data->instance), dialog->show_hidden))
			actions->toggle_hidden = true;
	}
}

static void uph__file_dialog_footer(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
{
	Naui_Dialog *dialog = data->dialog;
	const Leaf_ID name_id = leaf_id_indexed("uph_file_dialog_name", data->instance);
	const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);

	if (data->error_key)
		leaf_text(NAUI_TR(data->error_key), { .color = uph_state.colors.record_armed, .font_size = LEAF_SIZE_FIXED(font_size) });

	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(8),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		const bool was_active = uph_ui_textfield_active(name_id);
		uph_ui_textfield(&dialog->name, name_id, UPH_UI_TEXTFIELD_FLAG_NONE, NAUI_TR("file_dialog.name"));
		const bool is_active = uph_ui_textfield_active(name_id);

		if (was_active && !is_active && naui_key_pressed(NAUI_KEY_ENTER))
			actions->confirm = true;

		char filters[NAUI_DIALOG_MAX_FILTERS * (NAUI_DIALOG_FILTER_MAX + 2)];
		uph__file_dialog_filter_text(dialog, filters, sizeof(filters));
		if (filters[0])
			leaf_text(filters, { .color = uph_state.colors.ui_text_disabled, .font_size = LEAF_SIZE_FIXED(font_size) });

		if (uph_ui_text_button_ex(dialog->confirm_label, leaf_id_indexed("uph_file_dialog_confirm", data->instance), uph_state.colors.widget_accent, NAUI_CORNER_ALL))
			actions->confirm = true;

		if (uph_ui_text_button(NAUI_TR("dialog.cancel"), leaf_id_indexed("uph_file_dialog_cancel", data->instance)))
			actions->cancel = true;
	}
}

static void uph__file_dialog_apply(Uph_FileDialogData *data, Uph_FileDialogActions *actions)
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
		data->error_key = naui_dialog_commit_path_text(dialog) ? NULL : "file_dialog.error.not_found";
		list_changed = true;
	}

	if (actions->has_navigate)
	{
		data->error_key = NULL;
		naui_dialog_navigate(dialog, actions->navigate_to);
		list_changed = true;
	}

	if (actions->go_up && naui_dialog_navigate_up(dialog))
	{
		data->error_key = NULL;
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
			data->error_key = NULL;
			naui_dialog_select(dialog, actions->clicked_row);
		}

		if (actions->activated_row >= 0)
		{
			const Naui_DialogConfirmResult result = naui_dialog_activate(dialog, actions->activated_row);
			if (result == NAUI_DIALOG_CONFIRM_NAVIGATED)
				data->scroll = 0.0f;

			uph__file_dialog_handle_result(data, result);
			return;
		}
	}

	if (actions->confirm)
		uph__file_dialog_handle_result(data, naui_dialog_confirm(dialog, false));
}

#pragma endregion

#pragma region Panel

static void uph_file_dialog_on_attach(void)
{
	Uph_FileDialogState *state = &uph__file_dialog_state;
	Uph_FileDialogData *data = (Uph_FileDialogData*)naui_panel_current_data();
	memset(data, 0, sizeof(*data));
	data->dialog = state->pending;
	data->instance = (uint64_t)(uintptr_t)state->pending;
	state->pending = NULL;

	const Naui_PanelID self = naui_panel_current();
	naui_panel_set_title(self, data->dialog->title);
	naui_panel_set_size(self, (Naui_Vec2){ NAUI_DPI(UPH_FILE_DIALOG_WIDTH), NAUI_DPI(UPH_FILE_DIALOG_HEIGHT) });
	naui_panel_set_min_size(self, (Naui_Vec2){ NAUI_DPI(480), NAUI_DPI(300) });
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
	Uph_FileDialogState *state = &uph__file_dialog_state;
	if (state->recentering)
		return;

	Uph_FileDialogData *data = (Uph_FileDialogData*)naui_panel_current_data();
	Naui_Dialog *dialog = data->dialog;
	data->dialog = NULL;

	if (dialog && dialog != state->ending)
		naui_dialog_cancel(dialog);
}

static void uph_file_dialog_on_update(void)
{
	Uph_FileDialogData *data = (Uph_FileDialogData*)naui_panel_current_data();
	if (!data->dialog)
		return;

	Uph_FileDialogActions actions = { .clicked_row = -1, .activated_row = -1 };

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.padding = LEAF_PADDING_ALL(NAUI_DPI(10)),
		.child_gap = NAUI_DPI(10),
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{
		uph__file_dialog_top_bar(data, &actions);

		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
			.child_gap = NAUI_DPI(10),
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			uph__file_dialog_sidebar(data, &actions);
			uph__file_dialog_list(data, &actions);
		}

		uph__file_dialog_footer(data, &actions);
	}

	const bool typing = uph_ui_textfield_active(leaf_id_indexed("uph_file_dialog_path", data->instance)) || uph_ui_textfield_active(leaf_id_indexed("uph_file_dialog_name", data->instance));
	if (typing)
		data->dialog->state |= NAUI_DIALOG_STATE_EDITING;
	else
		data->dialog->state &= ~NAUI_DIALOG_STATE_EDITING;

	uph__file_dialog_apply(data, &actions);
}

#pragma endregion

#pragma region Renderer

static bool uph_file_dialog_renderer_begin(Naui_Dialog *dialog)
{
	Uph_FileDialogState *state = &uph__file_dialog_state;
	uint32_t slot = 0;
	while (slot < NAUI_MAX_DIALOGS && state->panel_owners[slot])
		slot++;

	if (slot == NAUI_MAX_DIALOGS)
		return false;

	const char *titles[] = { NAUI_TR("file_dialog.open_file.title"), NAUI_TR("file_dialog.save_file.title"), NAUI_TR("file_dialog.open_folder.title"), NAUI_TR("file_dialog.open_file.title") };
	const char *labels[] = { NAUI_TR("file_dialog.open"), NAUI_TR("file_dialog.save"), NAUI_TR("file_dialog.select"), NAUI_TR("file_dialog.open") };
	snprintf(dialog->title, sizeof(dialog->title), "%s", titles[dialog->mode]);
	snprintf(dialog->confirm_label, sizeof(dialog->confirm_label), "%s", labels[dialog->mode]);

	state->pending = dialog;
	const Naui_PanelID panel = NAUI_ATTACH_PANEL(uph_file_dialog);
	state->pending = NULL;
	if (!panel)
		return false;

	state->panels[slot] = panel;
	state->panel_owners[slot] = dialog;
	state->recentering = true;
	naui_panel_close(panel);
	state->recentering = false;
	naui_panel_open(panel);
	return true;
}

static void uph_file_dialog_renderer_end(Naui_Dialog *dialog)
{
	Uph_FileDialogState *state = &uph__file_dialog_state;
	for (uint32_t i = 0; i < NAUI_MAX_DIALOGS; i++)
	{
		if (state->panel_owners[i] != dialog)
			continue;

		const Naui_PanelID panel = state->panels[i];
		state->panel_owners[i] = NULL;
		state->panels[i] = 0;

		state->ending = dialog;
		naui_panel_close(panel);
		state->ending = NULL;
		naui_panel_detach(panel);
		return;
	}
}

static const Naui_DialogRenderer uph__file_dialog_renderer = {
	.begin = uph_file_dialog_renderer_begin,
	.end = uph_file_dialog_renderer_end
};

void uph_file_dialog_register(void)
{
	naui_dialog_set_renderer(&uph__file_dialog_renderer);
}

#pragma endregion
