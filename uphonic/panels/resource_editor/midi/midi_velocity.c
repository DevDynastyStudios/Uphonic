#define UPH_MIDI_EDITOR_DEFAULT_NOTE_VELOCITY 100
#define UPH_MIDI_EDITOR_VELOCITY_LANE_RATIO 0.25f
#define UPH_MIDI_EDITOR_VELOCITY_LANE_MAX_RATIO 0.7f
#define UPH_MIDI_EDITOR_VELOCITY_LANE_MIN_HEIGHT 48
#define UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS 4
#define UPH_MIDI_EDITOR_VELOCITY_PADDING 6
#define UPH_MIDI_EDITOR_VELOCITY_MIN 1

#define UPH_MIDI_EDITOR_VELOCITY_HEADER_FONT_SIZE 11
#define UPH_MIDI_EDITOR_VELOCITY_HEADER_TICK_WIDTH 4
#define UPH_MIDI_EDITOR_VELOCITY_HEADER_TEXT_MIX 0.45f
#define UPH_MIDI_EDITOR_VELOCITY_COLUMN_ALPHA 22
#define UPH_MIDI_EDITOR_VELOCITY_COLUMN_ACTIVE_ALPHA 40

static const float uph__velocity_guides[] = { 32.0f, 64.0f, 96.0f, 127.0f };

static float uph__velocity_usable_height(Leaf_BoundingBox box)
{
	return fmaxf(1.0f, box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING) * 2.0f);
}

static float uph__velocity_to_y(Leaf_BoundingBox box, float velocity)
{
	const float bottom = box.y + box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING);
	return bottom - uph__velocity_usable_height(box) * (velocity / (float)UPH_MIDI_EDITOR_VELOCITY_MAX);
}

static int32_t uph__velocity_from_y(Leaf_BoundingBox box, float y)
{
	const float bottom = box.y + box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING);
	const float t = (bottom - y) / uph__velocity_usable_height(box);
	return NAUI_CLAMP((int32_t)lroundf(t * (float)UPH_MIDI_EDITOR_VELOCITY_MAX), UPH_MIDI_EDITOR_VELOCITY_MIN, UPH_MIDI_EDITOR_VELOCITY_MAX);
}

static uint8_t uph__velocity_clamp(int32_t velocity)
{
	return (uint8_t)NAUI_CLAMP(velocity, UPH_MIDI_EDITOR_VELOCITY_MIN, UPH_MIDI_EDITOR_VELOCITY_MAX);
}

static float uph__velocity_hit_radius(void)
{
	return NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS + 2);
}

void uph_midi_velocity_init(Uph_VelocityLane *lane)
{
	lane->height_ratio = UPH_MIDI_EDITOR_VELOCITY_LANE_RATIO;
	lane->hovered_note = -1;
	lane->visible = false;
}

static inline float uph__velocity_lane_min_ratio(const Uph_VelocityLane *lane)
{
	return fminf((float)NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_LANE_MIN_HEIGHT) / fmaxf(lane->area_height, 1.0f), UPH_MIDI_EDITOR_VELOCITY_LANE_MAX_RATIO);
}

float uph_midi_velocity_lane_height(const Uph_VelocityLane *lane)
{
	const float ratio = NAUI_CLAMP(lane->height_ratio, uph__velocity_lane_min_ratio(lane), UPH_MIDI_EDITOR_VELOCITY_LANE_MAX_RATIO);
	return ratio * lane->area_height;
}

void uph_midi_velocity_cancel_drag(Uph_VelocityLane *lane)
{
	Uph_VelocityDragState *drag = &lane->drag;
	naui_list_free(drag->originals);
	drag->originals = NULL;
	drag->mode = UPH_VELOCITY_DRAG_NONE;
}

#pragma region Splitter

