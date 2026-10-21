static float _uph_velocity_usable_height(Leaf_BoundingBox box)
{
	return fmaxf(1.0f, box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING) * 2.0f);
}

static float _uph_velocity_to_y(Leaf_BoundingBox box, float velocity)
{
	const float bottom = box.y + box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING);
	return bottom - _uph_velocity_usable_height(box) * (velocity / (float)UPH_MIDI_EDITOR_VELOCITY_MAX);
}

static int32_t _uph_velocity_from_y(Leaf_BoundingBox box, float y)
{
	const float bottom = box.y + box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING);
	const float t = (bottom - y) / _uph_velocity_usable_height(box);
	return NAUI_CLAMP((int32_t)lroundf(t * (float)UPH_MIDI_EDITOR_VELOCITY_MAX), UPH_MIDI_EDITOR_VELOCITY_MIN, UPH_MIDI_EDITOR_VELOCITY_MAX);
}

static uint8_t _uph_velocity_clamp(int32_t velocity)
{
	return (uint8_t)NAUI_CLAMP(velocity, UPH_MIDI_EDITOR_VELOCITY_MIN, UPH_MIDI_EDITOR_VELOCITY_MAX);
}

static float _uph_velocity_hit_radius(void)
{
	return NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS + 2.0f);
}

void uph_midi_velocity_init(Uph_VelocityLane *lane)
{
	lane->height = UPH_MIDI_EDITOR_VELOCITY_LANE_HEIGHT;
	lane->hovered_note = -1;
	lane->visible = true;
}

void uph_midi_velocity_cancel_drag(Uph_VelocityLane *lane)
{
	Uph_VelocityDragState *drag = &lane->drag;
	naui_list_free(drag->originals);
	drag->originals = NULL;
	drag->mode = UPH_VELOCITY_DRAG_NONE;
}

#pragma region Splitter

void uph_midi_velocity_update_splitter(Uph_MidiEditorData *editor, Leaf_ID splitter_id)
{
	Uph_VelocityLane *lane = &editor->velocity_lane;
	lane->splitter_hovered = leaf_hovered(splitter_id) && editor->panel_hovered;

	const bool busy = editor->note_input.drag.active || editor->note_input.marquee.active || lane->drag.mode != UPH_VELOCITY_DRAG_NONE;
	if (lane->splitter_hovered && !busy && naui_mouse_pressed(NAUI_MOUSE_LEFT))
	{
		lane->resizing = true;
		lane->resize_start_mouse_y = (float)naui_mouse_y();
		lane->resize_start_height = lane->height;
	}

	if (lane->splitter_hovered || lane->resizing)
		naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

	if (!lane->resizing)
		return;

	const float delta = (lane->resize_start_mouse_y - (float)naui_mouse_y()) / NAUI_DPI(1.0f);
	lane->height = NAUI_CLAMP(lane->resize_start_height + delta, UPH_MIDI_EDITOR_VELOCITY_LANE_MIN_HEIGHT, UPH_MIDI_EDITOR_VELOCITY_LANE_MAX_HEIGHT);

	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		lane->resizing = false;
}

#pragma endregion

#pragma region Input

static int32_t _uph_velocity_hit_test(const Uph_MidiEditorData *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern)
{
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();
	const float radius = _uph_velocity_hit_radius();

	int32_t best = -1;
	float best_score = FLT_MAX;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
	{
		const Uph_MidiNote *note = &pattern->notes[i];
		const float x = uph_midi_view_beat_to_x(&editor->view, box, note->start_beat);
		const float dx = fabsf(x - mouse_x);
		if (dx > radius)
			continue;

		const float score = dx + fabsf(_uph_velocity_to_y(box, (float)note->velocity) - mouse_y) * 0.25f - (note->selected ? 0.5f : 0.0f);
		if (score < best_score)
		{
			best_score = score;
			best = (int32_t)i;
		}
	}

	return best;
}

static void _uph_velocity_apply_handle(Uph_VelocityLane *lane, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	const Uph_VelocityDragState *drag = &lane->drag;
	const int32_t delta = _uph_velocity_from_y(box, (float)naui_mouse_y()) - drag->grab_mouse_velocity;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->originals); i++)
	{
		const Uph_NoteSnapshot *snapshot = &drag->originals[i];
		if (snapshot->index < (uint32_t)naui_list_len(pattern->notes))
			pattern->notes[snapshot->index].velocity = _uph_velocity_clamp((int32_t)snapshot->note.velocity + delta);
	}
}

