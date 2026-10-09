void uph_midi_notes_render(Leaf_BoundingBox box, const Uph_MidiView *view, const Uph_NoteMap *map, const Uph_MidiPattern *pattern)
{
	const float lane_height = uph_midi_view_lane_height(view);
	const float note_label_font_size = fminf(NAUI_DPI(11.0f), lane_height * 0.8f);
	const float note_label_padding = NAUI_DPI(3.0f);
	const float note_rounding = NAUI_DPI(3.0f);
	const Leaf_Color note_label_color = LEAF_COLOR_WHITE;
	const Leaf_Color note_color = naui_theme_color("uph_midi_editor_default_note_color");

	for (uint32_t i = 0; i < naui_list_len(pattern->notes); i++)
	{
		const Uph_MidiNote *note = &pattern->notes[i];
		const float y_position = uph_midi_view_row_to_y(view, box, uph_note_map_row(map, note->key_number));
		if (y_position > box.y + box.height || y_position < box.y - lane_height)
			continue;

		const float note_x = uph_midi_view_beat_to_x(view, box, note->start_beat);
		const float note_width = (float)(note->length_beats * view->zoom.x);

		naui_fill_rect(
			(Naui_Vec2) { note_x, y_position },
			(Naui_Vec2) { note_width, lane_height },
			note_color,
			note_rounding,
			LEAF_CORNER_ALL
		);

		if (note->selected)
		{
			naui_draw_rect(
				(Naui_Vec2) { note_x, y_position },
				(Naui_Vec2) { note_width, lane_height },
				LEAF_COLOR_WHITE,
				NAUI_DPI(2.0f),
				note_rounding,
				LEAF_CORNER_ALL,
				NAUI_SIDE_ALL
			);
		}

		if (note_label_font_size >= NAUI_DPI(6.0f) && note->key_number < UPH_NOTE_MAP_KEY_COUNT)
		{
			naui_push_clip_rect(note_x, y_position, note_width, lane_height);
			naui_draw_text(
				(Naui_Vec2) { note_x + note_label_padding, y_position + (lane_height - note_label_font_size) * 0.5f },
				map->notes[note->key_number].name,
				note_label_font_size,
				0,
				note_label_color
			);
			naui_pop_clip_rect();
		}
	}
}

void uph_midi_notes_render_marquee(const Uph_MidiView *view, Leaf_BoundingBox box, const Uph_NoteMarqueeState *marquee)
{
	if (!marquee->active)
		return;

	const float corner_x = uph_midi_view_beat_to_x(view, box, marquee->start_beat);
	const float corner_y = box.y - view->scroll.y + marquee->start_content_y;

	const Naui_Vec2 top_left = { fminf(corner_x, (float)naui_mouse_x()), fminf(corner_y, (float)naui_mouse_y()) };
	const Naui_Vec2 size = { fabsf(corner_x - (float)naui_mouse_x()), fabsf(corner_y - (float)naui_mouse_y()) };

	naui_fill_rect(top_left, size, leaf_rgba(255, 255, 255, 30), 0.0f, NAUI_CORNER_NONE);
	naui_draw_rect(top_left, size, leaf_rgba(255, 255, 255, 160), NAUI_DPI(1.0f), 0.0f, NAUI_CORNER_NONE, NAUI_SIDE_ALL);
}

void uph_midi_notes_render_cut_line(Leaf_BoundingBox box, const Uph_MidiView *view, Uph_SnapResolution snap)
{
	const double mouse_beat = uph_midi_view_x_to_beat(view, box, (float)naui_mouse_x());
	const double cut_beat = uph_snap_beat_round(mouse_beat, snap);
	const float x = uph_midi_view_beat_to_x(view, box, cut_beat);

	naui_draw_line(
		(Naui_Vec2) { x, box.y },
		(Naui_Vec2) { x, box.y + box.height },
		naui_theme_color("uph_playhead_color"),
		NAUI_DPI(1.5f)
	);
}
