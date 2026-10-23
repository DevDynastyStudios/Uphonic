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
	uph_grid_render_beats(box, view->zoom.x, view->scroll.x, uph_snap_division(snap));
}

void uph_midi_grid_render_ruler(Leaf_BoundingBox box, const Uph_MidiView *view, Uph_SnapResolution snap)
{
	uph_grid_render_ruler(box, view->zoom.x, view->scroll.x, uph_snap_division(snap));
}
