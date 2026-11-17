#define UPH_SAVE_PROJECT_DIALOG_KEY "uph__save_project"

static void uph_render_titlebar_icon_button(Naui_Image *image, Leaf_ID id, Leaf_Color fg_color, Leaf_Color bg_color, void (*event)(void))
{
	const bool hovered = leaf_hovered(id);
	if (hovered)
	{
		if (naui_mouse_clicked(NAUI_MOUSE_LEFT))
			event();
	}

	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(8), 0.0f),
		.color = hovered ?
			bg_color :
			LEAF_COLOR_TRANSPARENT,
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
	})
	leaf({
		.image = image,
		.size = { .width = LEAF_SIZE_DERIVED, .height = LEAF_SIZE_FIXED(NAUI_DPI(12)) },
		.color = fg_color,
		.aspect_ratio = 1.0f
	});
}

static inline void uph_defer_minimize(void) { naui_defer((Naui_DeferredEvent)naui_app_minimize, NULL, 0); }
static inline void uph_defer_maximize(void) { naui_defer(naui_app_maximized() ? (Naui_DeferredEvent)naui_app_restore : (Naui_DeferredEvent)naui_app_maximize, NULL, 0); }

#pragma region File Dialogs

static void uph__open_project_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	(void)user_data;
	if (is_cancelled || result->count == 0)
		return;

	const Naui_Path picked = naui_path_normalize(result->paths[0]);
	uph_project_load(&uph_state.project, picked);
}

static void uph__import_midi(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	if (is_cancelled || result->count == 0)
		return;

	uph_project_add_file(&uph_state.project, result->paths[0]);
}

static void uph__export_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	if (is_cancelled || result->count == 0)
		return;

	uph_project_export(&uph_state.project, result->paths[0], (Uph_ExportFormat)(uintptr_t)user_data);
}

static void uph__open_export_dialog(const char *key, const char *title, const char *extension, Uph_ExportFormat format)
{
	if (!naui_dialog_save_file(key, NAUI_EXTENSIONS(extension), uph__export_picked, (void*)(uintptr_t)format))
		return;

	Naui_String name = uph_state.project.title;
	naui_string_append_cstr(&name, extension);
	naui_dialog_title_set(key, title);
	naui_dialog_name_set(key, name.data);
}

static void uph__save_project_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	(void)user_data;
	if (is_cancelled || result->count == 0)
		return;

	if (uph_project_save_as(&uph_state.project, result->paths[0]))
		return;

	uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("dialog.save_project.failed.title"),
		.message = NAUI_TR("dialog.save_project.failed.message"),
		.buttons = { NAUI_TR("dialog.ok") },
		.default_button = 0,
		.cancel_button = 0
	});
}

static void uph__open_save_dialog(const char *title)
{
	if (naui_dialog_find(UPH_SAVE_PROJECT_DIALOG_KEY))
		return;

	naui_dialog_default_path_set(UPHONIC_WORKSPACE_FOLDER);
	if (!naui_dialog_save_file(UPH_SAVE_PROJECT_DIALOG_KEY, NULL, uph__save_project_picked, NULL))
		return;

	Naui_String suggestion = { 0 };
	if (uph_project_has_been_saved(&uph_state.project))
	{
		const Naui_Path free_folder = uph_project_unique_workspace_folder(uph_state.project.title.data);
		suggestion = naui_view_to_string(naui_file_filename(&free_folder));
	}

	naui_dialog_title_set(UPH_SAVE_PROJECT_DIALOG_KEY, title);
	naui_dialog_name_set(UPH_SAVE_PROJECT_DIALOG_KEY, suggestion.data);
}

static void uph__save_project(void)
{
	if (uph_project_has_been_saved(&uph_state.project))
		uph_project_save(&uph_state.project, UPH_SAVE_TYPE_CANONICAL);
	else
		uph__open_save_dialog(NAUI_TR("menu.file.save"));
}

#pragma endregion

