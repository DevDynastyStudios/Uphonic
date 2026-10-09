void uph_midi_toolbar_render(Uph_ActionMode *action_mode, Uph_SnapResolution *snap_resolution)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(16),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		const int32_t button_size = NAUI_DPI(14);
		const Naui_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
		})
		{
			const Naui_Color icon_color = naui_theme_color("uph_tool_icon_color");
			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_select"),
				leaf_id("uph_midi_editor_select"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TL | NAUI_CORNER_BL,
				*action_mode == UPH_ACTION_SELECT
			)) *action_mode = UPH_ACTION_SELECT;

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_draw"),
				leaf_id("uph_midi_editor_draw"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_NONE,
				*action_mode == UPH_ACTION_DRAW
			)) *action_mode = UPH_ACTION_DRAW;

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_cut"),
				leaf_id("uph_midi_editor_cut"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				*action_mode == UPH_ACTION_CUT
			)) *action_mode = UPH_ACTION_CUT;
		}
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
		})
		{
			leaf_text("Snap: ", {
				.font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
				.color = naui_theme_color("uph_ui_text_color")
			});
			static const char *snap_options[] = {
				"Beat",
				"1/2",
				"1/4",
				"1/8",
				"1/16"
			};

			leaf({
				.size = {LEAF_SIZE_FIXED(NAUI_DPI(60)), LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
			}) uph_ui_dropdown(snap_options, 5, (uint32_t*)snap_resolution, leaf_id("uph_midi_editor_snap"));
		}
	}
}
