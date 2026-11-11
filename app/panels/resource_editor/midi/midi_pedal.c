static const char *const s_uph_pedal_labels[UPH_MIDI_PEDAL_COUNT] = { "Sustain", "Sostenuto", "Soft" };

static float _uph_pedal_usable_height(Leaf_BoundingBox box)
{
	return fmaxf(1.0f, box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING) * 2.0f);
}

static float _uph_pedal_to_y(Leaf_BoundingBox box, float value)
{
	const float bottom = box.y + box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING);
	return bottom - _uph_pedal_usable_height(box) * (value / 127.0f);
}

static uint8_t _uph_pedal_from_y(Leaf_BoundingBox box, float y)
{
	const float bottom = box.y + box.height - NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_PADDING);
	const float t = (bottom - y) / _uph_pedal_usable_height(box);
	return (uint8_t)NAUI_CLAMP((int32_t)lroundf(t * 127.0f), 0, 127);
}

void uph_midi_pedal_init(Uph_PedalLane *lane)
{
	lane->type = UPH_MIDI_PEDAL_SUSTAIN;
	lane->hovered_event = -1;
}

void uph_midi_pedal_cancel_drag(Uph_PedalLane *lane)
{
	Uph_PedalDragState *drag = &lane->drag;
	naui_list_free(drag->originals);
	drag->originals = NULL;
	drag->active = false;
	drag->moved = false;
	drag->created = false;
}

#pragma region Input

static int32_t _uph_pedal_hit_test(const Uph_MidiEditorData *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern)
{
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();
	const float radius = NAUI_DPI(UPH_MIDI_EDITOR_PEDAL_HANDLE_RADIUS + 3.0f);

	int32_t best = -1;
	float best_distance = FLT_MAX;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->pedals); i++)
	{
		const Uph_MidiPedalEvent *pedal = &pattern->pedals[i];
		if (pedal->type != editor->pedal_lane.type)
			continue;

		const float dx = uph_midi_view_beat_to_x(&editor->view, box, pedal->beat) - mouse_x;
		const float dy = _uph_pedal_to_y(box, (float)pedal->value) - mouse_y;
		const float distance = sqrtf(dx * dx + dy * dy);
		if (distance <= radius && distance < best_distance)
		{
			best_distance = distance;
			best = (int32_t)i;
		}
	}

	return best;
}

static uint32_t _uph_pedal_resort(Naui_List(Uph_MidiPedalEvent) pedals, uint32_t index)
{
	const uint32_t count = (uint32_t)naui_list_len(pedals);
	while (index > 0 && pedals[index - 1].beat > pedals[index].beat)
	{
		const Uph_MidiPedalEvent swap = pedals[index - 1];
		pedals[index - 1] = pedals[index];
		pedals[index] = swap;
		index--;
	}
	while (index + 1 < count && pedals[index + 1].beat < pedals[index].beat)
	{
		const Uph_MidiPedalEvent swap = pedals[index + 1];
		pedals[index + 1] = pedals[index];
		pedals[index] = swap;
		index++;
	}
	return index;
}

static bool _uph_pedal_lists_equal(Naui_List(Uph_MidiPedalEvent) a, Naui_List(Uph_MidiPedalEvent) b)
{
	if (naui_list_len(a) != naui_list_len(b))
		return false;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(a); i++)
	{
		if (a[i].beat != b[i].beat || a[i].value != b[i].value || a[i].type != b[i].type)
			return false;
	}

	return true;
}

static void _uph_pedal_commit(Uph_ResourceIndex pattern_index, Uph_MidiPattern *pattern, Naui_List(Uph_MidiPedalEvent) originals)
{
	if (_uph_pedal_lists_equal(originals, pattern->pedals))
	{
		naui_list_free(originals);
		return;
	}

	Uph_ActionPedalsEdit data = {
		.pattern_index = pattern_index,
		.old_pedals = originals,
		.new_pedals = naui_list_clone(pattern->pedals),
		.applied_live = true
	};

	if (!naui_action_execute_stack(UPH_ACTION_MIDI_PEDALS_EDIT, data))
	{
		naui_list_free(data.old_pedals);
		naui_list_free(data.new_pedals);
	}

	uph_state.shared.current_pattern_updated = true;
}

