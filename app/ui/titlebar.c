static void uph_render_titlebar_icon_button(Naui_Image *image, Leaf_ID id, Leaf_Color fg_color, Leaf_Color bg_color, void (*event)(void))
{
	const float dpi_scale = naui_app_dpi_scale();
	const bool hovered = leaf_hovered(id);
	if (hovered)
	{
		if (naui_mouse_clicked(NAUI_MOUSE_LEFT))
			event();
	}

	leaf({
		.id = id,
		.size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(8.0f * dpi_scale, 8.0f * dpi_scale),
		.color = hovered ?
			bg_color :
			LEAF_COLOR_TRANSPARENT
	})
	leaf({
		.size = {LEAF_SIZE_DERIVED, LEAF_SIZE_FIXED(12.0f * dpi_scale)},
		.image = image,
		.color = fg_color,
		.aspect_ratio = 1.0f
	});
}

static inline void minimize(void) { naui_defer((Naui_DeferredEvent)naui_app_minimize, NULL, 0); }
static inline void maximize(void) { naui_defer(naui_app_maximized() ? (Naui_DeferredEvent)naui_app_restore : (Naui_DeferredEvent)naui_app_maximize, NULL, 0); }

static void uph_menu_file_dropdown(void)
{
	uph_ui_text_button("New File", leaf_id("uph_file_menu_new_file"));
	if (uph_ui_text_button("Export Wav", leaf_id("uph_file_menu_export_wav")))
		uph_project_export(&uph_state.project, NAUI_PATH("test.wav"), UPH_EXPORT_WAV);
}

static void uph_menu_edit_dropdown(void)
{
	uph_ui_text_button("Undo", leaf_id("uph_edit_menu_undo"));
	uph_ui_text_button("Redo", leaf_id("uph_edit_menu_redo"));
}

static void uph_menu_windows_dropdown(void)
{
	if (uph_ui_text_button("Song Timeline", leaf_id("uph_windows_menu_song_timeline")))
	{
		Naui_PanelID panel_id = NAUI_FIND_PANEL_OF_TYPE(uph_song_timeline);
		naui_open_panel(panel_id);
	}
	if (uph_ui_text_button("Midi Editor", leaf_id("uph_windows_menu_midi_editor")))
	{
		Naui_PanelID panel_id = NAUI_FIND_PANEL_OF_TYPE(uph_midi_editor);
		naui_open_panel(panel_id);
	}
	if (uph_ui_text_button("Pattern List", leaf_id("uph_windows_menu_pattern_list")))
	{
		Naui_PanelID panel_id = NAUI_FIND_PANEL_OF_TYPE(uph_pattern_list);
		naui_open_panel(panel_id);
	}
	if (uph_ui_text_button("Sample List", leaf_id("uph_windows_menu_sample_list")))
	{
		Naui_PanelID panel_id = NAUI_FIND_PANEL_OF_TYPE(uph_sample_list);
		naui_open_panel(panel_id);
	}
}

