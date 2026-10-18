void uph_song_timeline_render_ruler(Leaf_BoundingBox bbox, float zoom_x, float scroll_x)
{
	const Leaf_Color beat_color = naui_theme_color("uph_timeline_grid_beat_color");
	const Leaf_Color bar_color = naui_theme_color("uph_timeline_grid_bar_color");
	const Leaf_Color sub_color = naui_theme_color("uph_timeline_grid_subbeat_color");

	const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);
	const float division_px = (float)(division * zoom_x);
	const bool draw_subdivisions = division < 1.0 && division_px >= 3.0f;
	const int64_t first_div = (int64_t)floor((double)scroll_x / division_px);
	const uint32_t div_count = (uint32_t)(bbox.width / division_px) + 2;

	for (uint32_t i = 0; i < div_count; i++)
	{
		const int64_t div_index = first_div + (int64_t)i;
		if (div_index < 0)
			continue;

		const float x = bbox.x + (float)div_index * division_px - scroll_x;
		if (x < bbox.x || x > bbox.x + bbox.width)
			continue;

		const double divs_per_beat = 1.0 / division;
		const bool is_beat = fmod((double)div_index, divs_per_beat) < 0.5;
		if (!is_beat)
		{
			if (draw_subdivisions)
				naui_draw_line((Naui_Vec2){x, bbox.y}, (Naui_Vec2){x, bbox.y + bbox.height}, sub_color, 1.0f);

			continue;
		}

		const int64_t beat_index = (int64_t)llround((double)div_index / divs_per_beat);
		const bool is_downbeat = (beat_index % 4) == 0;
		const Leaf_Color line_color = is_downbeat ? bar_color : beat_color;
		naui_draw_line(
			(Naui_Vec2){x, bbox.y},
			(Naui_Vec2){x, bbox.y + bbox.height},
			line_color,
			1.0f
		);
	}
}

static void uph_song_timeline_update_playhead_drag(Leaf_BoundingBox bbox)
{
	static bool dragging_playhead;
	Uph_SongTimelineData *data = &uph_song_timeline_data;

	const bool mouse_over_ruler = uph_song_timeline_vec4_contains_vec2(
		(Naui_Vec4){ bbox.x, bbox.y, bbox.width, bbox.height },
		(Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() }
	);

	if (!dragging_playhead && mouse_over_ruler && naui_mouse_pressed(NAUI_MOUSE_LEFT) && naui_panel_is_hovered(uph_state.panels.song_timeline))
		dragging_playhead = true;

	if (!dragging_playhead)
		return;

	const double mouse_beat_raw = ((double)naui_mouse_x() - bbox.x + data->scroll.x) / data->zoom.x;
	const double mouse_beat = fmax(0.0, mouse_beat_raw);
	if (uph_state.shared.song_timeline_playing)
	{
		const double current_beat = (double)uph_state.shared.song_timeline_playhead_position;
		const double distance = mouse_beat - current_beat;

		if (fabs(distance) > 1.0)
			uph_state.shared.song_timeline_playhead_position = (float)round(mouse_beat);
	}
	else
	{
		uph_state.shared.song_timeline_playhead_position = (float)round(mouse_beat);
	}

	naui_set_cursor(NAUI_CURSOR_HAND);
	if (naui_mouse_released(NAUI_MOUSE_LEFT))
		dragging_playhead = false;
}

static void uph_song_timeline_render_top_ruler(Leaf_BoundingBox bbox, void *user_data)
{
	const Leaf_Color beat_color = naui_theme_color("uph_timeline_top_ruler_grid_beat_color");
	const Leaf_Color bar_color = naui_theme_color("uph_timeline_top_ruler_grid_bar_color");
	const Leaf_Color sub_color = naui_theme_color("uph_timeline_top_ruler_grid_subbeat_color");
	const Leaf_Color number_color = naui_theme_color("uph_timeline_top_ruler_grid_text_color");

	const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);
	const float division_px = (float)(division * uph_song_timeline_data.zoom.x);
	const bool draw_subdivisions = division < 1.0 && division_px >= 3.0f;
	const double divs_per_beat = 1.0 / division;
	const float scroll_x = uph_song_timeline_data.scroll.x;
	const int64_t first_div = (int64_t)floor((double)scroll_x / division_px);
	const uint32_t div_count = (uint32_t)(bbox.width / division_px) + 2;

	uph_song_timeline_update_playhead_drag(bbox);
	naui_push_clip_rect(bbox.x - 0.5f, bbox.y, bbox.width, bbox.height);
	for (uint32_t i = 0; i < div_count; i++)
	{
		const int64_t div_index = first_div + (int64_t)i;
		if (div_index < 0)
			continue;

		const float x = bbox.x + (float)div_index * division_px - scroll_x;
		if (x < bbox.x - division_px || x > bbox.x + bbox.width)
			continue;

		const bool is_beat = fmod((double)div_index, divs_per_beat) < 0.5;
		if (!is_beat)
		{
			if (draw_subdivisions)
			{
				naui_draw_line(
					(Naui_Vec2){ x, bbox.y + bbox.height * 0.6f },
					(Naui_Vec2){ x, bbox.y + bbox.height },
					sub_color,
					1.0f
				);
			}
			continue;
		}

		const int64_t beat_index = (int64_t)llround((double)div_index / divs_per_beat);
		const bool is_downbeat = (beat_index % 4) == 0;
		const Leaf_Color line_color = is_downbeat ? bar_color : beat_color;
		naui_draw_line(
			(Naui_Vec2){ x, bbox.y + bbox.height * (is_downbeat ? 0.4f : 0.6f)},
			(Naui_Vec2){ x, bbox.y + bbox.height },
			line_color,
			1.0f
		);

		if (is_downbeat)
		{
			char label[16];
			snprintf(label, sizeof(label), "%d", (int)(beat_index / 4));
			naui_draw_text((Naui_Vec2) { x + 4.0f, bbox.y + bbox.height * 0.4f }, label, NAUI_DPI(13.0f), 0, number_color);
		}
	}

	naui_pop_clip_rect();
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
