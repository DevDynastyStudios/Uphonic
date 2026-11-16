#define UPH_SONG_TIMELINE_MINIMAP_HEIGHT 30
#define UPH_SONG_TIMELINE_TIME_DISPLAY_SCALE 1.3f
#define UPH_SONG_TIMELINE_MINIMAP_INSET 2

typedef struct
{
	Leaf_BoundingBox rect;
	float scale_x;
	float scale_y;
}
Uph_SongTimelineMinimapMapping;

static float _uph_song_timeline_minimap_view_width(void)
{
	return fmaxf(1.0f, _uph_song_timeline_state.panel_bounding_box.width - NAUI_DPI(uph_song_timeline_header_column_width()));
}

static double _uph_song_timeline_minimap_total_beats(void)
{
	const Uph_SongTimelineState *state = &_uph_song_timeline_state;
	const double song_end = uph_audio_engine_get_song_length() + uph_time_signature_bar_length() * 4.0;
	const double view_end = ((double)state->scroll.x + _uph_song_timeline_minimap_view_width()) / state->zoom.x;
	return fmax(song_end, view_end);
}

static Leaf_BoundingBox _uph_song_timeline_minimap_inset(Leaf_BoundingBox bbox)
{
	const int32_t inset = NAUI_DPI(UPH_SONG_TIMELINE_MINIMAP_INSET);
	bbox.x += inset;
	bbox.y += inset;
	bbox.width = fmaxf(1.0f, bbox.width - inset * 2.0f);
	bbox.height = fmaxf(1.0f, bbox.height - inset * 2.0f);
	return bbox;
}

static Uph_SongTimelineMinimapMapping _uph_song_timeline_minimap_mapping(Leaf_BoundingBox bbox, double total_beats, float total_height)
{
	const Leaf_BoundingBox rect = _uph_song_timeline_minimap_inset(bbox);
	return (Uph_SongTimelineMinimapMapping){
		.rect = rect,
		.scale_x = (float)(rect.width / fmax(total_beats, 1e-6)),
		.scale_y = rect.height / fmaxf(total_height, 1.0f)
	};
}

static void _uph_song_timeline_minimap_draw_tracks(Naui_List(Uph_Track) tracks, float *y, const Uph_SongTimelineMinimapMapping *map)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	const float zoom_y = state->zoom.y;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		const Uph_Track *track = &tracks[i];
		const bool group = uph_track_is_folder(track);
		const float height = group ? NAUI_DPI(UPH_SONG_TIMELINE_FOLDER_ROW_HEIGHT) : NAUI_DPI(zoom_y);
		const float row_y = map->rect.y + *y * map->scale_y;
		const float row_h = fmaxf(1.0f, height * map->scale_y);
		const float gap = row_h > 4.0f ? 1.0f : 0.0f;
		Naui_Color color = uph_resources_track_color(track->color_index);

		if (group)
		{
			color.a = 20;
			naui_fill_rect((Naui_Vec2){ map->rect.x, row_y + gap }, (Naui_Vec2){ map->rect.width, row_h - gap * 2.0f }, color, 0.0f, NAUI_CORNER_NONE);
		}
		else
		{
			color.a = 90;
			for (uint32_t b = 0; b < (uint32_t)naui_list_len(track->blocks); b++)
			{
				const Uph_TimelineBlock *block = &track->blocks[b];
				const float block_x = map->rect.x + (float)(block->start_beat * map->scale_x);
				const float block_w = fmaxf(1.0f, (float)(block->length_beats * map->scale_x));
				naui_fill_rect((Naui_Vec2){ block_x, row_y + gap }, (Naui_Vec2){ block_w, row_h - gap * 2.0f }, color, 0.0f, NAUI_CORNER_NONE);
			}
		}

		*y += height;
		_uph_song_timeline_minimap_draw_tracks(track->subtracks, y, map);
	}
}

