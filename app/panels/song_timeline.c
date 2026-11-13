NAUI_PANEL(uph_song_timeline)

void uph_song_timeline_invalidate_tracks(void)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	naui_list_free(data->drag.group_blocks);
	data->drag.group_blocks = NULL;
	data->drag.active = false;
	data->drag.creating = false;
	data->drag.group = false;
	data->drag.track = NULL;
	data->drag.mode = UPH_BLOCK_INTERACTION_NONE;

	data->automation_edit.dragging_point_index = -1;
	data->automation_edit.creating = false;
	data->automation_edit.track = NULL;

	data->hovered_block.active = false;
	data->hovered_block.track = NULL;
	data->block_menu = (Uph_BlockMenuState){ 0 };
	data->current_options_track = NULL;
	data->current_color_options_track = NULL;
	data->current_hovered_track = NULL;
	data->frame_hovered_track = NULL;
	data->hovered_track = NULL;
	data->bottom_hovered = false;
	data->pending_automation.active = false;
	data->marquee.active = false;
	data->track_drag = (Uph_TrackDragState){ 0 };
	naui_list_clear(data->selected_tracks);
	data->selection_anchor = NULL;

	if (data->rename_track)
	{
		data->rename_track = NULL;
		data->disable_space_to_play = false;
	}
}

void uph_song_timeline_reset_scroll(void)
{
	uph_song_timeline_data.scroll = (Naui_Vec2) { 0.0f, 0.0f };
}

#pragma region Panel Events
static void uph_song_timeline_on_attach(void)
{
	Naui_PanelID panel = naui_panel_current();

	uph_song_timeline_data.scroll = (Naui_Vec2) { 0.0f, 0.0f };
	uph_song_timeline_data.zoom = (Naui_Vec2) { NAUI_DPI(64.0f), 104.0f };
	uph_song_timeline_data.snap_resolution = UPH_SNAP_LINE;
	uph_song_timeline_data.current_action_mode = UPH_ACTION_DRAW;
	uph_song_timeline_data.automation_edit.dragging_point_index = -1;
}

static void uph_song_timeline_on_detach(void)
{

}

static void uph_song_timeline_on_open(void)
{
	
}

static void uph_song_timeline_on_close(void)
{
	
}