static double _uph_pedal_mouse_beat(const Uph_MidiEditorData *editor, Leaf_BoundingBox box)
{
	return uph_midi_view_x_to_beat(&editor->view, box, (float)naui_mouse_x());
}

static void _uph_pedal_begin(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern, int32_t hovered)
{
	Uph_PedalLane *lane = &editor->pedal_lane;
	Uph_PedalDragState *drag = &lane->drag;
	uph_midi_pedal_cancel_drag(lane);

	drag->originals = naui_list_clone(pattern->pedals);
	drag->pattern_index = uph_state.shared.selected_resource.index;
	drag->initial_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
	drag->active = true;

	const double mouse_beat = _uph_pedal_mouse_beat(editor, box);
	if (hovered >= 0)
	{
		drag->index = (uint32_t)hovered;
		drag->grab_beat_offset = mouse_beat - pattern->pedals[hovered].beat;
		return;
	}

	const Uph_MidiPedalEvent pedal = {
		.beat = fmax(0.0, uph_snap_beat_round(mouse_beat, editor->snap_resolution, editor->view.zoom.x)),
		.value = _uph_pedal_from_y(box, (float)naui_mouse_y()),
		.type = lane->type
	};

	naui_list_push(pattern->pedals, pedal);
	drag->index = _uph_pedal_resort(pattern->pedals, (uint32_t)naui_list_len(pattern->pedals) - 1);
	drag->grab_beat_offset = mouse_beat - pedal.beat;
	drag->created = true;
	drag->moved = true;
	uph_state.shared.current_pattern_updated = true;
}

static void _uph_pedal_apply_drag(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_PedalDragState *drag = &editor->pedal_lane.drag;
	if (drag->index >= (uint32_t)naui_list_len(pattern->pedals))
		return;

	if (!drag->moved)
	{
		const float dx = (float)naui_mouse_x() - drag->initial_mouse.x;
		const float dy = (float)naui_mouse_y() - drag->initial_mouse.y;
		if (fabsf(dx) < NAUI_DPI(UPH_MIDI_EDITOR_CLICK_THRESHOLD) && fabsf(dy) < NAUI_DPI(UPH_MIDI_EDITOR_CLICK_THRESHOLD))
			return;
		drag->moved = true;
	}

	const double beat = _uph_pedal_mouse_beat(editor, box) - drag->grab_beat_offset;
	Uph_MidiPedalEvent *pedal = &pattern->pedals[drag->index];
	pedal->beat = fmax(0.0, uph_snap_beat_round(beat, editor->snap_resolution, editor->view.zoom.x));
	pedal->value = _uph_pedal_from_y(box, (float)naui_mouse_y());
	drag->index = _uph_pedal_resort(pattern->pedals, drag->index);
	uph_state.shared.current_pattern_updated = true;
}

static void _uph_pedal_delete(Uph_MidiPattern *pattern, uint32_t index)
{
	Naui_List(Uph_MidiPedalEvent) originals = naui_list_clone(pattern->pedals);
	naui_list_remove(pattern->pedals, index);
	_uph_pedal_commit(uph_state.shared.selected_resource.index, pattern, originals);
}