static void _uph_velocity_apply_paint(const Uph_MidiEditorData *editor, Uph_VelocityLane *lane, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_VelocityDragState *drag = &lane->drag;
	const float mouse_x = (float)naui_mouse_x();
	const float radius = _uph_velocity_hit_radius();
	const float left = fminf(drag->last_mouse_x, mouse_x) - radius;
	const float right = fmaxf(drag->last_mouse_x, mouse_x) + radius;
	const uint8_t velocity = _uph_velocity_clamp(_uph_velocity_from_y(box, (float)naui_mouse_y()));

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->originals); i++)
	{
		const uint32_t index = drag->originals[i].index;
		if (index >= (uint32_t)naui_list_len(pattern->notes))
			continue;

		Uph_MidiNote *note = &pattern->notes[index];
		const float x = uph_midi_view_beat_to_x(&editor->view, box, note->start_beat);
		if (x >= left && x <= right)
			note->velocity = velocity;
	}

	drag->last_mouse_x = mouse_x;
}

static void _uph_velocity_begin(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern, int32_t hovered)
{
	Uph_VelocityLane *lane = &editor->velocity_lane;
	Uph_VelocityDragState *drag = &lane->drag;
	uph_midi_velocity_cancel_drag(lane);

	drag->pattern_index = uph_state.shared.selected_resource.index;
	drag->grab_mouse_velocity = _uph_velocity_from_y(box, (float)naui_mouse_y());
	drag->last_mouse_x = (float)naui_mouse_x();

	if (hovered >= 0)
	{
		const Uph_MidiNote *note = &pattern->notes[hovered];
		drag->mode = UPH_VELOCITY_DRAG_HANDLE;
		drag->grabbed_index = (uint32_t)hovered;

		if (note->selected && uph_note_selected_count(pattern) > 1)
			uph_note_collect_selected(pattern, &drag->originals);
		else
		{
			Uph_NoteSnapshot snapshot = { .index = (uint32_t)hovered, .note = *note };
			naui_list_push(drag->originals, snapshot);
		}

		return;
	}

	drag->mode = UPH_VELOCITY_DRAG_PAINT;
	const bool only_selected = uph_note_selected_count(pattern) > 0;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
	{
		if (only_selected && !pattern->notes[i].selected)
			continue;

		Uph_NoteSnapshot snapshot = { .index = i, .note = pattern->notes[i] };
		naui_list_push(drag->originals, snapshot);
	}
}

static void _uph_velocity_commit(Uph_ResourceIndex pattern_index, Uph_MidiPattern *pattern, Naui_List(Uph_NoteSnapshot) originals)
{
	uint32_t changed = 0;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(originals); i++)
	{
		const uint32_t index = originals[i].index;
		changed += index < (uint32_t)naui_list_len(pattern->notes) && pattern->notes[index].velocity != originals[i].note.velocity;
	}

	if (changed == 0)
		return;

	naui_action_group_start(UPH_ACTION_MIDI_NOTES_VELOCITY_GROUP);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(originals); i++)
	{
		const uint32_t index = originals[i].index;
		if (index >= (uint32_t)naui_list_len(pattern->notes) || pattern->notes[index].velocity == originals[i].note.velocity)
			continue;

		Uph_ActionNoteVelocity data = {
			.pattern_index = pattern_index,
			.note_index = index,
			.old_velocity = originals[i].note.velocity,
			.new_velocity = pattern->notes[index].velocity,
			.applied_live = true
		};
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_VELOCITY, data);
	}
	naui_action_group_end();
}

static void _uph_velocity_finish(Uph_MidiEditorData *editor, Uph_MidiPattern *pattern)
{
	Uph_VelocityDragState *drag = &editor->velocity_lane.drag;
	_uph_velocity_commit(drag->pattern_index, pattern, drag->originals);

	if (drag->mode == UPH_VELOCITY_DRAG_HANDLE && drag->grabbed_index < (uint32_t)naui_list_len(pattern->notes))
		editor->note_input.last_note_velocity = pattern->notes[drag->grabbed_index].velocity;

	uph_midi_velocity_cancel_drag(&editor->velocity_lane);
}

