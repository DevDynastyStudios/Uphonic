typedef struct
{
	Leaf_BoundingBox rect;
	float scale_x;
	float scale_y;
}
Uph_SongTimelineMinimapMapping;

static float _uph_song_timeline_minimap_view_width(void)
{
	const Uph_SongTimelineData *data = &uph_song_timeline_data;
	return fmaxf(1.0f, data->panel_bounding_box.width - NAUI_DPI(uph_song_timeline_header_column_width()));
}

static double _uph_song_timeline_minimap_total_beats(void)
{
	const Uph_SongTimelineData *data = &uph_song_timeline_data;
	const double song_end = uph_audio_engine_get_song_length() + uph_time_signature_bar_length() * 4.0;
	const double view_end = ((double)data->scroll.x + _uph_song_timeline_minimap_view_width()) / data->zoom.x;
	return fmax(song_end, view_end);
}

static Leaf_BoundingBox _uph_song_timeline_minimap_inset(Leaf_BoundingBox bbox)
{
	const float inset = NAUI_DPI(UPH_SONG_TIMELINE_MINIMAP_INSET);
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
	const float zoom_y = uph_song_timeline_data.zoom.y;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		const Uph_Track *track = &tracks[i];
		const bool group = track->type == UPH_RESOURCE_GROUP;
		const float height = group ? NAUI_DPI(56) : NAUI_DPI(zoom_y);
		const float row_y = map->rect.y + *y * map->scale_y;
		const float row_h = fmaxf(1.0f, height * map->scale_y);
		const float gap = row_h > 4.0f ? 1.0f : 0.0f;
		Naui_Color color = uph_resources_track_color(track->color_index);

		if (group)
		{
			color.a = 40;
			naui_fill_rect((Naui_Vec2){ map->rect.x, row_y + gap }, (Naui_Vec2){ map->rect.width, row_h - gap * 2.0f }, color, 0.0f, NAUI_CORNER_NONE);
		}
		else
		{
			color.a = 220;
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
	float height = 0.0f;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		const Uph_Track *track = &tracks[i];
		height += track->type == UPH_RESOURCE_GROUP ? NAUI_DPI(56) : NAUI_DPI(uph_song_timeline_data.zoom.y);
		height += _uph_song_timeline_minimap_rows_height(track->subtracks);
	}
	return height;
}

static void _uph_song_timeline_minimap_draw(Leaf_BoundingBox bbox, void *user_data)
{
	const Uph_SongTimelineData *data = &uph_song_timeline_data;
	const double total_beats = data->minimap.dragging ? data->minimap.total_beats : _uph_song_timeline_minimap_total_beats();
	const Uph_SongTimelineMinimapMapping map = _uph_song_timeline_minimap_mapping(bbox, total_beats, _uph_song_timeline_minimap_rows_height(uph_state.project.tracks));

	naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);

	float y = 0.0f;
	_uph_song_timeline_minimap_draw_tracks(uph_state.project.tracks, &y, &map);

	const float playhead_x = map.rect.x + (float)(uph_state.shared.song_timeline_playhead_position * map.scale_x);
	naui_draw_line((Naui_Vec2){ playhead_x, map.rect.y }, (Naui_Vec2){ playhead_x, map.rect.y + map.rect.height }, naui_theme_color("uph_playhead_color"), NAUI_DPI(1.0f));

	const Naui_Vec2 thumb_pos = { map.rect.x + (data->scroll.x / data->zoom.x) * map.scale_x, bbox.y };
	const Naui_Vec2 thumb_size = { fmaxf(NAUI_DPI(6.0f), (_uph_song_timeline_minimap_view_width() / data->zoom.x) * map.scale_x), bbox.height };
	const bool active = data->minimap.dragging || data->minimap.hovered;
	Naui_Color fill = naui_theme_color("uph_ui_scrollbar_handle_color");
	Naui_Color outline = fill;
	fill.a = active ? 110 : 70;
	outline.a = active ? 255 : 180;
	const float rounding = NAUI_DPI(UPH_UI_SCROLLBAR_HANDLE_ROUNDING * 0.5f);
	naui_fill_rect(thumb_pos, thumb_size, fill, rounding, NAUI_CORNER_ALL);
	naui_draw_rect(thumb_pos, thumb_size, outline, NAUI_DPI(1.0f), rounding, NAUI_CORNER_ALL, NAUI_SIDE_ALL);

	naui_pop_clip_rect();
}

static void _uph_song_timeline_minimap_apply(Leaf_BoundingBox bbox)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	const Uph_SongTimelineMinimapMapping map = _uph_song_timeline_minimap_mapping(bbox, data->minimap.total_beats, 1.0f);
	const float thumb_start = (float)naui_mouse_x() - data->minimap.grab_offset - map.rect.x;
	data->scroll.x = fmaxf(0.0f, (thumb_start / map.scale_x) * data->zoom.x);
}