void uph_midi_pedal_update(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_PedalLane *lane = &editor->pedal_lane;
	Uph_PedalDragState *drag = &lane->drag;
	lane->hovered_event = -1;

	if (drag->active && drag->pattern_index != uph_state.shared.selected_resource.index)
		uph_midi_pedal_cancel_drag(lane);

	if (!drag->active)
	{
		if (!lane->hovered || editor->velocity_lane.resizing || editor->note_input.drag.active || editor->note_input.marquee.active)
			return;

		const int32_t hovered = _uph_pedal_hit_test(editor, box, pattern);
		lane->hovered_event = hovered;
		naui_set_cursor(hovered >= 0 ? NAUI_CURSOR_RESIZE_ALL : NAUI_CURSOR_CROSSHAIR);

		if (hovered >= 0 && naui_mouse_pressed(NAUI_MOUSE_RIGHT))
		{
			_uph_pedal_delete(pattern, (uint32_t)hovered);
			lane->hovered_event = -1;
			return;
		}

		if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
			return;

		_uph_pedal_begin(editor, box, pattern, hovered);
	}

	_uph_pedal_apply_drag(editor, box, pattern);
	lane->hovered_event = (int32_t)drag->index;
	naui_set_cursor(NAUI_CURSOR_RESIZE_ALL);

	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
	{
		_uph_pedal_commit(drag->pattern_index, pattern, drag->originals);
		drag->originals = NULL;
		uph_midi_pedal_cancel_drag(lane);
	}
}

#pragma endregion

#pragma region Rendering

void uph_midi_pedal_render_header(Uph_MidiEditorData *editor, Leaf_BoundingBox box)
{
	const float font_size = NAUI_DPI(10.0f);
	const float padding = NAUI_DPI(6.0f);
	const float row_height = NAUI_DPI(15.0f);
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, naui_theme_color("uph_toolbox_bg_color"), 0.0f, NAUI_CORNER_NONE);

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
	{
		const float y = box.y + padding * 0.5f + row_height * (float)type;
		const bool active = editor->pedal_lane.type == type;
		const bool hovered = editor->panel_hovered && !editor->pedal_lane.drag.active
			&& mouse_x >= box.x && mouse_x < box.x + box.width && mouse_y >= y && mouse_y < y + row_height;

		if (active || hovered)
			naui_fill_rect((Naui_Vec2) { box.x + NAUI_DPI(3.0f), y }, (Naui_Vec2) { box.width - NAUI_DPI(6.0f), row_height }, naui_theme_color(active ? "uph_ui_frame_pressed_bg_color" : "uph_ui_frame_hovered_bg_color"), NAUI_DPI(3.0f), LEAF_CORNER_ALL);

		const char *label = s_uph_pedal_labels[type];
		const Naui_Vec2 size = naui_measure_text(label, (uint32_t)strlen(label), font_size, 0);
		naui_draw_text((Naui_Vec2) { box.x + box.width - size.x - padding, y + (row_height - size.y) * 0.5f }, label, font_size, 0, naui_theme_color(active || hovered ? "uph_ui_text_color" : "uph_ui_text_disabled_color"));

		if (hovered)
		{
			naui_set_cursor(NAUI_CURSOR_HAND);
			if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
				editor->pedal_lane.type = type;
		}
	}
	naui_pop_clip_rect();
}

static void _uph_pedal_render_steps(const Uph_MidiEditorData *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern, Uph_MidiPedalType type, Leaf_Color color, bool fill)
{
	const float end_x = uph_midi_view_beat_to_x(&editor->view, box, uph_midi_preview_loop_length(pattern));
	const float bottom = _uph_pedal_to_y(box, 0.0f);
	float x = box.x;
	float y = bottom;

	Leaf_Color fill_color = color;
	fill_color.a = 45;

	for (uint32_t i = 0; i <= (uint32_t)naui_list_len(pattern->pedals); i++)
	{
		const bool last = i == (uint32_t)naui_list_len(pattern->pedals);
		if (!last && pattern->pedals[i].type != type)
			continue;

		const float next_x = last ? fmaxf(end_x, x) : uph_midi_view_beat_to_x(&editor->view, box, pattern->pedals[i].beat);
		const float clipped_x = fmaxf(x, box.x);
		const float clipped_next = fminf(next_x, box.x + box.width);
		if (clipped_next > clipped_x)
		{
			if (fill && y < bottom)
				naui_fill_rect((Naui_Vec2) { clipped_x, y }, (Naui_Vec2) { clipped_next - clipped_x, bottom - y }, fill_color, 0.0f, NAUI_CORNER_NONE);
			naui_draw_line((Naui_Vec2) { clipped_x, y }, (Naui_Vec2) { clipped_next, y }, color, NAUI_DPI(1.5f));
		}

		if (last)
			break;

		const float next_y = _uph_pedal_to_y(box, (float)pattern->pedals[i].value);
		if (next_x >= box.x && next_x <= box.x + box.width)
			naui_draw_line((Naui_Vec2) { next_x, y }, (Naui_Vec2) { next_x, next_y }, color, NAUI_DPI(1.5f));

		x = next_x;
		y = next_y;
	}
}