void uph_midi_velocity_update_splitter(Uph_MidiEditorState *editor, bool handle_hovered)
{
	Uph_VelocityLane *lane = &editor->velocity_lane;
	lane->splitter_hovered = handle_hovered && editor->panel_hovered;

	const bool busy = editor->note_input.drag.active || editor->note_input.marquee.active || lane->drag.mode != UPH_VELOCITY_DRAG_NONE || editor->pedal_lane.drag.active;
	if (lane->splitter_hovered && !busy && naui_mouse_pressed(NAUI_MOUSE_LEFT))
	{
		lane->resizing = true;
		lane->resize_start_mouse_y = (float)naui_mouse_y();
		lane->resize_start_ratio = uph_midi_velocity_lane_height(lane) / fmaxf(lane->area_height, 1.0f);
	}

	if (lane->splitter_hovered || lane->resizing)
		naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

	if (!lane->resizing)
		return;

	const float delta = (lane->resize_start_mouse_y - (float)naui_mouse_y()) / fmaxf(lane->area_height, 1.0f);
	lane->height_ratio = NAUI_CLAMP(lane->resize_start_ratio + delta, uph__velocity_lane_min_ratio(lane), UPH_MIDI_EDITOR_VELOCITY_LANE_MAX_RATIO);

	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		lane->resizing = false;
}

#pragma endregion

#pragma region Input

static int32_t uph__velocity_hit_test(const Uph_MidiEditorState *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern)
{
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();
	const float radius = uph__velocity_hit_radius();

	int32_t best = -1;
	float best_score = FLT_MAX;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
	{
		const Uph_MidiNote *note = &pattern->notes[i];
		const float x = uph_midi_view_beat_to_x(&editor->view, box, note->start_beat);
		const float dx = fabsf(x - mouse_x);
		if (dx > radius)
			continue;

		const float score = dx + fabsf(uph__velocity_to_y(box, (float)note->velocity) - mouse_y) * 0.25f - (note->selected ? 0.5f : 0.0f);
		if (score < best_score)
		{
			best_score = score;
			best = (int32_t)i;
		}
	}

	return best;
}

static void uph__velocity_apply_handle(Uph_VelocityLane *lane, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	const Uph_VelocityDragState *drag = &lane->drag;
	const int32_t delta = uph__velocity_from_y(box, (float)naui_mouse_y()) - drag->grab_mouse_velocity;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->originals); i++)
	{
		const Uph_NoteSnapshot *snapshot = &drag->originals[i];
		if (snapshot->index < (uint32_t)naui_list_len(pattern->notes))
			pattern->notes[snapshot->index].velocity = uph__velocity_clamp((int32_t)snapshot->note.velocity + delta);
	}
}

static void uph__velocity_apply_paint(const Uph_MidiEditorState *editor, Uph_VelocityLane *lane, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_VelocityDragState *drag = &lane->drag;
	const float mouse_x = (float)naui_mouse_x();
	const float radius = uph__velocity_hit_radius();
	const float left = fminf(drag->last_mouse_x, mouse_x) - radius;
	const float right = fmaxf(drag->last_mouse_x, mouse_x) + radius;
	const uint8_t velocity = uph__velocity_clamp(uph__velocity_from_y(box, (float)naui_mouse_y()));

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

static void uph__velocity_begin(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern, int32_t hovered)
{
	Uph_VelocityLane *lane = &editor->velocity_lane;
	Uph_VelocityDragState *drag = &lane->drag;
	uph_midi_velocity_cancel_drag(lane);

	drag->pattern_index = uph_state.shared.selected_resource.index;
	drag->grab_mouse_velocity = uph__velocity_from_y(box, (float)naui_mouse_y());
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

static void uph__velocity_commit(Uph_ResourceIndex pattern_index, Uph_MidiPattern *pattern, Naui_List(Uph_NoteSnapshot) originals)
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

static void uph__velocity_finish(Uph_MidiEditorState *editor, Uph_MidiPattern *pattern)
{
	Uph_VelocityDragState *drag = &editor->velocity_lane.drag;
	uph__velocity_commit(drag->pattern_index, pattern, drag->originals);

	if (drag->mode == UPH_VELOCITY_DRAG_HANDLE && drag->grabbed_index < (uint32_t)naui_list_len(pattern->notes))
		editor->note_input.last_note_velocity = pattern->notes[drag->grabbed_index].velocity;

	uph_midi_velocity_cancel_drag(&editor->velocity_lane);
}

static void uph__velocity_reset(Uph_MidiPattern *pattern, uint32_t note_index)
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

	uph__velocity_commit(uph_state.shared.selected_resource.index, pattern, originals);
	naui_list_free(originals);
}

void uph_midi_velocity_update(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
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

		const int32_t hovered = uph__velocity_hit_test(editor, box, pattern);
		lane->hovered_note = hovered;
		if (hovered >= 0)
			naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			uph__velocity_begin(editor, box, pattern, hovered);
		else if (hovered >= 0 && naui_mouse_pressed(NAUI_MOUSE_RIGHT))
			uph__velocity_reset(pattern, (uint32_t)hovered);

		if (drag->mode == UPH_VELOCITY_DRAG_NONE)
			return;
	}

	if (drag->mode == UPH_VELOCITY_DRAG_HANDLE)
	{
		lane->hovered_note = (int32_t)drag->grabbed_index;
		uph__velocity_apply_handle(lane, box, pattern);
		naui_set_cursor(NAUI_CURSOR_RESIZE_NS);
	}
	else
	{
		uph__velocity_apply_paint(editor, lane, box, pattern);
		naui_set_cursor(NAUI_CURSOR_CROSSHAIR);
	}

	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		uph__velocity_finish(editor, pattern);
}

