#define UPH_MIDI_NOTE_VELOCITY_BAR_HEIGHT 3.0f
#define UPH_MIDI_NOTE_VELOCITY_BAR_INSET 2.0f
#define UPH_MIDI_NOTE_VELOCITY_BAR_MAX_LANE_RATIO 0.25f
#define UPH_MIDI_NOTE_VELOCITY_BAR_MIN_LANE_HEIGHT 10.0f
#define UPH_MIDI_NOTE_VELOCITY_TRACK_DARKEN_MIX 0.45f
#define UPH_MIDI_NOTE_VELOCITY_FILL_LIGHTEN_MIX 0.60f

void uph_midi_notes_render(Leaf_BoundingBox box, const Uph_MidiView *view, const Uph_NoteMap *map, const Uph_MidiPattern *pattern)
{
	const float lane_height = uph_midi_view_lane_height(view);
	const bool show_velocity_bar = lane_height >= NAUI_DPI(UPH_MIDI_NOTE_VELOCITY_BAR_MIN_LANE_HEIGHT);
	const float velocity_bar_height = fminf(NAUI_DPI(UPH_MIDI_NOTE_VELOCITY_BAR_HEIGHT), lane_height * UPH_MIDI_NOTE_VELOCITY_BAR_MAX_LANE_RATIO);
	const float velocity_bar_inset = NAUI_DPI(UPH_MIDI_NOTE_VELOCITY_BAR_INSET);
	const float velocity_bar_reserved = show_velocity_bar ? velocity_bar_height + velocity_bar_inset : 0.0f;
	const float note_label_font_size = fminf(NAUI_DPI(11.0f), (lane_height - velocity_bar_reserved) * (show_velocity_bar ? 0.9f : 0.8f));
	const float note_label_padding = NAUI_DPI(3.0f);
	const float note_rounding = NAUI_DPI(3.0f);
	const Leaf_Color note_label_color = LEAF_COLOR_WHITE;
	const Leaf_Color note_color = uph_midi_view_note_color();
	Leaf_Color velocity_track_color = uph_color_mix(note_color, LEAF_COLOR_BLACK, UPH_MIDI_NOTE_VELOCITY_TRACK_DARKEN_MIX);
	velocity_track_color.a = 150;
	const Leaf_Color velocity_fill_color = uph_color_mix(note_color, LEAF_COLOR_WHITE, UPH_MIDI_NOTE_VELOCITY_FILL_LIGHTEN_MIX);

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

		const float track_width = note_width - velocity_bar_inset * 2.0f;
		if (show_velocity_bar && track_width > 1.0f)
		{
			const float track_y = y_position + lane_height - velocity_bar_inset - velocity_bar_height;
			const float fill_width = track_width * (float)NAUI_MIN((int32_t)note->velocity, UPH_MIDI_EDITOR_VELOCITY_MAX) / (float)UPH_MIDI_EDITOR_VELOCITY_MAX;
			const float bar_rounding = velocity_bar_height * 0.5f;

			naui_fill_rect((Naui_Vec2) { note_x + velocity_bar_inset, track_y }, (Naui_Vec2) { track_width, velocity_bar_height }, velocity_track_color, bar_rounding, LEAF_CORNER_ALL);
			naui_fill_rect((Naui_Vec2) { note_x + velocity_bar_inset, track_y }, (Naui_Vec2) { fill_width, velocity_bar_height }, velocity_fill_color, fminf(bar_rounding, fill_width * 0.5f), LEAF_CORNER_ALL);
		}

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
				(Naui_Vec2) { note_x + note_label_padding, y_position + (lane_height - velocity_bar_reserved - note_label_font_size) * 0.5f },
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
