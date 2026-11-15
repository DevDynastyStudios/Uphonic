#define UPH_SONG_TIMELINE_ZOOM_X_MIN 8.0f
#define UPH_SONG_TIMELINE_ZOOM_X_MAX 256.0f
#define UPH_SONG_TIMELINE_PAN_SPEED 1.0f
#define UPH_SONG_TIMELINE_SCROLL_Y_SPEED 40.0f
#define UPH_SONG_TIMELINE_ZOOM_SPEED 0.1f

static float _uph_song_timeline_content_origin_x(void)
{
	return _uph_song_timeline_state.panel_bounding_box.x + NAUI_DPI(uph_song_timeline_header_column_width());
}

void uph_song_timeline_update_input(void)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	const float wheel_y = (float)naui_mouse_scroll_delta();
	const bool ctrl_held = naui_key_down(NAUI_KEY_LCONTROL);
	const float max_scroll_y = uph_song_timeline_max_scroll_y();

	if (state->panel_hovered)
	{
		if (ctrl_held && wheel_y != 0.0f)
		{
			const float old_zoom_x = state->zoom.x;
			const float mouse_local_x = (float)naui_mouse_x() - _uph_song_timeline_content_origin_x();
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
		panning = true;
		pan_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
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

void uph_song_timeline_update_selection_keys(void)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;

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
}

static bool _uph_song_timeline_group_row_hovered(Naui_List(Uph_Track) tracks, float mouse_x, float mouse_y)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		const Uph_Track *track = &tracks[i];
		if (!uph_track_is_folder(track))
			continue;

		const Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed("uph_song_timeline_row", (uint64_t)track));
		if (mouse_x >= box.x && mouse_x < box.x + box.width && mouse_y >= box.y && mouse_y < box.y + box.height)
			return true;

		if (!(track->state & UPH_TRACK_TIMELINE_COLLAPSED) && _uph_song_timeline_group_row_hovered(track->subtracks, mouse_x, mouse_y))
			return true;
	}
	return false;
}

void uph_song_timeline_update_empty_space_marquee(void)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;

	if (!state->tracks_hovered || state->marquee.active || state->drag.active || state->automation_edit.active)
		return;

	if (!naui_mouse_pressed(NAUI_MOUSE_LEFT) || uph_ui_any_widget_hovered())
		return;

	const float origin_x = _uph_song_timeline_content_origin_x();
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();
	const float rows_bottom = state->panel_bounding_box.y - state->scroll.y + state->content_height;
	const bool below_rows = mouse_y >= rows_bottom;
	const bool on_group_lane = mouse_x >= origin_x && _uph_song_timeline_group_row_hovered(uph_state.project.tracks, mouse_x, mouse_y);
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