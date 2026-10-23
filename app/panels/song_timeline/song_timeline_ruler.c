void uph_song_timeline_render_ruler(Leaf_BoundingBox bbox, float zoom_x, float scroll_x)
{
	uph_grid_render_beats(bbox, zoom_x, scroll_x, uph_snap_division(uph_song_timeline_data.snap_resolution));
}

static void uph_song_timeline_update_playhead_drag(Leaf_BoundingBox bbox)
{
	static bool dragging_playhead;
	static double drag_beat;
	Uph_SongTimelineData *data = &uph_song_timeline_data;

	const bool mouse_over_ruler = uph_song_timeline_vec4_contains_vec2(
		(Naui_Vec4){ bbox.x, bbox.y, bbox.width, bbox.height },
		(Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() }
	);

	if (!dragging_playhead && mouse_over_ruler && naui_mouse_pressed(NAUI_MOUSE_LEFT) && naui_panel_is_hovered(uph_state.panels.song_timeline))
	{
		dragging_playhead = true;
		drag_beat = -1.0;
	}

	if (!dragging_playhead)
		return;

	const double mouse_beat_raw = ((double)naui_mouse_x() - bbox.x + data->scroll.x) / data->zoom.x;
	double beat = fmax(0.0, uph_snap_beat_floor(mouse_beat_raw, data->snap_resolution));
	if (uph_state.project.loop_enabled)
	{
		const double length = uph_audio_engine_get_song_length();
		if (length > 0.0 && beat >= length)
			beat = fmax(0.0, length - uph_snap_division(data->snap_resolution));
	}

	if (beat != drag_beat)
	{
		drag_beat = beat;
		uph_state.shared.song_timeline_playhead_position = beat;
	}

	naui_set_cursor(NAUI_CURSOR_HAND);
	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		dragging_playhead = false;
}

static void uph_song_timeline_render_top_ruler(Leaf_BoundingBox bbox, void *user_data)
{
	uph_song_timeline_update_playhead_drag(bbox);
	uph_grid_render_ruler(bbox, uph_song_timeline_data.zoom.x, uph_song_timeline_data.scroll.x, uph_snap_division(uph_song_timeline_data.snap_resolution));
}

void uph_song_timeline_render_top_bar(void)
{
	const float header_width = naui_theme_float("uph_song_timeline_header_width");
	const Naui_Vec2 header_padding = naui_theme_vec2("uph_song_timeline_header_padding");
	const uint32_t height = NAUI_DPI(24);

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(height)}
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width + uph_song_timeline_header_depth_width())), LEAF_SIZE_FULL},
			.padding = LEAF_PADDING_AXES(NAUI_DPI(header_padding.x), 0.0f),
			.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
		});

		leaf({
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL}
		})
		{
			leaf({
				.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(height - 2)},
				.custom_draw = (Leaf_CustomDrawFn)uph_song_timeline_render_top_ruler,
				.border = {
					.width = 1,
					.sides = LEAF_SIDE_ALL,
					.color = naui_theme_color("uph_timeline_top_ruler_border_color")
				},
				.color = naui_theme_color("uph_timeline_top_ruler_bg_color")
			});
		}
	}
}

void uph_song_timeline_render_playhead_overlay(Leaf_BoundingBox bbox, void *data)
{
	const Naui_Image *playhead_image = naui_asset_image("uph_icon_playhead");
	const Naui_Color color = naui_theme_color("uph_playhead_color");
	const float x_offset = NAUI_DPI(uph_song_timeline_header_column_width());
	const float playhead_size = NAUI_DPI(16.0f);
	const float playhead_half_size = playhead_size * 0.5f;
	naui_push_clip_rect(bbox.x + x_offset - NAUI_DPI(1), bbox.y, bbox.width, bbox.height);

	if (uph_song_timeline_data.current_action_mode == UPH_ACTION_CUT)
	{
		const double mouse_beat = ((double)naui_mouse_x() - (bbox.x + x_offset) + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
		const double cut_beat = uph_snap_beat_round(mouse_beat, uph_song_timeline_data.snap_resolution);
		const float x = bbox.x + x_offset + (float)(cut_beat * uph_song_timeline_data.zoom.x) - uph_song_timeline_data.scroll.x;
		naui_draw_line(
			(Naui_Vec2){ x, bbox.y + playhead_half_size },
			(Naui_Vec2){ x, bbox.y + bbox.height },
			color,
			NAUI_DPI(1.0f)
		);
	}

	const Uph_MarqueeState *marquee = &uph_song_timeline_data.marquee;
	const float x = bbox.x + x_offset + uph_state.shared.song_timeline_playhead_position * uph_song_timeline_data.zoom.x - uph_song_timeline_data.scroll.x;
	naui_draw_line(
		(Naui_Vec2){ x, bbox.y + playhead_half_size },
		(Naui_Vec2){ x, bbox.y + bbox.height },
		color,
		NAUI_DPI(1.0f)
	);

	naui_draw_image(
		playhead_image,
		(Naui_Vec2){x - playhead_half_size, bbox.y + playhead_half_size},
		(Naui_Vec2){playhead_size, playhead_size},
		color,
		0.0f,
		NAUI_CORNER_NONE
	);

	if (marquee->active)
	{
		const float corner_x = bbox.x + x_offset + (float)(marquee->start_beat * uph_song_timeline_data.zoom.x) - uph_song_timeline_data.scroll.x;
		const float corner_y = uph_song_timeline_data.panel_bounding_box.y - uph_song_timeline_data.scroll.y + marquee->start_content_y;
		const Naui_Vec2 top_left = { fminf(corner_x, (float)naui_mouse_x()), fminf(corner_y, (float)naui_mouse_y()) };
		const Naui_Vec2 size = { fabsf(corner_x - (float)naui_mouse_x()), fabsf(corner_y - (float)naui_mouse_y()) };
		naui_fill_rect(top_left, size, leaf_rgba(255, 255, 255, 30), 0.0f, NAUI_CORNER_NONE);
		naui_draw_rect(top_left, size, leaf_rgba(255, 255, 255, 160), NAUI_DPI(1.0f), 0.0f, NAUI_CORNER_NONE, NAUI_SIDE_ALL);
	}

	naui_pop_clip_rect();
}
