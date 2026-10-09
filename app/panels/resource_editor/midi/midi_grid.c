void uph_midi_grid_render_lanes(Leaf_BoundingBox box, const Uph_MidiView *view, const Uph_NoteMap *map, const Uph_Piano *piano)
{
	const float lane_height = uph_midi_view_lane_height(view);
	const Leaf_Color accidental_color = naui_theme_color("uph_timeline_row_bg1_color");
	const Leaf_Color natural_color = naui_theme_color("uph_timeline_row_bg2_color");
	const Leaf_Color pressed_color = naui_theme_color("uph_piano_sharp_pressed_color");
	const Leaf_Color accent_color = naui_theme_color("naui_widget_accent_color");

	for (int32_t key = map->lowest; key <= map->highest; key++)
	{
		const float y_position = uph_midi_view_row_to_y(view, box, uph_note_map_row(map, key));
		if (y_position > box.y + box.height || y_position < box.y - lane_height)
			continue;

		const Uph_NoteDef *def = &map->notes[key];
		Leaf_Color color = def->kind == UPH_NOTE_KIND_ACCIDENTAL ? accidental_color : natural_color;
		if (def->highlight == UPH_NOTE_HIGHLIGHT_SCALE)
			color = uph_color_mix(color, accent_color, UPH_MIDI_GRID_SCALE_TINT);
		else if (def->highlight == UPH_NOTE_HIGHLIGHT_ROOT)
			color = uph_color_mix(color, accent_color, UPH_MIDI_GRID_ROOT_TINT);

		if (uph_piano_key_pressed(piano, (uint8_t)key))
			color = pressed_color;

		naui_fill_rect(
			(Naui_Vec2) { box.x, y_position },
			(Naui_Vec2) { box.width, lane_height },
			color,
			0.0f,
			NAUI_CORNER_NONE
		);
	}
}

void uph_midi_grid_render_beats(Leaf_BoundingBox box, const Uph_MidiView *view, Uph_SnapResolution snap)
{
	const float zoom_x = view->zoom.x;
	const float scroll_x = view->scroll.x;
	const Leaf_Color beat_color = naui_theme_color("uph_timeline_grid_beat_color");
	const Leaf_Color bar_color = naui_theme_color("uph_timeline_grid_bar_color");
	const Leaf_Color sub_color = naui_theme_color("uph_timeline_grid_subbeat_color");

	const double division = uph_snap_division(snap);
	const float division_px = (float)(division * zoom_x);
	const bool draw_subdivisions = division < 1.0 && division_px >= 3.0f;
	const int64_t first_div = (int64_t)floor((double)scroll_x / division_px);
	const uint32_t div_count = (uint32_t)(box.width / division_px) + 2;

	for (uint32_t i = 0; i < div_count; i++)
	{
		const int64_t div_index = first_div + (int64_t)i;
		if (div_index < 0)
			continue;

		const float x = box.x + (float)div_index * division_px - scroll_x;
		if (x < box.x || x > box.x + box.width)
			continue;

		const double divs_per_beat = 1.0 / division;
		const bool is_beat = fmod((double)div_index, divs_per_beat) < 0.5;

		if (!is_beat)
		{
			if (draw_subdivisions)
				naui_draw_line((Naui_Vec2) { x, box.y }, (Naui_Vec2) { x, box.y + box.height }, sub_color, 1.0f);

			continue;
		}

		const int64_t beat_index = (int64_t)llround((double)div_index / divs_per_beat);
		const bool is_downbeat = (beat_index % 4) == 0;
		const Leaf_Color line_color = is_downbeat ? bar_color : beat_color;
		naui_draw_line((Naui_Vec2) { x, box.y }, (Naui_Vec2) { x, box.y + box.height }, line_color, 1.0f);
	}
}

void uph_midi_grid_render_ruler(Leaf_BoundingBox box, const Uph_MidiView *view, Uph_SnapResolution snap)
{
	const Leaf_Color beat_color = naui_theme_color("uph_timeline_top_ruler_grid_beat_color");
	const Leaf_Color bar_color = naui_theme_color("uph_timeline_top_ruler_grid_bar_color");
	const Leaf_Color sub_color = naui_theme_color("uph_timeline_top_ruler_grid_subbeat_color");
	const Leaf_Color number_color = naui_theme_color("uph_timeline_top_ruler_grid_text_color");

	const float zoom_x = view->zoom.x;
	const float scroll_x = view->scroll.x;

	const double division = uph_snap_division(snap);
	const float division_px = (float)(division * zoom_x);
	const bool draw_subdivisions = division < 1.0 && division_px >= 3.0f;
	const double divs_per_beat = 1.0 / division;
	const int64_t first_div = (int64_t)floor((double)scroll_x / division_px);
	const uint32_t div_count = (uint32_t)(box.width / division_px) + 2;

	naui_push_clip_rect(box.x - 0.5f, box.y, box.width, box.height);
	for (uint32_t i = 0; i < div_count; i++)
	{
		const int64_t div_index = first_div + (int64_t)i;
		if (div_index < 0)
			continue;

		const float x = box.x + (float)div_index * division_px - scroll_x;
		if (x < box.x - division_px || x > box.x + box.width)
			continue;

		const bool is_beat = fmod((double)div_index, divs_per_beat) < 0.5;

		if (!is_beat)
		{
			if (draw_subdivisions)
			{
				naui_draw_line(
					(Naui_Vec2) { x, box.y + box.height * 0.6f },
					(Naui_Vec2) { x, box.y + box.height },
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
			(Naui_Vec2) { x, box.y + box.height * (is_downbeat ? 0.4f : 0.6f) },
			(Naui_Vec2) { x, box.y + box.height },
			line_color,
			1.0f
		);

		if (is_downbeat)
		{
			char label[16];
			snprintf(label, sizeof(label), "%d", (int)(beat_index / 4));
			naui_draw_text((Naui_Vec2) { x + 4.0f, box.y + box.height * 0.4f }, label, NAUI_DPI(13.0f), 0, number_color);
		}
	}
	
	naui_pop_clip_rect();
}
