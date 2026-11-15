#define UPH_SONG_TIMELINE_TOP_RULER_HEIGHT 28

void uph_song_timeline_render_ruler(Leaf_BoundingBox bbox, float zoom_x, float scroll_x)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	uph_grid_render_beats(bbox, zoom_x, scroll_x, uph_snap_division(state->snap_resolution, state->zoom.x), (Uph_GridColors){
		.sub_color = naui_theme_color("uph_song_timeline_grid_subbeat_color"),
		.beat_color = naui_theme_color("uph_song_timeline_grid_beat_color"),
		.bar_color = naui_theme_color("uph_song_timeline_grid_bar_color")
	});
}

static void uph_song_timeline_update_playhead_drag(Leaf_BoundingBox bbox)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	Uph_PlayheadDragState *drag = &state->playhead_drag;

	const bool mouse_over_ruler = uph_song_timeline_vec4_contains_vec2(
		(Naui_Vec4){ bbox.x, bbox.y, bbox.width, bbox.height },
		(Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() }
	);

	const bool loop_busy = uph_loop_region_update(
		&uph_state.project.loop_region, &state->loop_drag, bbox,
		state->zoom.x, state->scroll.x, uph_snap_division(state->snap_resolution, state->zoom.x),
		!drag->active && naui_panel_is_hovered(uph_state.panels.song_timeline)
	);
	if (state->loop_drag.created)
	{
		state->loop_drag.created = false;
		uph_state.project.loop_enabled = true;
	}

	if (!drag->active && !loop_busy && mouse_over_ruler && naui_mouse_pressed(NAUI_MOUSE_LEFT) && naui_panel_is_hovered(uph_state.panels.song_timeline))
	{
		drag->active = true;
		drag->beat = -1.0;
	}

	if (!drag->active)
		return;

	state->follow.anchored = false;
	const double mouse_beat_raw = ((double)naui_mouse_x() - bbox.x + state->scroll.x) / state->zoom.x;
	double beat = fmax(0.0, uph_snap_beat_floor(mouse_beat_raw, state->snap_resolution, state->zoom.x));
	if (uph_state.project.loop_enabled && !uph_loop_region_valid(&uph_state.project.loop_region))
	{
		const double length = uph_audio_engine_get_song_length();
		if (length > 0.0 && beat >= length)
			beat = fmax(0.0, length - uph_snap_division(state->snap_resolution, state->zoom.x));
	}

	if (beat != drag->beat)
	{
		drag->beat = beat;
		uph_state.shared.song_timeline_playhead_position = beat;
	}

	naui_set_cursor(NAUI_CURSOR_HAND);
	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		drag->active = false;
}

static void uph_song_timeline_render_top_ruler(Leaf_BoundingBox bbox, void *user_data)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	uph_song_timeline_update_playhead_drag(bbox);
	uph_loop_region_render_ruler(&uph_state.project.loop_region, bbox, state->zoom.x, state->scroll.x, uph_state.project.loop_enabled, naui_theme_color("uph_song_timeline_selection_outline_color"));
	uph_grid_render_ruler(bbox, state->zoom.x, state->scroll.x, uph_snap_division(state->snap_resolution, state->zoom.x), (Uph_GridColors){
		.sub_color = naui_theme_color("uph_song_timeline_ruler_grid_subbeat_color"),
		.beat_color = naui_theme_color("uph_song_timeline_ruler_grid_beat_color"),
		.bar_color = naui_theme_color("uph_song_timeline_ruler_grid_bar_color"),
		.number_color = naui_theme_color("uph_song_timeline_ruler_grid_text_color")
	});
}

