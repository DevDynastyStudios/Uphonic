void uph_song_timeline_render_toolbox(void)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_TOOLBOX_HEIGHT))},
		.padding = { .top = 1, .bottom = 1 },
		.color = {naui_theme_color("uph_toolbox_border_color")}
	})
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_TOOLBOX_PADDING_X), 0.0f),
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_gap = NAUI_DPI(22),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		const int32_t button_size = NAUI_DPI(UPH_TOOLBOX_ICON_SIZE);
		const Naui_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
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
				NAUI_CORNER_NONE
			))
			{
				uph_state.shared.song_timeline_playhead_position = 0.0;
			}

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_loop"),
				leaf_id("uph_song_timeline_loop"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				uph_state.project.loop_enabled
			)) uph_state.project.loop_enabled = !uph_state.project.loop_enabled;
		}
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
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
		})
		{
			leaf_text("Snap: ", {
				.font_size = NAUI_DPI(UPH_TOOLBOX_FONT_SIZE),
				.color = naui_theme_color("uph_ui_text_color")
			});
			leaf({
				.size = {LEAF_SIZE_FIXED(NAUI_DPI(64)), LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
			}) uph_ui_snap_dropdown(&data->snap_resolution, leaf_id("uph_song_timeline_snap"));
		}

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(8)
		})
		{
			const Leaf_ID bpm_id = leaf_id("uph_bpm_drag");
			const bool tempo_live = uph_tempo_lane_active() && uph_state.shared.song_timeline_playing;
			float shown_bpm = tempo_live ? uph_tempo_current_bpm() : uph_state.project.bpm;
			if (uph_ui_bpm(&shown_bpm, "uph_bpm_drag", 1.0f, 10000.0f) && !tempo_live)
				uph_state.project.bpm = shown_bpm;

			Uph_UIMenuID bpm_menu = uph_ui_context_menu(leaf_id("uph_song_timeline_bpm_menu"));
			const bool bpm_hovered = !uph_dialog_blocks_input() && naui_panel_is_hovered(naui_panel_current()) && leaf_hovered_clipped(bpm_id);
			if (bpm_hovered && naui_mouse_pressed(NAUI_MOUSE_RIGHT))
				uph_ui_open_context_menu(bpm_menu);

			Uph_Track *tempo_lane = uph_tempo_find_lane();
			if (!tempo_lane)
			{
				if (uph_ui_menu_item(bpm_menu, NAUI_TR("automation.menu.automate"), leaf_id("uph_bpm_menu_automate")))
				{
					Uph_ActionTrackAutomationCreate lane = {
						.parent = uph_action_track_ref_master(),
						.name = naui_string_from_cstr(NAUI_TR("automation.target.tempo")),
						.target = UPH_AUTOMATION_TARGET_TEMPO
					};
					uph_song_timeline_create_automation_lane(lane);
				}
			}
			else if (uph_ui_menu_item(bpm_menu, NAUI_TR("automation.menu.remove"), leaf_id("uph_bpm_menu_remove_automation")))
			{
				Naui_List(Uph_TrackPtr) targets = NULL;
				naui_list_push(targets, tempo_lane);
				uph_resources_delete_tracks(targets);
				naui_list_free(targets);
			}

			if (uph_ui_time_signature(&uph_state.project.time_signature, "uph_song_timeline_time_signature"))
				uph_action_refresh_block_defaults();
		}
	}
}
