NAUI_PANEL(uph_song_timeline)

void uph_song_timeline_invalidate_tracks(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	naui_list_free(state->drag.group_blocks);
	state->drag.group_blocks = NULL;
	state->drag.active = false;
	state->drag.creating = false;
	state->drag.group = false;
	state->drag.track = NULL;
	state->drag.mode = UPH_BLOCK_INTERACTION_NONE;

	state->automation_edit.dragging_point_index = -1;
	state->automation_edit.creating = false;
	state->automation_edit.track = NULL;
	state->automation_edit.hovered_point_index = -1;

	state->hovered_block.active = false;
	state->hovered_block.track = NULL;
	state->block_menu = (Uph_BlockMenuState){ 0 };
	state->current_options_track = NULL;
	state->current_color_options_track = NULL;
	state->current_hovered_track = NULL;
	state->frame_hovered_track = NULL;
	state->hovered_track = NULL;
	state->bottom_hovered = false;
	state->pending_automation.active = false;
	state->marquee.active = false;
	state->track_drag = (Uph_TrackDragState){ 0 };
	naui_list_clear(state->selected_tracks);
	state->selection_anchor = NULL;

	if (state->rename_track)
	{
		state->rename_track = NULL;
		state->disable_space_to_play = false;
	}
}

void uph_song_timeline_reset_scroll(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	state->scroll = (Naui_Vec2) { 0.0f, 0.0f };
}

#pragma region Panel Events
static void uph_song_timeline_on_attach(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;

	state->scroll = (Naui_Vec2) { 0.0f, 0.0f };
	state->zoom = (Naui_Vec2) { NAUI_DPI(64.0f), 124.0f };
	state->snap_resolution = UPH_SNAP_LINE;
	state->current_action_mode = UPH_ACTION_DRAW;
	state->automation_edit.dragging_point_index = -1;
}

static void uph_song_timeline_on_detach(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	uph_particles_free(&state->particles);
}

static void uph_song_timeline_on_open(void)
{
	
}

static void uph_song_timeline_on_close(void)
{
	
}



static void uph__song_timeline_follow_playhead(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	if (!state->follow_playhead || !uph_state.shared.song_timeline_playing)
	{
		state->follow.anchored = false;
		return;
	}

	const float view_width = state->panel_bounding_box.width - NAUI_DPI(uph_song_timeline_header_column_width());
	if (view_width <= 0.0f || state->minimap.dragging)
		return;

	const float playhead_x = (float)(uph_state.shared.song_timeline_playhead_position * state->zoom.x);
	if (!state->follow.anchored)
	{
		state->follow.anchor_x = NAUI_CLAMP(playhead_x - state->scroll.x, 0.0f, view_width);
		state->follow.anchored = true;
	}

	state->scroll.x = fmaxf(0.0f, playhead_x - state->follow.anchor_x);
}