void uph_song_timeline_render_top_bar(void)
{
	const float header_width = UPH_SONG_TIMELINE_HEADER_WIDTH;
	const Naui_Vec2 header_padding = UPH_SONG_TIMELINE_HEADER_PADDING;
	const uint32_t height = NAUI_DPI(UPH_SONG_TIMELINE_TOP_RULER_HEIGHT);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(height) },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(header_width + uph_song_timeline_header_depth_width())), .height = LEAF_SIZE_FULL },
			.padding = LEAF_PADDING_AXES(NAUI_DPI(header_padding.x), 0.0f),
			.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER }
		});

		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL }
		})
		{
			leaf({
				.custom_draw = (Leaf_CustomDrawFn)uph_song_timeline_render_top_ruler,
				.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(height - 2) },
				.border = {
					.width = 1,
					.sides = LEAF_SIDE_ALL,
					.color = naui_theme_color("uph_song_timeline_ruler_border_color")
				},
				.color = naui_theme_color("uph_song_timeline_ruler_bg_color")
			});
		}
	}
}

void uph_song_timeline_render_playhead_overlay(Leaf_BoundingBox bbox, void *data)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	const Naui_Image *playhead_image = naui_asset_image("uph_icon_playhead");
	const Naui_Color color = naui_theme_color("uph_song_timeline_playhead_color");
	const int32_t x_offset = NAUI_DPI(uph_song_timeline_header_column_width());
	const int32_t playhead_size = NAUI_DPI(16);
	const float playhead_half_size = playhead_size * 0.5f;
	const float playhead_y = bbox.y + NAUI_DPI(UPH_SONG_TIMELINE_TOP_RULER_HEIGHT) - playhead_size;
	naui_push_clip_rect(bbox.x + x_offset - NAUI_DPI(1), bbox.y, bbox.width, bbox.height);

	if (state->current_action_mode == UPH_ACTION_CUT)
	{
		const double mouse_beat = ((double)naui_mouse_x() - (bbox.x + x_offset) + state->scroll.x) / state->zoom.x;
		const double cut_beat = uph_snap_beat_round(mouse_beat, state->snap_resolution, state->zoom.x);
		const float x = bbox.x + x_offset + (float)(cut_beat * state->zoom.x) - state->scroll.x;
		naui_draw_line(
			(Naui_Vec2){ x, playhead_y },
			(Naui_Vec2){ x, bbox.y + bbox.height },
			color,
			NAUI_DPI(1)
		);
	}

	uph_loop_region_render_bounds(
		&uph_state.project.loop_region,
		bbox.x + x_offset, bbox.y + NAUI_DPI(UPH_SONG_TIMELINE_TOP_RULER_HEIGHT), bbox.y + bbox.height,
		state->zoom.x, state->scroll.x, uph_state.project.loop_enabled, naui_theme_color("uph_song_timeline_selection_outline_color")
	);

	const Uph_MarqueeState *marquee = &state->marquee;
	const float x = bbox.x + x_offset + uph_state.shared.song_timeline_playhead_position * state->zoom.x - state->scroll.x;
	naui_draw_line(
		(Naui_Vec2){ x, playhead_y },
		(Naui_Vec2){ x, bbox.y + bbox.height },
		color,
		NAUI_DPI(1)
	);

	naui_draw_image(
		playhead_image,
		(Naui_Vec2){x - playhead_half_size, playhead_y},
		(Naui_Vec2){playhead_size, playhead_size},
		color,
		0.0f,
		NAUI_CORNER_NONE
	);

	if (marquee->active)
	{
		const float corner_x = bbox.x + x_offset + (float)(marquee->start_beat * state->zoom.x) - state->scroll.x;
		const float corner_y = state->panel_bounding_box.y - state->scroll.y + marquee->start_content_y;
		const Naui_Vec2 top_left = { fminf(corner_x, (float)naui_mouse_x()), fminf(corner_y, (float)naui_mouse_y()) };
		const Naui_Vec2 size = { fabsf(corner_x - (float)naui_mouse_x()), fabsf(corner_y - (float)naui_mouse_y()) };
		const Leaf_Color fill_color = naui_theme_color("uph_song_timeline_marquee_color");
		const Leaf_Color border_color = naui_theme_color("uph_song_timeline_marquee_border_color");
		naui_fill_rect(top_left, size, fill_color, 0.0f, NAUI_CORNER_NONE);
		naui_draw_rect(top_left, size, border_color, NAUI_DPI(1), 0.0f, NAUI_CORNER_NONE, NAUI_SIDE_ALL);
	}

	naui_pop_clip_rect();
}
