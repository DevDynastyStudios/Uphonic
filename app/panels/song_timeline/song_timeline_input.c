void uph_song_timeline_update_input(void)
{
	const float wheel_y = (float)naui_mouse_scroll_delta();
	const bool ctrl_held = naui_key_down(NAUI_KEY_LCONTROL);
	const float max_scroll_y = uph_song_timeline_max_scroll_y();

	if (uph_song_timeline_data.panel_hovered)
	{
		if (ctrl_held && wheel_y != 0.0f)
		{
			const float old_zoom_x = uph_song_timeline_data.zoom.x;
			const double mouse_beat_before = ((double)naui_mouse_x() + uph_song_timeline_data.scroll.x) / old_zoom_x;
			float new_zoom_x = old_zoom_x * (1.0f + wheel_y * UPH_SONG_TIMELINE_ZOOM_SPEED);

			new_zoom_x = NAUI_CLAMP(new_zoom_x, UPH_SONG_TIMELINE_ZOOM_X_MIN, UPH_SONG_TIMELINE_ZOOM_X_MAX);
			uph_song_timeline_data.zoom.x = new_zoom_x;
			uph_song_timeline_data.scroll.x = (float)(mouse_beat_before * new_zoom_x) - (float)naui_mouse_x();
			uph_song_timeline_data.scroll.x = fmaxf(0.0f, uph_song_timeline_data.scroll.x);
		}
		else if (wheel_y != 0.0f)
		{
			uph_song_timeline_data.scroll.y -= wheel_y * UPH_SONG_TIMELINE_SCROLL_Y_SPEED;
			uph_song_timeline_data.scroll.y = NAUI_CLAMP(uph_song_timeline_data.scroll.y, 0.0f, max_scroll_y);
		}
	}

	static Naui_Vec2 pan_last_mouse;
	static bool panning = false;
	if (naui_panel_hovered(uph_state.panels.song_timeline) && naui_mouse_pressed(NAUI_MOUSE_MIDDLE))
	{
		panning = true;
		pan_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
	}

	if (panning)
	{
		Naui_Vec2 current = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
		Naui_Vec2 delta = (Naui_Vec2) { current.x - pan_last_mouse.x, current.y - pan_last_mouse.y };

		uph_song_timeline_data.scroll.x -= delta.x * UPH_SONG_TIMELINE_PAN_SPEED;
		uph_song_timeline_data.scroll.x = fmaxf(0.0f, uph_song_timeline_data.scroll.x);

		uph_song_timeline_data.scroll.y -= delta.y * UPH_SONG_TIMELINE_PAN_SPEED;
		uph_song_timeline_data.scroll.y = NAUI_CLAMP(uph_song_timeline_data.scroll.y, 0.0f, max_scroll_y);
		pan_last_mouse = current;

		if (naui_mouse_released(NAUI_MOUSE_MIDDLE))
			panning = false;
	}
}

void uph_song_timeline_update_selection_keys(void)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;

	if (!data->panel_focused || data->disable_space_to_play || data->drag.active || data->marquee.active || data->automation_edit.dragging_point_index >= 0)
		return;

	const bool ctrl = uph_ui_ctrl_down();
	if (naui_key_pressed(NAUI_KEY_DELETE))
		uph_block_delete_selected();

	else if (ctrl && naui_key_pressed(NAUI_KEY_A))
		uph_block_select_all(true);

	else if (ctrl && naui_key_pressed(NAUI_KEY_D))
		uph_block_duplicate_selected();

	else if (naui_key_pressed(NAUI_KEY_ESCAPE))
		uph_block_select_all(false);
}