static float _uph_song_timeline_minimap_rows_height(Naui_List(Uph_Track) tracks)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	float height = 0.0f;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		const Uph_Track *track = &tracks[i];
		height += uph_track_is_folder(track) ? NAUI_DPI(UPH_SONG_TIMELINE_FOLDER_ROW_HEIGHT) : NAUI_DPI(state->zoom.y);
		height += _uph_song_timeline_minimap_rows_height(track->subtracks);
	}
	return height;
}

static void _uph_song_timeline_minimap_draw(Leaf_BoundingBox bbox, void *user_data)
{
	const Uph_SongTimelineState *state = &_uph_song_timeline_state;
	const double total_beats = state->minimap.dragging ? state->minimap.total_beats : _uph_song_timeline_minimap_total_beats();
	const Uph_SongTimelineMinimapMapping map = _uph_song_timeline_minimap_mapping(bbox, total_beats, _uph_song_timeline_minimap_rows_height(uph_state.project.tracks));

	naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);

	float y = 0.0f;
	_uph_song_timeline_minimap_draw_tracks(uph_state.project.tracks, &y, &map);

	const float playhead_x = map.rect.x + (float)(uph_state.shared.song_timeline_playhead_position * map.scale_x);
	Naui_Color playhead_color = uph_state.colors.song_timeline_playhead;
	playhead_color.a = (uint8_t)(playhead_color.a * 0.6f);
	naui_draw_line((Naui_Vec2){ playhead_x, map.rect.y }, (Naui_Vec2){ playhead_x, map.rect.y + map.rect.height }, playhead_color, NAUI_DPI(1));

	const Naui_Vec2 thumb_pos = { map.rect.x + (state->scroll.x / state->zoom.x) * map.scale_x, bbox.y };
	const Naui_Vec2 thumb_size = { fmaxf(NAUI_DPI(6), (_uph_song_timeline_minimap_view_width() / state->zoom.x) * map.scale_x), bbox.height };
	const bool active = state->minimap.dragging || state->minimap.hovered;
	Naui_Color fill = uph_state.colors.ui_scrollbar_handle;
	Naui_Color outline = fill;
	fill.a = active ? 110 : 70;
	outline.a = active ? 255 : 180;
	const int32_t rounding = NAUI_DPI(UPH_UI_SCROLLBAR_HANDLE_ROUNDING * 0.5f);
	naui_fill_rect(thumb_pos, thumb_size, fill, rounding, NAUI_CORNER_ALL);
	naui_draw_rect(thumb_pos, thumb_size, outline, NAUI_DPI(1), rounding, NAUI_CORNER_ALL, NAUI_SIDE_ALL);

	naui_pop_clip_rect();
}

static void _uph_song_timeline_minimap_apply(Leaf_BoundingBox bbox)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	const Uph_SongTimelineMinimapMapping map = _uph_song_timeline_minimap_mapping(bbox, state->minimap.total_beats, 1.0f);
	const float thumb_start = (float)naui_mouse_x() - state->minimap.grab_offset - map.rect.x;
	state->scroll.x = fmaxf(0.0f, (thumb_start / map.scale_x) * state->zoom.x);
}

static void _uph_song_timeline_minimap_press(Leaf_BoundingBox bbox)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	state->minimap.total_beats = _uph_song_timeline_minimap_total_beats();
	const Uph_SongTimelineMinimapMapping map = _uph_song_timeline_minimap_mapping(bbox, state->minimap.total_beats, 1.0f);

	const float mouse = (float)naui_mouse_x();
	const float thumb_start = map.rect.x + (state->scroll.x / state->zoom.x) * map.scale_x;
	const float thumb_length = (_uph_song_timeline_minimap_view_width() / state->zoom.x) * map.scale_x;
	const bool on_thumb = mouse >= thumb_start && mouse < thumb_start + thumb_length;
	state->minimap.grab_offset = on_thumb ? mouse - thumb_start : thumb_length * 0.5f;
	state->minimap.dragging = true;
	_uph_song_timeline_minimap_apply(bbox);
}