#pragma endregion

#pragma region Rendering

void uph_midi_velocity_render_header(Leaf_BoundingBox box)
{
	const int32_t small_font_size = NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HEADER_FONT_SIZE);
	const int32_t tick_width = NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HEADER_TICK_WIDTH);
	const int32_t padding = NAUI_DPI(6) + tick_width;
	const Leaf_Color dim_color = uph_color_mix(uph_state.colors.ui_text, uph_state.colors.ui_text_disabled, UPH_MIDI_EDITOR_VELOCITY_HEADER_TEXT_MIX);

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, uph_state.colors.midi_editor_toolbox_bg, 0.0f, NAUI_CORNER_NONE);

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	static const int32_t marks[] = { 127, 64, 1 };
	for (uint32_t i = 0; i < (uint32_t)(sizeof(marks) / sizeof(marks[0])); i++)
	{
		char label[8];
		snprintf(label, sizeof(label), "%d", (int)marks[i]);
		const Naui_Vec2 size = naui_measure_text(label, (uint32_t)strlen(label), small_font_size);
		const float y = NAUI_CLAMP(uph__velocity_to_y(box, (float)marks[i]) - size.y * 0.5f, box.y, box.y + box.height - size.y);

		naui_draw_text((Naui_Vec2) { box.x + box.width - size.x - padding, y }, label, small_font_size, dim_color);
	}

	for (uint32_t i = 0; i < (uint32_t)(sizeof(uph__velocity_guides) / sizeof(uph__velocity_guides[0])); i++)
	{
		const float y = uph__velocity_to_y(box, uph__velocity_guides[i]);
		naui_fill_rect((Naui_Vec2) { box.x + box.width - tick_width, y }, (Naui_Vec2) { tick_width, 1.0f }, uph_state.colors.ui_text_disabled, 0.0f, NAUI_CORNER_NONE);
	}
	naui_pop_clip_rect();
}

static void uph__velocity_render_note(const Uph_MidiEditorState *editor, Leaf_BoundingBox box, const Uph_MidiNote *note, bool hovered, Leaf_Color base_color)
{
	const int32_t radius = NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS);
	const float x = uph_midi_view_beat_to_x(&editor->view, box, note->start_beat);
	const float width = (float)(note->length_beats * editor->view.zoom.x);
	if (x - radius > box.x + box.width || x + width < box.x - radius)
		return;

	const float y = uph__velocity_to_y(box, (float)note->velocity);
	const Leaf_Color color = note->selected ? uph_state.colors.midi_editor_velocity_highlight : uph_midi_view_velocity_color(base_color, note->velocity);
	Leaf_Color bar_color = color;
	bar_color.a = 70;
	Leaf_Color column_color = color;
	column_color.a = note->selected || hovered ? UPH_MIDI_EDITOR_VELOCITY_COLUMN_ACTIVE_ALPHA : UPH_MIDI_EDITOR_VELOCITY_COLUMN_ALPHA;

	naui_fill_rect((Naui_Vec2) { x, y }, (Naui_Vec2) { width, box.y + box.height - y }, column_color, 0.0f, NAUI_CORNER_NONE);
	naui_fill_rect((Naui_Vec2) { x, y - NAUI_DPI(1) }, (Naui_Vec2) { width, NAUI_DPI(2) }, bar_color, 0.0f, NAUI_CORNER_NONE);
	naui_draw_line((Naui_Vec2) { x, box.y + box.height }, (Naui_Vec2) { x, y }, color, NAUI_DPI(1.5f));
	naui_fill_rect((Naui_Vec2) { x - radius, y - radius }, (Naui_Vec2) { radius * 2.0f, radius * 2.0f }, color, radius, LEAF_CORNER_ALL);

	if (hovered)
	{
		naui_draw_rect(
			(Naui_Vec2) { x - radius - NAUI_DPI(2), y - radius - NAUI_DPI(2) },
			(Naui_Vec2) { radius * 2.0f + NAUI_DPI(4), radius * 2.0f + NAUI_DPI(4) },
			uph_state.colors.midi_editor_velocity_highlight,
			NAUI_DPI(1),
			radius + NAUI_DPI(2),
			LEAF_CORNER_ALL,
			NAUI_SIDE_ALL
		);
	}
}