static void _uph_song_timeline_minimap_press(Leaf_BoundingBox bbox)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	data->minimap.total_beats = _uph_song_timeline_minimap_total_beats();
	const Uph_SongTimelineMinimapMapping map = _uph_song_timeline_minimap_mapping(bbox, data->minimap.total_beats, 1.0f);

	const float mouse = (float)naui_mouse_x();
	const float thumb_start = map.rect.x + (data->scroll.x / data->zoom.x) * map.scale_x;
	const float thumb_length = (_uph_song_timeline_minimap_view_width() / data->zoom.x) * map.scale_x;
	const bool on_thumb = mouse >= thumb_start && mouse < thumb_start + thumb_length;
	data->minimap.grab_offset = on_thumb ? mouse - thumb_start : thumb_length * 0.5f;
	data->minimap.dragging = true;
	_uph_song_timeline_minimap_apply(bbox);
}

void uph_song_timeline_update_minimap(void)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	extern Uph_GlobalWidgetData uph_global_widget_data;

	const Leaf_BoundingBox bbox = leaf_get_bounding_box(leaf_id("uph_song_timeline_minimap"));
	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };

	data->minimap.hovered = false;

	if (data->minimap.dragging)
	{
		if (!naui_mouse_down(NAUI_MOUSE_LEFT))
			data->minimap.dragging = false;
		else
		{
			_uph_song_timeline_minimap_apply(bbox);
			uph_global_widget_data.any_widget_hovered = true;
			naui_set_cursor(NAUI_CURSOR_HAND);
			return;
		}
	}

	if (!data->panel_hovered || data->drag.active || data->marquee.active)
		return;

	if (!uph_song_timeline_vec4_contains_vec2((Naui_Vec4){ bbox.x, bbox.y, bbox.width, bbox.height }, mouse))
		return;

	data->minimap.hovered = true;
	uph_global_widget_data.any_widget_hovered = true;
	naui_set_cursor(NAUI_CURSOR_HAND);
	if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		_uph_song_timeline_minimap_press(bbox);
}

void uph_song_timeline_render_minimap(void)
{
	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_SONG_TIMELINE_MINIMAP_HEIGHT))}
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_header_column_width())), LEAF_SIZE_FULL}
		});
		leaf({
			.id = leaf_id("uph_song_timeline_minimap"),
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.color = naui_theme_color("uph_ui_scrollbar_bg_color"),
			.custom_draw = (Leaf_CustomDrawFn)_uph_song_timeline_minimap_draw
		});
	}
}

void uph_song_timeline_render_time_display(void)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	const Leaf_ID id = leaf_id("uph_song_timeline_time_display");
	const double beat = uph_state.shared.song_timeline_playhead_position;
	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const Naui_Vec2 padding = UPH_SONG_TIMELINE_HEADER_PADDING;

	const bool hovered = uph_ui_widget_hovered(id);
	if (hovered)
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			data->time_display_seconds = !data->time_display_seconds;
	}

	char primary_text[32];
	char secondary_text[32];
	const char *primary_label = uph_ui_format_time(beat, data->time_display_seconds, primary_text, sizeof(primary_text));
	const char *secondary_label = uph_ui_format_time(beat, !data->time_display_seconds, secondary_text, sizeof(secondary_text));

	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	Leaf_Color dim_color = text_color;
	dim_color.a = (uint8_t)(text_color.a * 0.6f);
	Leaf_Color faint_color = text_color;
	faint_color.a = (uint8_t)(text_color.a * 0.4f);

	leaf({
		.id = id,
		.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {
			LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_header_column_width()) - NAUI_DPI(padding.x) * 2.0f),
			LEAF_SIZE_FIXED(NAUI_DPI(UPH_SONG_TIMELINE_MINIMAP_HEIGHT + UPH_SONG_TIMELINE_TOP_RULER_HEIGHT))
		},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0.0f),
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.color = naui_theme_color(hovered ? "uph_song_timeline_header_selected_color" : "uph_song_timeline_header_color"),
		.border = {
			.width = 1,
			.color = {naui_theme_color("uph_timeline_top_ruler_border_color")},
			.sides = LEAF_SIDE_ALL
		}
	})
	{
		leaf({
			.direction = LEAF_DIRECTION_VERTICAL,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
		})
		{
			leaf_text(primary_text, {
				.font_size = font_size * UPH_SONG_TIMELINE_TIME_DISPLAY_SCALE,
				.color = text_color
			});
			leaf_text(primary_label, {
				.font_size = font_size * 0.72f,
				.color = dim_color
			});
		}
		leaf({
			.direction = LEAF_DIRECTION_VERTICAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
			.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
		})
		{
			leaf_text(secondary_text, {
				.font_size = font_size * 0.9f,
				.color = dim_color
			});
			leaf_text(secondary_label, {
				.font_size = font_size * 0.72f,
				.color = faint_color
			});
		}
	}
}
