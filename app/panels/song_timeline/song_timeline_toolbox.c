void uph_song_timeline_render_toolbox(void)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(40))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_gap = NAUI_DPI(8),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		const int32_t button_size = NAUI_DPI(naui_theme_float("uph_song_timeline_icon_size"));
		const Naui_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
		const Naui_Color line_color = naui_theme_color("uph_toolbox_separator_color");
		const Naui_Color icon_color = naui_theme_color("uph_tool_icon_color");

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
		})
		{
			if (uph_ui_image_button_ex(
				naui_asset_image(uph_state.shared.song_timeline_playing ? "uph_icon_pause" : "uph_icon_play"),
				leaf_id("uph_song_timeline_play"),
				(Naui_Vec2){button_size, button_size},
				naui_theme_color(uph_state.shared.song_timeline_playing ? "uph_pause_icon_color" : "uph_play_icon_color"),
				bg_color,
				NAUI_CORNER_TL | NAUI_CORNER_BL
			))
			{
				uph_state.shared.song_timeline_playing = !uph_state.shared.song_timeline_playing;
			}

			if (uph_ui_image_button_ex(
				naui_asset_image("uph_icon_stop"),
				leaf_id("uph_song_timeline_stop"),
				(Naui_Vec2){button_size, button_size},
				naui_theme_color("uph_stop_icon_color"),
				bg_color,
				NAUI_CORNER_NONE
			))
			{
				uph_state.shared.song_timeline_playing = false;
				uph_state.shared.song_timeline_playhead_position = 0.0;
			}

			if (uph_ui_image_button_ex(
				naui_asset_image("uph_icon_reset"),
				leaf_id("uph_song_timeline_reset"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR
			))
			{
				uph_state.shared.song_timeline_playhead_position = 0.0;
			}

			/*if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_loop"),
				leaf_id("uph_song_timeline_loop"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				false
			));*/
		}
		leaf({.size = {LEAF_SIZE_FIXED(NAUI_DPI(1)), LEAF_SIZE_PERCENT(0.5f)}, .color = {line_color}});
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
		})
		{
			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_select"),
				leaf_id("uph_song_timeline_select"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TL | NAUI_CORNER_BL,
				data->current_action_mode == UPH_ACTION_SELECT
			)) data->current_action_mode = UPH_ACTION_SELECT;

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_draw"),
				leaf_id("uph_song_timeline_draw"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_NONE,
				data->current_action_mode == UPH_ACTION_DRAW
			)) data->current_action_mode = UPH_ACTION_DRAW;

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_cut"),
				leaf_id("uph_song_timeline_cut"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				data->current_action_mode == UPH_ACTION_CUT
			)) data->current_action_mode = UPH_ACTION_CUT;
		}
		leaf({.size = {LEAF_SIZE_FIXED(NAUI_DPI(1)), LEAF_SIZE_PERCENT(0.5f)}, .color = {line_color}});
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
			}) uph_ui_dropdown(snap_options, 5, (uint32_t*)&data->snap_resolution, leaf_id("uph_song_timeline_snap"));
		}

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			leaf_text("BPM: ", {.font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")), .color = naui_theme_color("uph_ui_text_color")});
			leaf({
				.size = { LEAF_SIZE_FIXED(NAUI_DPI(56)), LEAF_SIZE_FULL },
				.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
			})
			{
				const Naui_String bpm = naui_string_format("BPM: %.1f", uph_state.project.bpm);
				uph_ui_drag_float(&uph_state.project.bpm, leaf_id("uph_bpm_drag"), 1.0f, 1.0f, 10000.0f, "%.2f", UPH_UI_DRAG_CLAMPED);
			}
		}
	}
}
