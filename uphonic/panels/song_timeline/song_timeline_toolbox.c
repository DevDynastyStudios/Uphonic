static void uph__song_timeline_toggle_recording(void)
{
	uph_state.shared.song_timeline_recording = !uph_state.shared.song_timeline_recording;
}

void uph_song_timeline_update_record_track(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_SharedState *shared = &uph_state.shared;
	const bool rolling = shared->song_timeline_recording && shared->song_timeline_playing;
	const bool started = rolling && !state->record_rolling;
	state->record_rolling = rolling;

	if (!started || uph_track_can_record(shared->record_track))
		return;

	const Uph_ActionTrackRef track_ref = uph_action_track_ref_top((uint32_t)naui_list_len(uph_state.project.tracks));
	naui_action_execute_stack(UPH_ACTION_TRACK_CREATE, (Uph_ActionTrackCreate){0});

	Uph_Track *track = uph_action_track_resolve(track_ref);
	uph__song_timeline_select_track(track);
	shared->record_track = track;
}

void uph_song_timeline_render_toolbox(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_TOOLBOX_HEIGHT)) },
		.padding = { .top = 1, .bottom = 1 },
		.color = uph_state.colors.song_timeline_toolbox_border
	})
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_TOOLBOX_PADDING_X), 0.0f),
		.color = uph_state.colors.song_timeline_toolbox_bg,
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(UPH_TOOLBOX_GROUP_GAP),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		const Naui_Color icon_color = uph_state.colors.song_timeline_tool_icon;

		leaf({
			.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
			.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			if (uph_ui_toolbox_button_ex(
				uph_state.shared.song_timeline_playing ? uph_state.icons.pause : uph_state.icons.play,
				leaf_id("uph_song_timeline_play"),
				uph_state.shared.song_timeline_playing ? uph_state.colors.song_timeline_pause_icon : uph_state.colors.song_timeline_play_icon,
				NAUI_CORNER_TL | NAUI_CORNER_BL
			))
			{
				uph_state.shared.song_timeline_playing = !uph_state.shared.song_timeline_playing;
			}

			if (uph_ui_toolbox_button_ex(
				uph_state.icons.stop,
				leaf_id("uph_song_timeline_stop"),
				uph_state.colors.song_timeline_stop_icon,
				NAUI_CORNER_NONE
			))
			{
				uph_state.shared.song_timeline_playing = false;
				uph_state.shared.song_timeline_playhead_position = 0.0;
			}

			const bool recording = uph_state.shared.song_timeline_recording;
			if (uph_ui_toolbox_toggle_button_ex(
				uph_state.icons.mic,
				leaf_id("uph_song_timeline_record"),
				recording ? uph_state.colors.record_armed : icon_color,
				NAUI_CORNER_NONE,
				recording
			)) uph__song_timeline_toggle_recording();

			if (uph_ui_toolbox_button_ex(
				uph_state.icons.reset,
				leaf_id("uph_song_timeline_reset"),
				icon_color,
				NAUI_CORNER_NONE
			))
			{
				uph_state.shared.song_timeline_playhead_position = 0.0;
				state->follow.anchored = false;
			}

			if (uph_ui_toolbox_toggle_button_ex(
				uph_state.icons.loop,
				leaf_id("uph_song_timeline_loop"),
				icon_color,
				NAUI_CORNER_NONE,
				uph_state.project.loop_enabled
			)) uph_state.project.loop_enabled = !uph_state.project.loop_enabled;

			if (uph_ui_toolbox_toggle_button_ex(
				uph_state.icons.playhead,
				leaf_id("uph_song_timeline_follow_playhead"),
				icon_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				state->follow_playhead
			)) state->follow_playhead = !state->follow_playhead;
		}
		leaf({
			.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			if (uph_ui_toolbox_toggle_button_ex(
				uph_state.icons.select,
				leaf_id("uph_song_timeline_select"),
				icon_color,
				NAUI_CORNER_TL | NAUI_CORNER_BL,
				state->current_action_mode == UPH_ACTION_SELECT
			)) state->current_action_mode = UPH_ACTION_SELECT;

			if (uph_ui_toolbox_toggle_button_ex(
				uph_state.icons.draw,
				leaf_id("uph_song_timeline_draw"),
				icon_color,
				NAUI_CORNER_NONE,
				state->current_action_mode == UPH_ACTION_DRAW
			)) state->current_action_mode = UPH_ACTION_DRAW;

			if (uph_ui_toolbox_toggle_button_ex(
				uph_state.icons.cut,
				leaf_id("uph_song_timeline_cut"),
				icon_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				state->current_action_mode == UPH_ACTION_CUT
			)) state->current_action_mode = UPH_ACTION_CUT;
		}
		leaf({
			.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
			.child_alignment = { .x = LEAF_ALIGN_Y_CENTER, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(UPH_TOOLBOX_LABEL_GAP),
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			leaf_text("Snap:", {
				.color = uph_state.colors.ui_text,
				.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_TOOLBOX_FONT_SIZE))
			});
			leaf({
				.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(64)), .height = LEAF_SIZE_FULL },
				.child_alignment = { .x = LEAF_ALIGN_Y_CENTER, .y = LEAF_ALIGN_Y_CENTER }
			}) uph_ui_snap_dropdown(&state->snap_resolution, leaf_id("uph_song_timeline_snap"));
		}

		leaf({
			.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(8),
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			const Leaf_ID bpm_id = leaf_id("uph_bpm_drag");
			const bool tempo_live = uph_tempo_lane_active() && uph_state.shared.song_timeline_playing;
			float shown_bpm = tempo_live ? uph_tempo_current_bpm() : uph_state.project.bpm;
			if (uph_ui_bpm(&shown_bpm, "uph_bpm_drag", 1.0f, UPH_TEMPO_MAX_BPM) && !tempo_live)
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