void uph_song_timeline_update_minimap(void)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;

	const Leaf_BoundingBox bbox = leaf_get_bounding_box(leaf_id("uph_song_timeline_minimap"));
	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };

	state->minimap.hovered = false;

	if (state->minimap.dragging)
	{
		if (!naui_mouse_down(NAUI_MOUSE_LEFT))
			state->minimap.dragging = false;
		else
		{
			_uph_song_timeline_minimap_apply(bbox);
			uph_global_widget_state.any_widget_hovered = true;
			naui_set_cursor(NAUI_CURSOR_HAND);
			return;
		}
	}

	if (!state->panel_hovered || state->drag.active || state->marquee.active)
		return;

	if (!uph_song_timeline_vec4_contains_vec2((Naui_Vec4){ bbox.x, bbox.y, bbox.width, bbox.height }, mouse))
		return;

	state->minimap.hovered = true;
	uph_global_widget_state.any_widget_hovered = true;
	naui_set_cursor(NAUI_CURSOR_HAND);
	if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		_uph_song_timeline_minimap_press(bbox);
}

void uph_song_timeline_render_minimap(void)
{
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SONG_TIMELINE_MINIMAP_HEIGHT)) },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_header_column_width())), .height = LEAF_SIZE_FULL }
		});
		leaf({
			.id = leaf_id("uph_song_timeline_minimap"),
			.custom_draw = (Leaf_CustomDrawFn)_uph_song_timeline_minimap_draw,
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL },
			.color = uph_state.colors.ui_scrollbar_bg
		});
	}
}

void uph_song_timeline_render_time_display(void)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	const Leaf_ID id = leaf_id("uph_song_timeline_time_display");
	const double beat = uph_state.shared.song_timeline_playhead_position;
	const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const Naui_Vec2 padding = UPH_SONG_TIMELINE_HEADER_PADDING;

	const bool hovered = uph_ui_widget_hovered(id);
	if (hovered)
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			state->time_display_seconds = !state->time_display_seconds;
	}

	char primary_text[32];
	char secondary_text[32];
	const char *primary_label = uph_ui_format_time(beat, state->time_display_seconds, primary_text, sizeof(primary_text));
	const char *secondary_label = uph_ui_format_time(beat, !state->time_display_seconds, secondary_text, sizeof(secondary_text));

	const Leaf_Color text_color = uph_state.colors.ui_text;
	Leaf_Color dim_color = text_color;
	dim_color.a = (uint8_t)(text_color.a * 0.6f);
	Leaf_Color faint_color = text_color;
	faint_color.a = (uint8_t)(text_color.a * 0.4f);

	leaf({
		.id = id,
		.size = {
			.width = LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_header_column_width()) - NAUI_DPI(padding.x) * 2.0f),
			.height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SONG_TIMELINE_MINIMAP_HEIGHT + UPH_SONG_TIMELINE_TOP_RULER_HEIGHT))
		},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0.0f),
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_ALL,
			.color = uph_state.colors.song_timeline_ruler_border
		},
		.color = uph_state.colors.song_timeline_header,
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.direction = LEAF_DIRECTION_VERTICAL
		})
		{
			leaf_text(primary_text, {
				.color = text_color,
				.font_size = LEAF_SIZE_FIXED(font_size * UPH_SONG_TIMELINE_TIME_DISPLAY_SCALE)
			});
			leaf_text(primary_label, {
				.color = dim_color,
				.font_size = LEAF_SIZE_FIXED(font_size * 0.69f)
			});
		}
		leaf({
			.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER },
			.direction = LEAF_DIRECTION_VERTICAL
		})
		{
			leaf_text(secondary_text, {
				.color = dim_color,
				.font_size = LEAF_SIZE_FIXED(font_size * 0.85f)
			});
			leaf_text(secondary_label, {
				.color = faint_color,
				.font_size = LEAF_SIZE_FIXED(font_size * 0.69f)
			});
		}
	}
}
