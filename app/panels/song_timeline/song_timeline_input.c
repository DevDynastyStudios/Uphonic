#define UPH_SONG_TIMELINE_ZOOM_X_MIN 8.0f
#define UPH_SONG_TIMELINE_ZOOM_X_MAX 256.0f
#define UPH_SONG_TIMELINE_PAN_SPEED 1.0f
#define UPH_SONG_TIMELINE_SCROLL_Y_SPEED 40.0f
#define UPH_SONG_TIMELINE_ZOOM_SPEED 0.1f
#define UPH_SONG_TIMELINE_ZOOM_Y_MIN 56.0f
#define UPH_SONG_TIMELINE_ZOOM_Y_MAX 320.0f
#define UPH_SONG_TIMELINE_ZOOM_Y_DRAG_SPEED 0.008f

typedef struct
{
	float target_y;
	float old_row;
	float new_row;
	float old_y;
	float new_y;
	float result_y;
	bool found;
}
Uph_SongTimelineRowRescale;

static void uph__song_timeline_rescale_track_rows(const Uph_Track *track, Uph_SongTimelineRowRescale *rescale)
{
	const bool folder = uph_track_is_folder(track);
	const float folder_height = NAUI_DPI(UPH_SONG_TIMELINE_FOLDER_ROW_HEIGHT);
	const float old_height = folder ? folder_height : rescale->old_row;
	const float new_height = folder ? folder_height : rescale->new_row;

	if (!rescale->found && rescale->target_y < rescale->old_y + old_height)
	{
		rescale->result_y = rescale->new_y + (rescale->target_y - rescale->old_y) * (new_height / old_height);
		rescale->found = true;
	}

	rescale->old_y += old_height;
	rescale->new_y += new_height;

	if (folder && (track->state & UPH_TRACK_TIMELINE_COLLAPSED))
		return;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
		uph__song_timeline_rescale_track_rows(&track->subtracks[i], rescale);
}