static void uph__velocity_render_label(Leaf_BoundingBox box, float x, float y, int32_t velocity)
{
	char label[8];
	snprintf(label, sizeof(label), "%d", (int)velocity);

	const int32_t font_size = NAUI_DPI(11);
	const int32_t padding = NAUI_DPI(3);
	const Naui_Vec2 size = naui_measure_text(label, (uint32_t)strlen(label), font_size);

	float label_x = x + NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS) + NAUI_DPI(4);
	if (label_x + size.x + padding * 2.0f > box.x + box.width)
		label_x = x - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS) - NAUI_DPI(4) - size.x - padding * 2.0f;

	const float label_y = NAUI_CLAMP(y - size.y * 0.5f - padding, box.y, box.y + box.height - size.y - padding * 2.0f);

	naui_fill_rect((Naui_Vec2) { label_x, label_y }, (Naui_Vec2) { size.x + padding * 2.0f, size.y + padding * 2.0f }, leaf_rgba(0, 0, 0, 170), NAUI_DPI(3), LEAF_CORNER_ALL);
	naui_draw_text((Naui_Vec2) { label_x + padding, label_y + padding }, label, font_size, LEAF_COLOR_WHITE);
}

void uph_midi_velocity_render(const Uph_MidiEditorState *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern)
{
	const Uph_VelocityLane *lane = &editor->velocity_lane;
	const Leaf_Color guide_color = uph_state.colors.midi_editor_grid_subbeat;
	const Leaf_Color note_color = uph_midi_view_note_color();

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, uph_state.colors.midi_editor_scale_lane, 0.0f, NAUI_CORNER_NONE);
	uph_midi_grid_render_beats(box, &editor->view, editor->snap_resolution);

	for (uint32_t i = 0; i < (uint32_t)(sizeof(uph__velocity_guides) / sizeof(uph__velocity_guides[0])); i++)
	{
		const float y = uph__velocity_to_y(box, uph__velocity_guides[i]);
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

			uph__velocity_render_note(editor, box, note, (int32_t)i == lane->hovered_note, note_color);
		}
	}

	if (lane->drag.mode == UPH_VELOCITY_DRAG_PAINT)
	{
		const int32_t velocity = uph__velocity_from_y(box, (float)naui_mouse_y());
		Leaf_Color paint_guide_color = uph_state.colors.midi_editor_velocity_highlight;
		paint_guide_color.a = 60;
		naui_draw_line((Naui_Vec2) { box.x, uph__velocity_to_y(box, (float)velocity) }, (Naui_Vec2) { box.x + box.width, uph__velocity_to_y(box, (float)velocity) }, paint_guide_color, 1.0f);
		uph__velocity_render_label(box, (float)naui_mouse_x(), (float)naui_mouse_y(), velocity);
	}
	else if (lane->hovered_note >= 0 && lane->hovered_note < (int32_t)naui_list_len(pattern->notes))
	{
		const Uph_MidiNote *note = &pattern->notes[lane->hovered_note];
		uph__velocity_render_label(box, uph_midi_view_beat_to_x(&editor->view, box, note->start_beat), uph__velocity_to_y(box, (float)note->velocity), note->velocity);
	}
}

#pragma endregion