static void _uph_pedal_render_label(Leaf_BoundingBox box, float x, float y, int32_t value)
{
	char label[8];
	snprintf(label, sizeof(label), "%d", (int)value);

	const float font_size = NAUI_DPI(11.0f);
	const float padding = NAUI_DPI(3.0f);
	const Naui_Vec2 size = naui_measure_text(label, (uint32_t)strlen(label), font_size, 0);

	float label_x = x + NAUI_DPI(UPH_MIDI_EDITOR_PEDAL_HANDLE_RADIUS) + NAUI_DPI(4.0f);
	if (label_x + size.x + padding * 2.0f > box.x + box.width)
		label_x = x - NAUI_DPI(UPH_MIDI_EDITOR_PEDAL_HANDLE_RADIUS) - NAUI_DPI(4.0f) - size.x - padding * 2.0f;

	const float label_y = NAUI_CLAMP(y - size.y * 0.5f - padding, box.y, box.y + box.height - size.y - padding * 2.0f);

	naui_fill_rect((Naui_Vec2) { label_x, label_y }, (Naui_Vec2) { size.x + padding * 2.0f, size.y + padding * 2.0f }, leaf_rgba(0, 0, 0, 170), NAUI_DPI(3.0f), LEAF_CORNER_ALL);
	naui_draw_text((Naui_Vec2) { label_x + padding, label_y + padding }, label, font_size, 0, LEAF_COLOR_WHITE);
}

void uph_midi_pedal_render(const Uph_MidiEditorData *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern)
{
	const Uph_PedalLane *lane = &editor->pedal_lane;
	const Leaf_Color guide_color = naui_theme_color("uph_timeline_grid_subbeat_color");
	const Leaf_Color color = uph_midi_view_note_color();

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, naui_theme_color("uph_midi_editor_scale_lane_color"), 0.0f, NAUI_CORNER_NONE);
	uph_midi_grid_render_beats(box, &editor->view, editor->snap_resolution);

	static const float guides[] = { 0.0f, 64.0f, 127.0f };
	for (uint32_t i = 0; i < (uint32_t)(sizeof(guides) / sizeof(guides[0])); i++)
	{
		const float y = _uph_pedal_to_y(box, guides[i]);
		naui_draw_line((Naui_Vec2) { box.x, y }, (Naui_Vec2) { box.x + box.width, y }, guide_color, 1.0f);
	}

	if (!pattern)
		return;

	Leaf_Color dim_color = color;
	dim_color.a = 60;
	for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
	{
		if (type != lane->type)
			_uph_pedal_render_steps(editor, box, pattern, type, dim_color, false);
	}
	_uph_pedal_render_steps(editor, box, pattern, lane->type, color, true);

	const float radius = NAUI_DPI(UPH_MIDI_EDITOR_PEDAL_HANDLE_RADIUS);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->pedals); i++)
	{
		const Uph_MidiPedalEvent *pedal = &pattern->pedals[i];
		if (pedal->type != lane->type)
			continue;

		const float x = uph_midi_view_beat_to_x(&editor->view, box, pedal->beat);
		if (x + radius < box.x || x - radius > box.x + box.width)
			continue;

		const float y = _uph_pedal_to_y(box, (float)pedal->value);
		const bool hovered = (int32_t)i == lane->hovered_event;
		naui_fill_rect((Naui_Vec2) { x - radius, y - radius }, (Naui_Vec2) { radius * 2.0f, radius * 2.0f }, hovered ? LEAF_COLOR_WHITE : color, radius, LEAF_CORNER_ALL);

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
			_uph_pedal_render_label(box, x, y, pedal->value);
		}
	}
}

#pragma endregion