static Uph_SongTimelineRowRescale uph__song_timeline_rescale_rows(float target_y, float old_row, float new_row)
{
	Uph_SongTimelineRowRescale rescale = { .target_y = target_y, .old_row = old_row, .new_row = new_row };
	const Uph_Track *master = &uph_state.project.tracks[0];
	if (naui_list_len(master->subtracks) > 0)
		uph__song_timeline_rescale_track_rows(master, &rescale);

	for (uint32_t i = 1; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
		uph__song_timeline_rescale_track_rows(&uph_state.project.tracks[i], &rescale);

	if (!rescale.found)
		rescale.result_y = rescale.new_y + (target_y - rescale.old_y);

	return rescale;
}

static float uph__song_timeline_content_origin_x(void)
{
	return uph__song_timeline_state.panel_bounding_box.x + NAUI_DPI(uph_song_timeline_header_column_width());
}

void uph_song_timeline_update_input(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const float wheel_y = (float)naui_mouse_scroll_delta();
	const bool ctrl_held = naui_key_down(NAUI_KEY_LCONTROL);
	const float max_scroll_y = uph_song_timeline_max_scroll_y();

	if (state->panel_hovered)
	{
		if (ctrl_held && wheel_y != 0.0f)
		{
			const float old_zoom_x = state->zoom.x;
			const float mouse_local_x = (float)naui_mouse_x() - uph__song_timeline_content_origin_x();
			const double mouse_beat_before = ((double)mouse_local_x + state->scroll.x) / old_zoom_x;
			float new_zoom_x = old_zoom_x * (1.0f + wheel_y * UPH_SONG_TIMELINE_ZOOM_SPEED);

			new_zoom_x = NAUI_CLAMP(new_zoom_x, UPH_SONG_TIMELINE_ZOOM_X_MIN, UPH_SONG_TIMELINE_ZOOM_X_MAX);
			state->zoom.x = new_zoom_x;
			state->scroll.x = (float)(mouse_beat_before * new_zoom_x) - mouse_local_x;
			state->scroll.x = fmaxf(0.0f, state->scroll.x);
		}
		else if (wheel_y != 0.0f && state->minimap.hovered)
		{
			state->scroll.x -= wheel_y * UPH_SONG_TIMELINE_SCROLL_Y_SPEED;
			state->scroll.x = fmaxf(0.0f, state->scroll.x);
		}
		else if (wheel_y != 0.0f)
		{
			state->scroll.y -= wheel_y * UPH_SONG_TIMELINE_SCROLL_Y_SPEED;
			state->scroll.y = NAUI_CLAMP(state->scroll.y, 0.0f, max_scroll_y);
		}
	}

	static Naui_Vec2 pan_last_mouse;
	static bool panning = false;
	if (naui_panel_is_hovered(uph_state.panels.song_timeline) && naui_mouse_pressed(NAUI_MOUSE_MIDDLE))
	{
		if (uph_ui_ctrl_down())
		{
			state->zoom_drag.active = true;
			state->zoom_drag.last_mouse_y = (float)naui_mouse_y();
			state->zoom_drag.anchor_local_y = (float)naui_mouse_y() - state->panel_bounding_box.y;
			state->zoom_drag.unsnapped_zoom_y = state->zoom.y;
		}
		else
		{
			panning = true;
			pan_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
		}
	}

	if (state->zoom_drag.active)
	{
		const float mouse_y = (float)naui_mouse_y();
		const float delta_y = mouse_y - state->zoom_drag.last_mouse_y;
		state->zoom_drag.last_mouse_y = mouse_y;
		naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

		state->zoom_drag.unsnapped_zoom_y = NAUI_CLAMP(state->zoom_drag.unsnapped_zoom_y * expf(-delta_y * UPH_SONG_TIMELINE_ZOOM_Y_DRAG_SPEED), UPH_SONG_TIMELINE_ZOOM_Y_MIN, UPH_SONG_TIMELINE_ZOOM_Y_MAX);

		const float dpi_scale = naui_app_dpi_scale();
		const int32_t old_row_px = NAUI_DPI(state->zoom.y);
		const int32_t new_row_px = NAUI_DPI(state->zoom_drag.unsnapped_zoom_y);
		if (new_row_px != old_row_px && old_row_px > 0)
		{
			const float anchor_content_y = state->zoom_drag.anchor_local_y + state->scroll.y;
			const Uph_SongTimelineRowRescale rescale = uph__song_timeline_rescale_rows(anchor_content_y, (float)old_row_px, (float)new_row_px);
			const float scaled_content_height = state->content_height - rescale.old_y + rescale.new_y;
			const float scaled_max_scroll_y = fmaxf(0.0f, scaled_content_height - (float)new_row_px / dpi_scale);
			state->zoom.y = (float)new_row_px / dpi_scale;
			state->scroll.y = NAUI_CLAMP(roundf(rescale.result_y - state->zoom_drag.anchor_local_y), 0.0f, scaled_max_scroll_y);
		}

		if (naui_mouse_released(NAUI_MOUSE_MIDDLE) || !naui_mouse_down(NAUI_MOUSE_MIDDLE))
			state->zoom_drag.active = false;
	}

	if (panning)
	{
		Naui_Vec2 current = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
		Naui_Vec2 delta = (Naui_Vec2) { current.x - pan_last_mouse.x, current.y - pan_last_mouse.y };

		state->scroll.x -= delta.x * UPH_SONG_TIMELINE_PAN_SPEED;
		state->scroll.x = fmaxf(0.0f, state->scroll.x);

		state->scroll.y -= delta.y * UPH_SONG_TIMELINE_PAN_SPEED;
		state->scroll.y = NAUI_CLAMP(state->scroll.y, 0.0f, max_scroll_y);
		pan_last_mouse = current;

		if (naui_mouse_released(NAUI_MOUSE_MIDDLE))
			panning = false;
	}
}

static bool uph__song_timeline_row_move_fits(Naui_List(Uph_TrackPtr) rows, const Naui_List(Uph_ActionBlockSnapshot) selected, Naui_List(Uph_TrackPtr) *targets, int32_t offset)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		const Uph_Track *source = uph_action_track_resolve(selected[i].track);
		const int32_t source_row = source ? uph__song_timeline_row_of(rows, source) : -1;
		const int32_t row = source_row + offset;
		if (source_row < 0 || row < 0 || row >= (int32_t)naui_list_len(rows))
			return false;

		Uph_Track *target = rows[row];
		const Uph_ResourceType type = selected[i].block.type;
		if (!uph__block_paste_fits(target, type) || uph__block_is_recording(source, selected[i].index))
			return false;

		for (uint32_t j = 0; j < i; j++)
		{
			if ((*targets)[j] == target && selected[j].block.type != type)
				return false;
		}

		naui_list_push(*targets, target);
	}

	return true;
}

static void uph__song_timeline_move_selected_rows(int32_t offset)
{
	Naui_List(Uph_ActionBlockSnapshot) selected = NULL;
	Naui_List(Uph_TrackPtr) rows = NULL;
	Naui_List(Uph_TrackPtr) targets = NULL;
	uph_block_collect_all(UPH_RESOURCE_NONE, true, false, 0, &selected);
	uph__song_timeline_collect_rows(uph_state.project.tracks, &rows);

	if (naui_list_len(selected) > 0 && uph__song_timeline_row_move_fits(rows, selected, &targets, offset))
	{
		naui_action_group_start(UPH_ACTION_BLOCKS_MOVE_GROUP);
		for (uint32_t i = (uint32_t)naui_list_len(selected); i > 0; i--)
		{
			const Uph_TimelineBlock *block = &selected[i - 1].block;
			Uph_ActionBlockTransform data = {
				.src_track = selected[i - 1].track, .src_index = selected[i - 1].index,
				.dst_track = uph_action_track_ref(targets[i - 1]), .dst_index = (uint32_t)naui_list_len(targets[i - 1]->blocks),
				.old_start = block->start_beat, .old_length = block->length_beats, .old_offset = block->start_offset_beats,
				.new_start = block->start_beat, .new_length = block->length_beats, .new_offset = block->start_offset_beats
			};
			naui_action_execute_stack(UPH_ACTION_BLOCK_MOVE, data);
		}

		naui_action_group_end();
	}

	naui_list_free(targets);
	naui_list_free(rows);
	naui_list_free(selected);
}