static void _uph_velocity_reset(Uph_MidiPattern *pattern, uint32_t note_index)
{
	Naui_List(Uph_NoteSnapshot) originals = NULL;
	if (pattern->notes[note_index].selected && uph_note_selected_count(pattern) > 1)
		uph_note_collect_selected(pattern, &originals);
	else
	{
		Uph_NoteSnapshot snapshot = { .index = note_index, .note = pattern->notes[note_index] };
		naui_list_push(originals, snapshot);
	}

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(originals); i++)
		pattern->notes[originals[i].index].velocity = UPH_MIDI_EDITOR_DEFAULT_NOTE_VELOCITY;

	_uph_velocity_commit(uph_state.shared.selected_resource.index, pattern, originals);
	naui_list_free(originals);
}

void uph_midi_velocity_update(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_VelocityLane *lane = &editor->velocity_lane;
	Uph_VelocityDragState *drag = &lane->drag;
	lane->hovered_note = -1;

	if (drag->mode != UPH_VELOCITY_DRAG_NONE && drag->pattern_index != uph_state.shared.selected_resource.index)
		uph_midi_velocity_cancel_drag(lane);

	if (drag->mode == UPH_VELOCITY_DRAG_NONE)
	{
		if (!lane->hovered || lane->resizing || editor->note_input.drag.active || editor->note_input.marquee.active)
			return;

		const int32_t hovered = _uph_velocity_hit_test(editor, box, pattern);
		lane->hovered_note = hovered;
		if (hovered >= 0)
			naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			_uph_velocity_begin(editor, box, pattern, hovered);
		else if (hovered >= 0 && naui_mouse_pressed(NAUI_MOUSE_RIGHT))
			_uph_velocity_reset(pattern, (uint32_t)hovered);

		if (drag->mode == UPH_VELOCITY_DRAG_NONE)
			return;
	}

	if (drag->mode == UPH_VELOCITY_DRAG_HANDLE)
	{
		lane->hovered_note = (int32_t)drag->grabbed_index;
		_uph_velocity_apply_handle(lane, box, pattern);
		naui_set_cursor(NAUI_CURSOR_RESIZE_NS);
	}
	else
	{
		_uph_velocity_apply_paint(editor, lane, box, pattern);
		naui_set_cursor(NAUI_CURSOR_CROSSHAIR);
	}

	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		_uph_velocity_finish(editor, pattern);
}

#pragma endregion

#pragma region Rendering

void uph_midi_velocity_render_header(Leaf_BoundingBox box)
{
	const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
	const float small_font_size = NAUI_DPI(10.0f);
	const float padding = NAUI_DPI(6.0f);
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	const Leaf_Color dim_color = naui_theme_color("uph_ui_text_disabled_color");

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, naui_theme_color("uph_toolbox_bg_color"), 0.0f, NAUI_CORNER_NONE);

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	naui_draw_text((Naui_Vec2) { box.x + padding, box.y + padding }, "Velocity", font_size, 0, text_color);

	static const int32_t marks[] = { 127, 64, 1 };
	for (uint32_t i = 0; i < (uint32_t)(sizeof(marks) / sizeof(marks[0])); i++)
	{
		char label[8];
		snprintf(label, sizeof(label), "%d", (int)marks[i]);
		const Naui_Vec2 size = naui_measure_text(label, (uint32_t)strlen(label), small_font_size, 0);
		const float y = _uph_velocity_to_y(box, (float)marks[i]) - size.y * 0.5f;
		if (y < box.y + padding + font_size)
			continue;

		naui_draw_text((Naui_Vec2) { box.x + box.width - size.x - padding, y }, label, small_font_size, 0, dim_color);
	}
	naui_pop_clip_rect();
}

static void _uph_velocity_render_note(const Uph_MidiEditorData *editor, Leaf_BoundingBox box, const Uph_MidiNote *note, bool hovered, Leaf_Color base_color)
{
	const float radius = NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS);
	const float x = uph_midi_view_beat_to_x(&editor->view, box, note->start_beat);
	const float width = (float)(note->length_beats * editor->view.zoom.x);
	if (x - radius > box.x + box.width || x + width < box.x - radius)
		return;

	const float y = _uph_velocity_to_y(box, (float)note->velocity);
	const Leaf_Color color = note->selected ? LEAF_COLOR_WHITE : uph_midi_view_velocity_color(base_color, note->velocity);
	Leaf_Color bar_color = color;
	bar_color.a = 70;

	naui_fill_rect((Naui_Vec2) { x, y - NAUI_DPI(1.0f) }, (Naui_Vec2) { width, NAUI_DPI(2.0f) }, bar_color, 0.0f, NAUI_CORNER_NONE);
	naui_draw_line((Naui_Vec2) { x, box.y + box.height }, (Naui_Vec2) { x, y }, color, NAUI_DPI(1.5f));
	naui_fill_rect((Naui_Vec2) { x - radius, y - radius }, (Naui_Vec2) { radius * 2.0f, radius * 2.0f }, color, radius, LEAF_CORNER_ALL);

	if (hovered)
	{
		naui_draw_rect(
			(Naui_Vec2) { x - radius - NAUI_DPI(2.0f), y - radius - NAUI_DPI(2.0f) },
			(Naui_Vec2) { radius * 2.0f + NAUI_DPI(4.0f), radius * 2.0f + NAUI_DPI(4.0f) },
			LEAF_COLOR_WHITE,
			NAUI_DPI(1.0f),
			radius + NAUI_DPI(2.0f),
			LEAF_CORNER_ALL,
			NAUI_SIDE_ALL
		);
	}
}