static void uph_song_timeline_on_update(void)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	naui_panel_set_title(naui_panel_current(), NAUI_TR("song_timeline.title"));
	const Leaf_ID track_section_id = leaf_id("uph_song_timeline_section");
	const Leaf_ID content_area_id = leaf_id("uph_song_timeline_content");

	data->content_height = leaf_get_bounding_box(content_area_id).height;
	data->panel_bounding_box = leaf_get_bounding_box(track_section_id);
	data->panel_focused = naui_panel_focused() == naui_panel_current();
	data->panel_hovered = naui_panel_is_hovered(naui_panel_current());
	data->tracks_hovered = leaf_hovered(track_section_id) && data->panel_hovered;
	data->hovered_track = data->frame_hovered_track;
	data->frame_hovered_track = NULL;
	data->bottom_drop_y = data->panel_bounding_box.y - data->scroll.y + data->content_height;
	{
		const float mouse_x = (float)naui_mouse_x();
		const float mouse_y = (float)naui_mouse_y();
		const Leaf_BoundingBox section = data->panel_bounding_box;
		data->bottom_hovered = data->panel_hovered &&
			mouse_y >= fmaxf(data->bottom_drop_y, section.y) && mouse_y < section.y + section.height &&
			mouse_x >= section.x && mouse_x < section.x + section.width;
	}
	if (data->automation_edit.dragging_point_index < 0)
	{
		data->automation_edit.block_index = -1;
		data->automation_edit.track = NULL;
	}
	
	data->automation_edit.active = false;
	data->hovered_block.active = false;
	const Leaf_BoundingBox title_probe = leaf_get_bounding_box(leaf_id("uph_song_timeline_block_menu_title"));
	if (title_probe.height > 0.0f)
		data->block_menu.measured_title_height = title_probe.height;
	data->block_menu.title_probe_used = false;
	data->block_menu.hover = data->block_menu.next_hover;
	data->block_menu.next_hover = (Uph_BlockMenuHover){ 0 };
	data->track_header_hovered = false;
	data->visual.row_counter = 0;
	data->visual.row_y = 0.0f;
	data->visual.settled_row_y = 0.0f;
	data->visual.snap_track_motion = data->visual.zoom_y != data->zoom.y;
	data->visual.zoom_y = data->zoom.y;
	data->max_track_depth = 0;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
	{
		const uint32_t depth = uph_song_timeline_track_depth(&uph_state.project.tracks[i]);
		if (depth > data->max_track_depth)
			data->max_track_depth = depth;
	}

	if (data->marquee.active && !naui_mouse_down(NAUI_MOUSE_LEFT))
		data->marquee.active = false;

	if (data->drag.active && !naui_mouse_down(NAUI_MOUSE_LEFT))
		uph_song_timeline_finish_block_drag();

	if (naui_panel_focused() == uph_state.panels.song_timeline && naui_keybind_pressed(uph_state.settings.keybinds.play_pause) && !data->disable_space_to_play)
		uph_state.shared.song_timeline_playing = !uph_state.shared.song_timeline_playing;
	
	uph_song_timeline_update_minimap();
	uph_song_timeline_update_input();
	uph_song_timeline_update_selection_keys();
	uph_song_timeline_update_drag_track_switch();
	uph_song_timeline_update_bottom_drop();
	uph_song_timeline_render_toolbox();

	Uph_UIMenuID track_options_context_menu = uph_ui_context_menu(leaf_id("uph_song_timeline_track_options_menu"));
	Uph_UIMenuID track_color_options_context_menu = uph_ui_context_menu_custom(leaf_id("uph_song_timeline_track_color_menu"), uph_song_timeline_render_track_color_menu);
	Uph_UIMenuID block_options_context_menu = uph_ui_context_menu(leaf_id("uph_song_timeline_block_options_menu"));
	if (data->block_menu.open_requested)
	{
		data->block_menu.open_requested = false;
		uph_ui_open_context_menu(block_options_context_menu);
	}

	leaf({
		.direction = LEAF_DIRECTION_VERTICAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW}
	})
	{
		uph_song_timeline_render_minimap();
		leaf({
			.direction = LEAF_DIRECTION_VERTICAL,
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW}
		})
		{
			uph_song_timeline_render_top_bar();
			leaf({
				.id = track_section_id,
				.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
				.child_offset = {0.0f, data->scroll.y},
				.clip_children = true
			})
			{
				leaf({
					.id = content_area_id,
					.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT}
				})
				{
					Uph_Track *master = &uph_state.project.tracks[0];
					if (naui_list_len(master->subtracks) > 0)
					{
						uph_song_timeline_render_track(master, 0, track_options_context_menu, track_color_options_context_menu);
						data->visual.row_counter++;
					}

					for (uint32_t i = 1; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
					{
						uph_song_timeline_render_track(&uph_state.project.tracks[i], 0, track_options_context_menu, track_color_options_context_menu);
						data->visual.row_counter++;
					}
				}
				uph_song_timeline_render_track_plus();
			}
			leaf({
				.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
				.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
				.custom_draw = uph_song_timeline_render_bottom_drop_overlay
			});
			leaf({
				.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
				.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
				.custom_draw = uph_song_timeline_render_playhead_overlay
			});
			uph_song_timeline_update_track_drag();
		}
		uph_song_timeline_render_time_display();
	}

	uph_song_timeline_update_empty_space_marquee();
	uph_song_timeline_render_track_options_menu(track_options_context_menu, track_color_options_context_menu);
	uph_song_timeline_render_block_options_menu(block_options_context_menu);
}
#pragma endregion