static void uph__song_timeline_update_move_keys(void)
{
	const Uph_SongTimelineState *state = &uph__song_timeline_state;
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;
	const double step = uph_snap_division(state->snap_resolution, state->zoom.x);

	if (naui_keybind_pressed_repeat(keybinds->move_left))
		uph_block_shift_selected(-step);

	else if (naui_keybind_pressed_repeat(keybinds->move_right))
		uph_block_shift_selected(step);

	else if (naui_keybind_pressed_repeat(keybinds->move_up))
		uph__song_timeline_move_selected_rows(-1);

	else if (naui_keybind_pressed_repeat(keybinds->move_down))
		uph__song_timeline_move_selected_rows(1);
}

static void uph__song_timeline_update_clipboard_keys(void)
{
	const Uph_SongTimelineState *state = &uph__song_timeline_state;
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;

	if (naui_keybind_pressed(keybinds->copy))
		uph_block_copy_selected();

	else if (naui_keybind_pressed(keybinds->cut) && uph_block_copy_selected())
		uph_block_delete_selected();

	else if (naui_keybind_pressed(keybinds->paste))
		uph_block_paste(fmax(0.0, uph_snap_beat_round(uph_state.shared.song_timeline_playhead_position, state->snap_resolution, state->zoom.x)));
}

static void uph__song_timeline_update_tool_keys(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;

	if (naui_keybind_pressed(keybinds->tool_select))
		state->current_action_mode = UPH_ACTION_SELECT;

	else if (naui_keybind_pressed(keybinds->tool_draw))
		state->current_action_mode = UPH_ACTION_DRAW;

	else if (naui_keybind_pressed(keybinds->tool_cut))
		state->current_action_mode = UPH_ACTION_CUT;
}

void uph_song_timeline_update_selection_keys(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;

	if (naui_panel_focused() != uph_state.panels.song_timeline || state->disable_space_to_play || state->drag.active || state->marquee.active || state->automation_edit.dragging_point_index >= 0)
		return;

	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;
	if (naui_keybind_pressed(keybinds->delete_selected))
		uph_block_delete_selected();

	else if (naui_keybind_pressed(keybinds->select_all))
		uph_block_select_all(true);

	else if (naui_keybind_pressed(keybinds->duplicate))
		uph_block_duplicate_selected();

	else if (naui_keybind_pressed(keybinds->deselect))
		uph_block_select_all(false);

	else if (naui_keybind_pressed(keybinds->invert_selection))
		uph_block_invert_selection();

	uph__song_timeline_update_move_keys();
	uph__song_timeline_update_clipboard_keys();
	uph__song_timeline_update_tool_keys();
}

static bool uph__song_timeline_group_row_hovered(Naui_List(Uph_Track) tracks, float mouse_x, float mouse_y)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		const Uph_Track *track = &tracks[i];
		if (!uph_track_is_folder(track))
			continue;

		const Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed("uph_song_timeline_row", (uint64_t)track));
		if (mouse_x >= box.x && mouse_x < box.x + box.width && mouse_y >= box.y && mouse_y < box.y + box.height)
			return true;

		if (!(track->state & UPH_TRACK_TIMELINE_COLLAPSED) && uph__song_timeline_group_row_hovered(track->subtracks, mouse_x, mouse_y))
			return true;
	}
	return false;
}

void uph_song_timeline_update_empty_space_marquee(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;

	if (!state->tracks_hovered || state->marquee.active || state->drag.active || state->automation_edit.active)
		return;

	if (!naui_mouse_pressed(NAUI_MOUSE_LEFT) || uph_ui_any_widget_hovered())
		return;

	const float origin_x = uph__song_timeline_content_origin_x();
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();
	const float rows_bottom = state->panel_bounding_box.y - state->scroll.y + state->content_height;
	const bool below_rows = mouse_y >= rows_bottom;
	const bool on_group_lane = mouse_x >= origin_x && uph__song_timeline_group_row_hovered(uph_state.project.tracks, mouse_x, mouse_y);
	if (!below_rows && !on_group_lane)
		return;

	if (uph_song_timeline_tool() != UPH_ACTION_SELECT)
	{
		uph_block_select_all(false);
		return;
	}

	state->marquee.active = true;
	state->marquee.start_beat = ((double)naui_mouse_x() - origin_x + state->scroll.x) / state->zoom.x;
	state->marquee.start_content_y = (float)naui_mouse_y() - state->panel_bounding_box.y + state->scroll.y;
	uph_block_select_all(false);
}