void uph_render_main_titlebar(void)
{
	Leaf_ID left_area_id = leaf_id("uph_titlebar_left_area");
	Leaf_ID right_area_id = leaf_id("uph_titlebar_right_area");

	bool titlebar_widgets_hovered = leaf_hovered(left_area_id) || leaf_hovered(right_area_id);

	const float dpi_scale = naui_app_dpi_scale();
	const float titlebar_height = 32.0f * dpi_scale;
	naui_app_set_caption_area(0, 0, naui_app_width() * dpi_scale, titlebar_widgets_hovered || naui_any_panel_hovered() ? 0 : titlebar_height);

	Naui_Vec2 padding = naui_theme_vec2(NAUI_PANEL_TITLEBAR_PADDING_TAG);
	Leaf_Color text_color = naui_theme_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG);

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(titlebar_height)},
		.color = {naui_theme_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG)},
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
	})
	{
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.padding = LEAF_PADDING_AXES(padding.x, 0.0f),
			.child_gap = NAUI_DPI(4.0f)
		})
		{
			leaf({
				.size = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT(0.5f)},
				.image = naui_asset_image("uph_logo_small"),
				.color = LEAF_COLOR_WHITE,
				.aspect_ratio = 1.0f
			});
			leaf({
				.id = left_area_id,
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
			})
			{
				Uph_UIMenuID file_menu = uph_ui_menu(NAUI_TR("menu.file"), leaf_id("uph_file_menu"));
				if (uph_ui_menu_item(file_menu, NAUI_TR("menu.file.new"), leaf_id("uph_file_menu_new_project")))
				{

				}

				if (uph_ui_menu_item(file_menu, NAUI_TR("menu.file.open"), leaf_id("uph_file_menu_open")))
				{

				}

				if (uph_ui_menu_item(file_menu, NAUI_TR("menu.file.save"), leaf_id("uph_file_menu_save")))
				{

				}


				Uph_UIMenuID import_menu = uph_ui_submenu(file_menu, NAUI_TR("menu.file.import"), leaf_id("uph_file_import_menu"));
				if (uph_ui_menu_item(import_menu, NAUI_TR("menu.file.import.midi"), leaf_id("uph_file_import_midi")))
				{

				}

				Uph_UIMenuID export_menu = uph_ui_submenu(file_menu, NAUI_TR("menu.file.export"), leaf_id("uph_file_export_menu"));
				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.uph"), leaf_id("uph_file_export_uph")))
				{
					Naui_Path archive_path = naui_path_normalize(naui_path_join(naui_directory_get(NAUI_DIR_APPDATA), NAUI_PATH("Uphonic/test.uph")));
					uph_project_export(&uph_state.project, archive_path, UPH_EXPORT_UPH);	
				}

				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.wav"), leaf_id("uph_file_export_wav")))
					uph_project_export(&uph_state.project, NAUI_PATH("test.wav"), UPH_EXPORT_WAV);

				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.ogg"), leaf_id("uph_file_export_ogg")))
				{

				}
					
				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.mp3"), leaf_id("uph_file_export_mp3")))
				{

				}

				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.flac"), leaf_id("uph_file_export_flac")))
				{

				}

				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.m4a"), leaf_id("uph_file_export_m4a")))
				{

				}

				if (uph_ui_menu_item(export_menu, NAUI_TR("menu.file.export.midi"), leaf_id("uph_file_export_midi")))
				{

				}


				Uph_UIMenuID edit_menu = uph_ui_menu(NAUI_TR("menu.edit"), leaf_id("uph_edit_menu"));
				if (uph_ui_menu_item(edit_menu, NAUI_TR("menu.edit.undo"), leaf_id("uph_edit_menu_undo")))
				{

				}

				if (uph_ui_menu_item(edit_menu, NAUI_TR("menu.edit.redo"), leaf_id("uph_edit_menu_redo")))
				{

				}


				Uph_UIMenuID windows_menu = uph_ui_menu(NAUI_TR("menu.windows"), leaf_id("uph_windows_menu"));
				if (uph_ui_menu_item(windows_menu, NAUI_TR("song_timeline.title"), leaf_id("uph_windows_menu_song_timeline")))
					naui_open_panel(NAUI_FIND_PANEL_OF_TYPE(uph_song_timeline));

				if (uph_ui_menu_item(windows_menu, NAUI_TR("midi_editor.title"), leaf_id("uph_windows_menu_midi_editor")))
					naui_open_panel(NAUI_FIND_PANEL_OF_TYPE(uph_midi_editor));

				if (uph_ui_menu_item(windows_menu, NAUI_TR("patterns.title"), leaf_id("uph_windows_menu_pattern_list")))
					naui_open_panel(NAUI_FIND_PANEL_OF_TYPE(uph_pattern_list));

				if (uph_ui_menu_item(windows_menu, NAUI_TR("samples.title"), leaf_id("uph_windows_menu_sample_list")))
					naui_open_panel(NAUI_FIND_PANEL_OF_TYPE(uph_sample_list));

				Uph_UIMenuID track_menu = uph_ui_menu(NAUI_TR("menu.track"), leaf_id("uph_track_menu"));
				if (uph_ui_menu_item(track_menu, NAUI_TR("menu.track.new"), leaf_id("uph_track_menu_new")))
				{
					Uph_Track track = {
						.name = naui_string_from_cstr("New Track"),
						.volume = 1.0f,
						.color = naui_theme_color("uph_palette_color_1")
					};
					naui_list_push(uph_state.project.tracks, track);
				}
			}
		}

		leaf({
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
		})
		leaf_text(NAUI_TR("uphonic.title"), {
			.font_size = naui_theme_float(NAUI_PANEL_FONT_SIZE_TAG) * dpi_scale,
			.color = text_color,
			.alignment = LEAF_TEXT_ALIGN_CENTER
		});
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
		})
		{
			leaf({
				.id = right_area_id,
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL}
			})
			{
				uph_render_titlebar_icon_button(naui_asset_image(NAUI_MINIMIZE_ICON_TAG), leaf_id_indexed("__naui_titlebar_btn", 0), text_color, naui_theme_color(NAUI_PANEL_BUTTON_HOVERED_BG_COLOR_TAG), minimize);
				uph_render_titlebar_icon_button(naui_asset_image(NAUI_MAXIMIZE_ICON_TAG), leaf_id_indexed("__naui_titlebar_btn", 1), text_color, naui_theme_color(NAUI_PANEL_BUTTON_HOVERED_BG_COLOR_TAG), maximize);
				uph_render_titlebar_icon_button(naui_asset_image(NAUI_CLOSE_ICON_TAG), leaf_id_indexed("__naui_titlebar_btn", 2), text_color, naui_theme_color(NAUI_PANEL_CLOSE_HOVERED_BG_COLOR_TAG), naui_app_close);
			}
		}
	}
}