void uph_render_main_titlebar(void)
{
	Leaf_ID left_area_id = leaf_id("uph_titlebar_left_area");
	Leaf_ID right_area_id = leaf_id("uph_titlebar_right_area");

	bool titlebar_widgets_hovered = leaf_hovered(left_area_id) || leaf_hovered(right_area_id);

	const int32_t titlebar_height = NAUI_DPI(34);
	naui_app_set_caption_area(0, 0, NAUI_DPI(naui_app_width()), titlebar_widgets_hovered || naui_panel_is_any_hovered() ? 0 : titlebar_height);

	Naui_Vec2 padding = NAUI_PANEL_TITLEBAR_PADDING;
	Leaf_Color text_color = uph_state.colors.panel_title_text;

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(titlebar_height) },
		.color = uph_state.colors.panel_title_bg,
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER }
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
			.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0.0f),
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(10),
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
		})
		{
			leaf({
				.image = uph_state.icons.logo_small,
				.size = { .width = LEAF_SIZE_DERIVED, .height = LEAF_SIZE_PERCENT(0.5f) },
				.color = uph_state.colors.logo,
				.aspect_ratio = 1.0f
			});
			leaf({
				.id = left_area_id,
				.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
				.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
				.child_gap = NAUI_DPI(UPH_UI_MENU_BAR_ITEM_GAP),
				.direction = LEAF_DIRECTION_HORIZONTAL
			})
			{
				const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;

				Uph_UIMenuID file_menu = uph_ui_menu(NAUI_TR("menu.file"), leaf_id("uph_file_menu"));
				if (uph_ui_menu_keybind_item(file_menu, NAUI_TR("menu.file.new"), leaf_id("uph_file_menu_new_project"), keybinds->new_project))
				{
					// Check if project has initial save
					// If not, delete the project folder
					uph_project_create(uph_project_untitled_name());
				}

				if (uph_ui_menu_keybind_item(file_menu, NAUI_TR("menu.file.open"), leaf_id("uph_file_menu_open"), keybinds->open_project))
				{
					naui_dialog_default_path_set(UPHONIC_WORKSPACE_FOLDER);
					naui_dialog_open_file_or_folder("uph_open_project", NAUI_EXTENSIONS(".uph"), uph__open_project_picked, NULL);
					naui_dialog_title_set("uph_open_project", NAUI_TR("menu.file.open"));
				}

				if (uph_ui_menu_keybind_item(file_menu, NAUI_TR("menu.file.save"), leaf_id("uph_file_menu_save"), keybinds->save_project))
					uph__save_project();

				if (uph_ui_menu_keybind_item(file_menu, NAUI_TR("menu.file.save_as"), leaf_id("uph_file_menu_save_as"), keybinds->save_project_as))
					uph__open_save_dialog(NAUI_TR("menu.file.save_as"));

				Uph_UIMenuID import_menu = uph_ui_submenu(file_menu, NAUI_TR("menu.file.import"), leaf_id("uph_file_import_menu"));
				if (uph_ui_menu_item(import_menu, NAUI_TR("menu.file.import.midi"), leaf_id("uph_file_import_midi")))
				{
					naui_dialog_open_file("menu.file.import.midi", NAUI_EXTENSIONS(".mid", ".midi", ".smf", ".kar", ".rmi"), uph__import_midi, NULL);
				}

				Uph_UIMenuID export_menu = uph_ui_submenu(file_menu, NAUI_TR("menu.file.export"), leaf_id("uph_file_export_menu"));
				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.uph"), leaf_id("uph_file_export_uph")))
					uph__open_export_dialog("uph_export_uph", NAUI_TR("menu.file.export.uph"), ".uph", UPH_EXPORT_UPH);

				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.wav"), leaf_id("uph_file_export_wav")))
					naui_panel_open(uph_state.panels.render);

				// if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.ogg"), leaf_id("uph_file_export_ogg")))
				// {

				// }
					
				// if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.mp3"), leaf_id("uph_file_export_mp3")))
				// {

				// }

				// if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.flac"), leaf_id("uph_file_export_flac")))
				// {

				// }

				// if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.m4a"), leaf_id("uph_file_export_m4a")))
				// {

				// }

				// if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.midi"), leaf_id("uph_file_export_midi")))
				// {

				// }


				Uph_UIMenuID edit_menu = uph_ui_menu(NAUI_TR("menu.edit"), leaf_id("uph_edit_menu"));
				if (uph_ui_menu_keybind_item(edit_menu, NAUI_TR("menu.edit.undo"), leaf_id("uph_edit_menu_undo"), keybinds->undo))
					naui_action_undo();

				if (uph_ui_menu_keybind_item(edit_menu, NAUI_TR("menu.edit.redo"), leaf_id("uph_edit_menu_redo"), keybinds->redo))
					naui_action_redo();

				if (uph_ui_menu_keybind_item(edit_menu, NAUI_TR("menu.edit.settings"), leaf_id("uph_edit_menu_settings"), keybinds->open_settings))
					naui_panel_open(uph_state.panels.settings);

				Uph_UIMenuID windows_menu = uph_ui_menu(NAUI_TR("menu.windows"), leaf_id("uph_windows_menu"));
				if (uph_ui_menu_item(windows_menu, NAUI_TR("song_timeline.title"), leaf_id("uph_windows_menu_song_timeline")))
					naui_panel_open(uph_state.panels.song_timeline);

				if (uph_ui_menu_item(windows_menu, NAUI_TR("resource_editor.title"), leaf_id("uph_windows_menu_midi_editor")))
					naui_panel_open(uph_state.panels.resource_editor);

				if (uph_ui_menu_item(windows_menu, NAUI_TR("resource_list.title"), leaf_id("uph_windows_menu_resource_list")))
					naui_panel_open(uph_state.panels.resource_list);

				if (uph_ui_menu_item(windows_menu, NAUI_TR("sample_browser.title"), leaf_id("uph_windows_menu_sample_browser")))
					naui_panel_open(uph_state.panels.sample_browser);

				if (uph_ui_menu_item(windows_menu, NAUI_TR("mixer.title"), leaf_id("uph_windows_menu_mixer")))
					naui_panel_open(uph_state.panels.mixer);

				uph_layouts_render_menu();
			}
		}

		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
			.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
		})
		leaf_text(NAUI_TR("uphonic.title"), {
			.color = text_color,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(NAUI_PANEL_FONT_SIZE)),
			.alignment = LEAF_TEXT_ALIGN_CENTER
		});
		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
			.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER },
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
		})
		{
			leaf({
				.id = right_area_id,
				.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
				.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER },
				.direction = LEAF_DIRECTION_HORIZONTAL
			})
			{
				uph_render_titlebar_icon_button(uph_state.icons.window_minimize, leaf_id_indexed("_naui__titlebar_btn", 0), text_color, uph_state.colors.panel_button_hovered_bg, uph_defer_minimize);
				uph_render_titlebar_icon_button(uph_state.icons.window_maximize, leaf_id_indexed("_naui__titlebar_btn", 1), text_color, uph_state.colors.panel_button_hovered_bg, uph_defer_maximize);
				uph_render_titlebar_icon_button(uph_state.icons.window_close, leaf_id_indexed("_naui__titlebar_btn", 2), text_color, uph_state.colors.panel_close_hovered_bg, naui_app_close);
			}
		}
	}
}