static void uph_song_timeline_on_update(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const Leaf_ID track_section_id = leaf_id("uph_song_timeline_section");
	const Leaf_ID content_area_id = leaf_id("uph_song_timeline_content");

	state->content_height = leaf_get_bounding_box(content_area_id).height;
	state->panel_bounding_box = leaf_get_bounding_box(track_section_id);
	state->panel_focused = naui_panel_focused() == naui_panel_current();
	state->panel_hovered = naui_panel_is_hovered(naui_panel_current());
	state->tracks_hovered = leaf_hovered(track_section_id) && state->panel_hovered;
	if (state->tracks_hovered && naui_mouse_x() >= state->panel_bounding_box.x + NAUI_DPI(uph_song_timeline_header_column_width()))
		naui_set_cursor(uph_action_mode_cursor(uph_song_timeline_tool()));
	state->hovered_track = state->frame_hovered_track;
	state->frame_hovered_track = NULL;
	state->bottom_drop_y = state->panel_bounding_box.y - state->scroll.y + state->content_height;
	{
		const float mouse_x = (float)naui_mouse_x();
		const float mouse_y = (float)naui_mouse_y();
		const Leaf_BoundingBox section = state->panel_bounding_box;
		state->bottom_hovered = state->panel_hovered &&
			mouse_y >= fmaxf(state->bottom_drop_y, section.y) && mouse_y < section.y + section.height &&
			mouse_x >= section.x && mouse_x < section.x + section.width;
	}
	if (state->automation_edit.dragging_point_index < 0)
	{
		state->automation_edit.block_index = -1;
		state->automation_edit.track = NULL;
	}
	
	state->automation_edit.active = false;
	state->automation_edit.hovered_point_index = -1;
	state->hovered_block.active = false;
	const Leaf_BoundingBox title_probe = leaf_get_bounding_box(leaf_id("uph_song_timeline_block_menu_title"));
	if (title_probe.height > 0.0f)
		state->block_menu.measured_title_height = title_probe.height;
	state->block_menu.title_probe_used = false;
	state->block_menu.hover = state->block_menu.next_hover;
	state->block_menu.next_hover = (Uph_BlockMenuHover){ 0 };
	state->track_header_hovered = false;
	state->visual.row_counter = 0;
	state->visual.row_y = 0.0f;
	state->visual.settled_row_y = 0.0f;
	state->max_track_depth = 0;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
	{
		const uint32_t depth = uph_song_timeline_track_depth(&uph_state.project.tracks[i]);
		if (depth > state->max_track_depth)
			state->max_track_depth = depth;
	}

	if (state->marquee.active && !naui_mouse_down(NAUI_MOUSE_LEFT))
		state->marquee.active = false;

	if (state->drag.active && !naui_mouse_down(NAUI_MOUSE_LEFT))
		uph_song_timeline_finish_block_drag();

	if (naui_panel_focused() == uph_state.panels.song_timeline && naui_keybind_pressed(uph_state.settings.keybinds.play_pause) && !state->disable_space_to_play)
		uph_state.shared.song_timeline_playing = !uph_state.shared.song_timeline_playing;
	
	uph_song_timeline_update_minimap();
	uph_song_timeline_update_input();
	uph__song_timeline_follow_playhead();
	state->visual.snap_track_motion = state->visual.zoom_y != state->zoom.y;
	state->visual.zoom_y = state->zoom.y;
	uph_song_timeline_update_selection_keys();
	uph_song_timeline_update_drag_track_switch();
	uph_song_timeline_update_bottom_drop();
	uph_song_timeline_render_toolbox();

	Uph_UIMenuID track_options_context_menu = uph_ui_context_menu(leaf_id("uph_song_timeline_track_options_menu"));
	Uph_UIMenuID track_color_options_context_menu = uph_ui_context_menu_custom(leaf_id("uph_song_timeline_track_color_menu"), uph_song_timeline_render_track_color_menu);
	Uph_UIMenuID block_options_context_menu = uph_ui_context_menu(leaf_id("uph_song_timeline_block_options_menu"));
	if (state->block_menu.open_requested)
	{
		state->block_menu.open_requested = false;
		uph_ui_open_context_menu(block_options_context_menu);
	}

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{
		uph_song_timeline_render_minimap();
		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
			.direction = LEAF_DIRECTION_VERTICAL
		})
		{
			uph_song_timeline_render_top_bar();
			leaf({
				.id = track_section_id,
				.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
				.child_offset = { .x = 0.0f, .y = state->scroll.y },
				.clip_children = true
			})
			{
				leaf({
					.id = content_area_id,
					.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT }
				})
				{
					Uph_Track *master = &uph_state.project.tracks[0];
					if (naui_list_len(master->subtracks) > 0)
					{
						uph_song_timeline_render_track(master, 0, track_options_context_menu, track_color_options_context_menu);
						state->visual.row_counter++;
					}

					for (uint32_t i = 1; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
					{
						uph_song_timeline_render_track(&uph_state.project.tracks[i], 0, track_options_context_menu, track_color_options_context_menu);
						state->visual.row_counter++;
					}
				}
				uph_song_timeline_render_track_plus();
			}
			leaf({
				.custom_draw = uph_song_timeline_render_bottom_drop_overlay,
				.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
				.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
			});
			leaf({
				.custom_draw = uph_song_timeline_render_playhead_overlay,
				.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
				.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
			});
			uph_song_timeline_update_track_drag();
		}
		uph_song_timeline_render_time_display();
	}

	uph_song_timeline_update_empty_space_marquee();
	uph_song_timeline_render_track_options_menu(track_options_context_menu, track_color_options_context_menu);
	uph_song_timeline_render_block_options_menu(block_options_context_menu);

	uph_particles_render(&state->particles);
}
#pragma endregion