static void _uph_velocity_render_label(Leaf_BoundingBox box, float x, float y, int32_t velocity)
{
	char label[8];
	snprintf(label, sizeof(label), "%d", (int)velocity);

	const float font_size = NAUI_DPI(11.0f);
	const float padding = NAUI_DPI(3.0f);
	const Naui_Vec2 size = naui_measure_text(label, (uint32_t)strlen(label), font_size, 0);

	float label_x = x + NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS) + NAUI_DPI(4.0f);
	if (label_x + size.x + padding * 2.0f > box.x + box.width)
		label_x = x - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS) - NAUI_DPI(4.0f) - size.x - padding * 2.0f;

	const float label_y = NAUI_CLAMP(y - size.y * 0.5f - padding, box.y, box.y + box.height - size.y - padding * 2.0f);

	naui_fill_rect((Naui_Vec2) { label_x, label_y }, (Naui_Vec2) { size.x + padding * 2.0f, size.y + padding * 2.0f }, leaf_rgba(0, 0, 0, 170), NAUI_DPI(3.0f), LEAF_CORNER_ALL);
	naui_draw_text((Naui_Vec2) { label_x + padding, label_y + padding }, label, font_size, 0, LEAF_COLOR_WHITE);
}

void uph_midi_velocity_render(const Uph_MidiEditorData *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern)
{
	const Uph_VelocityLane *lane = &editor->velocity_lane;
	const Leaf_Color guide_color = naui_theme_color("uph_timeline_grid_subbeat_color");
	const Leaf_Color note_color = uph_midi_view_note_color();

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, naui_theme_color("uph_timeline_row_bg2_color"), 0.0f, NAUI_CORNER_NONE);
	uph_midi_grid_render_beats(box, &editor->view, editor->snap_resolution);

	static const float guides[] = { 32.0f, 64.0f, 96.0f, 127.0f };
	for (uint32_t i = 0; i < (uint32_t)(sizeof(guides) / sizeof(guides[0])); i++)
	{
		const float y = _uph_velocity_to_y(box, guides[i]);
		naui_draw_line((Naui_Vec2) { box.x, y }, (Naui_Vec2) { box.x + box.width, y }, guide_color, 1.0f);
	}

	if (!pattern)
		return;

	for (uint32_t pass = 0; pass < 2; pass++)
	{
		for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
		{
			const Uph_MidiNote *note = &pattern->notes[i];
			if (note->selected != (pass == 1))
				continue;

			_uph_velocity_render_note(editor, box, note, (int32_t)i == lane->hovered_note, note_color);
		}
	}

	if (lane->drag.mode == UPH_VELOCITY_DRAG_PAINT)
	{
		const int32_t velocity = _uph_velocity_from_y(box, (float)naui_mouse_y());
		naui_draw_line((Naui_Vec2) { box.x, _uph_velocity_to_y(box, (float)velocity) }, (Naui_Vec2) { box.x + box.width, _uph_velocity_to_y(box, (float)velocity) }, leaf_rgba(255, 255, 255, 60), 1.0f);
		_uph_velocity_render_label(box, (float)naui_mouse_x(), (float)naui_mouse_y(), velocity);
	}
	else if (lane->hovered_note >= 0 && lane->hovered_note < (int32_t)naui_list_len(pattern->notes))
	{
		const Uph_MidiNote *note = &pattern->notes[lane->hovered_note];
		_uph_velocity_render_label(box, uph_midi_view_beat_to_x(&editor->view, box, note->start_beat), _uph_velocity_to_y(box, (float)note->velocity), note->velocity);
	}
}

#pragma endregion
