static Uph_NoteInteractionMode _uph_note_input_classify_hover(Naui_Vec4 hover_box, float mouse_x)
{
	if (mouse_x <= hover_box.x + UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH)
		return UPH_NOTE_INTERACTION_RESIZE_LEFT;

	if (mouse_x >= hover_box.x + hover_box.z - UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH)
		return UPH_NOTE_INTERACTION_RESIZE_RIGHT;
		
	return UPH_NOTE_INTERACTION_MOVE;
}

#pragma region Note Drag Actions

void uph_midi_note_input_cancel_drag(Uph_MidiEditorData *editor)
{
	Uph_DraggingNoteState *drag = &editor->note_input.drag;
	naui_list_free(drag->group_notes);
	drag->group_notes = NULL;
	drag->active = false;
	drag->creating = false;
	drag->group = false;
	drag->mode = UPH_NOTE_INTERACTION_NONE;
}

static bool _uph_note_input_geometry_equal(const Uph_MidiNote *a, const Uph_MidiNote *b)
{
	return a->start_beat == b->start_beat && a->length_beats == b->length_beats && a->key_number == b->key_number;
}

static void _uph_note_input_begin_drag(Uph_MidiEditorData *editor, Uph_MidiPattern *pattern, uint32_t note_index, Uph_NoteInteractionMode mode, Leaf_BoundingBox box)
{
	Uph_MidiNoteInput *input = &editor->note_input;
	Uph_DraggingNoteState *drag = &input->drag;
	Uph_MidiNote *note = &pattern->notes[note_index];

	naui_list_free(drag->group_notes);
	drag->group_notes = NULL;

	drag->active = true;
	drag->creating = false;
	drag->group = false;
	drag->additive = false;
	drag->note_index = note_index;
	drag->pattern_index = uph_state.shared.selected_resource.index;
	drag->mode = mode;
	drag->initial_start_beat = note->start_beat;
	drag->initial_length_beats = note->length_beats;
	drag->initial_key_number = note->key_number;

	const double mouse_beat = uph_midi_view_x_to_beat(&editor->view, box, (float)naui_mouse_x());
	drag->initial_drag_beat_offset = note->start_beat - mouse_beat;

	const int32_t mouse_row_at_grab = uph_midi_view_y_to_row(&editor->view, box, (float)naui_mouse_y());
	const int32_t note_row = uph_note_map_row(&editor->note_map, note->key_number);
	drag->initial_drag_key_offset = note_row - mouse_row_at_grab;

	if (editor->current_action_mode == UPH_ACTION_SELECT)
	{
		drag->additive = uph_ui_ctrl_down() || uph_ui_shift_down();
		if (drag->additive)
		{
			note->selected = !note->selected;
			if (!note->selected)
			{
				drag->active = false;
				drag->mode = UPH_NOTE_INTERACTION_NONE;
				return;
			}
		}
		else if (!note->selected)
		{
			uph_note_select_all(pattern, false);
			note->selected = true;
		}

		drag->group = true;
		uph_note_collect_selected(pattern, &drag->group_notes);
	}

	input->last_note_length = note->length_beats;
	input->last_note_velocity = note->velocity;

	uph_state.shared.current_pattern_updated = true;
}

static void _uph_note_input_apply_group_drag(Uph_MidiEditorData *editor, Uph_MidiPattern *pattern, const Uph_MidiNote *grabbed)
{
	const Uph_DraggingNoteState *drag = &editor->note_input.drag;
	const double division = uph_snap_division(editor->snap_resolution);

	double delta_start = grabbed->start_beat - drag->initial_start_beat;
	double delta_length = grabbed->length_beats - drag->initial_length_beats;
	int32_t delta_key = (int32_t)grabbed->key_number - (int32_t)drag->initial_key_number;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_notes); i++)
	{
		const Uph_MidiNote *initial = &drag->group_notes[i].note;
		if (drag->mode == UPH_NOTE_INTERACTION_MOVE)
		{
			delta_start = fmax(delta_start, -initial->start_beat);
			delta_key = NAUI_MAX(delta_key, -(int32_t)initial->key_number);
			delta_key = NAUI_MIN(delta_key, (UPH_NOTE_MAP_KEY_COUNT - 1) - (int32_t)initial->key_number);
		}
		else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_LEFT)
		{
			delta_start = fmax(delta_start, -initial->start_beat);
			delta_start = fmin(delta_start, initial->length_beats - division);
		}
		else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_RIGHT)
		{
			delta_length = fmax(delta_length, division - initial->length_beats);
		}
	}

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_notes); i++)
	{
		const Uph_NoteSnapshot *snapshot = &drag->group_notes[i];
		if (snapshot->index >= (uint32_t)naui_list_len(pattern->notes))
			continue;

		Uph_MidiNote *note = &pattern->notes[snapshot->index];
		if (drag->mode == UPH_NOTE_INTERACTION_MOVE)
		{
			note->start_beat = snapshot->note.start_beat + delta_start;
			note->key_number = (uint8_t)((int32_t)snapshot->note.key_number + delta_key);
		}
		else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_LEFT)
		{
			note->start_beat = snapshot->note.start_beat + delta_start;
			note->length_beats = snapshot->note.length_beats - delta_start;
		}
		else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_RIGHT)
		{
			note->length_beats = snapshot->note.length_beats + delta_length;
		}
	}
}

static void _uph_note_input_finish_drag(Uph_MidiEditorData *editor, Uph_MidiPattern *pattern)
{
	Uph_DraggingNoteState *drag = &editor->note_input.drag;
	const char *transform_action = drag->mode == UPH_NOTE_INTERACTION_MOVE ? UPH_ACTION_MIDI_NOTE_MOVE : UPH_ACTION_MIDI_NOTE_RESIZE;

	if (drag->creating)
	{
		if (drag->note_index < (uint32_t)naui_list_len(pattern->notes))
		{
			Uph_ActionNoteCreate data = {
				.pattern_index = drag->pattern_index,
				.note_index = drag->note_index,
				.note = pattern->notes[drag->note_index],
				.applied_live = true
			};
			naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_CREATE, data);
		}
	}
	else if (drag->group)
	{
		bool changed = false;
		naui_action_group_start(drag->mode == UPH_NOTE_INTERACTION_MOVE ? UPH_ACTION_MIDI_NOTES_MOVE_GROUP : UPH_ACTION_MIDI_NOTES_RESIZE_GROUP);

		for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_notes); i++)
		{
			const Uph_NoteSnapshot *snapshot = &drag->group_notes[i];
			if (snapshot->index >= (uint32_t)naui_list_len(pattern->notes))
				continue;

			const Uph_MidiNote *note = &pattern->notes[snapshot->index];
			if (_uph_note_input_geometry_equal(note, &snapshot->note))
				continue;

			Uph_ActionNoteTransform data = {
				.pattern_index = drag->pattern_index, .note_index = snapshot->index,
				.old_start = snapshot->note.start_beat, .old_length = snapshot->note.length_beats, .old_key = snapshot->note.key_number,
				.new_start = note->start_beat, .new_length = note->length_beats, .new_key = note->key_number,
				.applied_live = true
			};

			naui_action_execute_stack(transform_action, data);
			changed = true;
		}

		naui_action_group_end();
		if (!changed && !drag->additive && drag->note_index < (uint32_t)naui_list_len(pattern->notes))
		{
			uph_note_select_all(pattern, false);
			pattern->notes[drag->note_index].selected = true;
		}
	}
	else if (drag->note_index < (uint32_t)naui_list_len(pattern->notes))
	{
		const Uph_MidiNote *note = &pattern->notes[drag->note_index];
		const bool changed = note->start_beat != drag->initial_start_beat || note->length_beats != drag->initial_length_beats || note->key_number != drag->initial_key_number;
		if (changed)
		{
			Uph_ActionNoteTransform data = {
				.pattern_index = drag->pattern_index, .note_index = drag->note_index,
				.old_start = drag->initial_start_beat, .old_length = drag->initial_length_beats, .old_key = drag->initial_key_number,
				.new_start = note->start_beat, .new_length = note->length_beats, .new_key = note->key_number,
				.applied_live = true
			};
			naui_action_execute_stack(transform_action, data);
		}
	}

	uph_midi_note_input_cancel_drag(editor);
}

#pragma endregion

static void _uph_note_input_update_drag(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_MidiNoteInput *input = &editor->note_input;
	Uph_DraggingNoteState *drag = &input->drag;

	if (drag->active && (drag->pattern_index != uph_state.shared.selected_resource.index || drag->note_index >= (uint32_t)naui_list_len(pattern->notes)))
		uph_midi_note_input_cancel_drag(editor);

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
	{
		Uph_MidiNote *note = &pattern->notes[i];
		bool is_dragging_this_note = drag->active && drag->note_index == i;

		if (is_dragging_this_note)
		{
			double mouse_beat = uph_midi_view_x_to_beat(&editor->view, box, (float)naui_mouse_x());

			if (drag->mode == UPH_NOTE_INTERACTION_MOVE)
			{
				const double snapped = uph_snap_beat_floor(mouse_beat + drag->initial_drag_beat_offset, editor->snap_resolution);
				note->start_beat = fmax(0.0, snapped);

				const int32_t mouse_row_now = uph_midi_view_y_to_row(&editor->view, box, (float)naui_mouse_y());
				const int32_t new_row = mouse_row_now + drag->initial_drag_key_offset;
				const int32_t new_key = uph_note_map_key_at_row(&editor->note_map, new_row);
				note->key_number = (uint8_t)uph_note_map_clamp_key(&editor->note_map, new_key);
				naui_set_cursor(NAUI_CURSOR_HAND);
			}
			else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_LEFT)
			{
				const double division = uph_snap_division(editor->snap_resolution);

				double new_start = fmax(0.0, uph_snap_beat_round(mouse_beat, editor->snap_resolution));
				double end_beat = drag->initial_start_beat + drag->initial_length_beats;
				new_start = fmin(new_start, end_beat - division);

				note->start_beat = new_start;
				note->length_beats = end_beat - new_start;
				input->last_note_length = note->length_beats;
				input->last_note_velocity = note->velocity;
				uph_state.shared.current_pattern_updated = true;
				naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
			}
			else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_RIGHT)
			{
				const double division = uph_snap_division(editor->snap_resolution);
				const double snapped_end = uph_snap_beat_round(note->start_beat + (mouse_beat - note->start_beat), editor->snap_resolution);
				double new_length = snapped_end - note->start_beat;
				new_length = fmax(division, new_length);

				note->length_beats = new_length;
				input->last_note_length = note->length_beats;
				input->last_note_velocity = note->velocity;
				uph_state.shared.current_pattern_updated = true;
				naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
			}

			if (drag->group)
				_uph_note_input_apply_group_drag(editor, pattern, note);

			if (naui_mouse_released(NAUI_MOUSE_LEFT))
				_uph_note_input_finish_drag(editor, pattern);

			return;
		}

		if (!editor->lanes_hovered)
			continue;

		if (!drag->active)
		{
			Naui_Vec4 hover_box = uph_midi_view_note_box(&editor->view, &editor->note_map, box, note);

			if (hover_box.y + hover_box.w < box.y || hover_box.y > box.y + box.height)
				continue;

			if (hover_box.x + hover_box.z < box.x || hover_box.x > box.x + box.width)
				continue;

			if (uph_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
			{
				Uph_NoteInteractionMode hover_mode = _uph_note_input_classify_hover(hover_box, (float)naui_mouse_x());
				if (editor->current_action_mode == UPH_ACTION_SELECT || editor->current_action_mode == UPH_ACTION_DRAW)
				{
					if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
						_uph_note_input_begin_drag(editor, pattern, i, hover_mode, box);

					naui_set_cursor(hover_mode == UPH_NOTE_INTERACTION_MOVE ? NAUI_CURSOR_HAND : NAUI_CURSOR_RESIZE_EW);
				}

				input->hovered_note.note_index = i;
				input->hovered_note.active = true;
			}
		}
	}
}

static void _uph_note_input_update_draw(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_MidiNoteInput *input = &editor->note_input;

	if (!editor->lanes_hovered)
		return;

	if (input->hovered_note.active)
		return;

	if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
		return;

	const double beat = uph_midi_view_x_to_beat(&editor->view, box, (float)naui_mouse_x());
	const int32_t row = uph_midi_view_y_to_row(&editor->view, box, (float)naui_mouse_y());
	const uint8_t key_number = (uint8_t)uph_note_map_clamp_key(&editor->note_map, uph_note_map_key_at_row(&editor->note_map, row));

	Uph_MidiNote note = {
		.start_beat = fmax(0.0, uph_snap_beat_floor(beat, editor->snap_resolution)),
		.length_beats = input->last_note_length,
		.key_number = key_number,
		.velocity = input->last_note_velocity
	};

	naui_list_free(input->drag.group_notes);
	input->drag.group_notes = NULL;
	input->drag.active = true;
	input->drag.creating = true;
	input->drag.group = false;
	input->drag.additive = false;
	input->drag.pattern_index = uph_state.shared.selected_resource.index;
	input->drag.note_index = (uint32_t)naui_list_len(pattern->notes);
	input->drag.mode = UPH_NOTE_INTERACTION_MOVE;
	input->drag.initial_drag_beat_offset = 0.0;
	input->drag.initial_drag_key_offset = 0;
	input->drag.initial_start_beat = note.start_beat;
	input->drag.initial_length_beats = note.length_beats;
	input->drag.initial_key_number = note.key_number;
	uph_state.shared.current_pattern_updated = true;
	naui_list_push(pattern->notes, note);
}

static void _uph_note_input_update_select(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_MidiNoteInput *input = &editor->note_input;

	if (!editor->lanes_hovered)
		return;

	if (input->hovered_note.active || input->drag.active)
		return;

	if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
		return;

	Uph_NoteMarqueeState *marquee = &input->marquee;
	marquee->active = true;
	marquee->start_beat = uph_midi_view_x_to_beat(&editor->view, box, (float)naui_mouse_x());
	marquee->start_content_y = (float)naui_mouse_y() - box.y + editor->view.scroll.y;
	uph_note_select_all(pattern, false);
}

static void _uph_note_input_update_marquee(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	const Uph_NoteMarqueeState *marquee = &editor->note_input.marquee;
	if (!marquee->active)
		return;

	const float corner_x = uph_midi_view_beat_to_x(&editor->view, box, marquee->start_beat);
	const float corner_y = box.y - editor->view.scroll.y + marquee->start_content_y;
	const float left = fminf(corner_x, (float)naui_mouse_x());
	const float right = fmaxf(corner_x, (float)naui_mouse_x());
	const float top = fminf(corner_y, (float)naui_mouse_y());
	const float bottom = fmaxf(corner_y, (float)naui_mouse_y());

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
	{
		Uph_MidiNote *note = &pattern->notes[i];
		const Naui_Vec4 note_box = uph_midi_view_note_box(&editor->view, &editor->note_map, box, note);
		note->selected = note_box.x < right && note_box.x + note_box.z > left && note_box.y < bottom && note_box.y + note_box.w > top;
	}
}

static void _uph_note_input_update_cut(Uph_MidiEditorData *editor, Leaf_BoundingBox box)
{
	const Uph_MidiNoteInput *input = &editor->note_input;

	if (!editor->lanes_hovered)
		return;

	if (!input->hovered_note.active)
		return;

	if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
		return;

	const double mouse_beat = uph_midi_view_x_to_beat(&editor->view, box, (float)naui_mouse_x());
	const double cut_beat = uph_snap_beat_round(mouse_beat, editor->snap_resolution);

	uph_note_cut(uph_state.shared.selected_resource.index, input->hovered_note.note_index, cut_beat, uph_snap_division(editor->snap_resolution));
}

static void _uph_note_input_update_delete(Uph_MidiEditorData *editor, Uph_MidiPattern *pattern)
{
	const Uph_MidiNoteInput *input = &editor->note_input;

	if (!editor->lanes_hovered)
		return;

	if (!input->hovered_note.active)
		return;

	if (!naui_mouse_pressed(NAUI_MOUSE_RIGHT))
		return;

	const uint32_t note_index = input->hovered_note.note_index;
	if (note_index >= (uint32_t)naui_list_len(pattern->notes))
		return;

	if (editor->current_action_mode == UPH_ACTION_SELECT && pattern->notes[note_index].selected && uph_note_selected_count(pattern) > 1)
	{
		uph_note_delete_selected(uph_state.shared.selected_resource.index);
		return;
	}

	Uph_ActionNoteDelete data = { .pattern_index = uph_state.shared.selected_resource.index, .note_index = note_index };
	naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_DELETE, data);
}

void uph_midi_note_input_update(Uph_MidiEditorData *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	_uph_note_input_update_drag(editor, box, pattern);
	
	if (editor->current_action_mode == UPH_ACTION_SELECT)
		_uph_note_input_update_select(editor, box, pattern);

	else if (editor->current_action_mode == UPH_ACTION_DRAW)
		_uph_note_input_update_draw(editor, box, pattern);

	else if (editor->current_action_mode == UPH_ACTION_CUT)
		_uph_note_input_update_cut(editor, box);

	_uph_note_input_update_delete(editor, pattern);
	_uph_note_input_update_marquee(editor, box, pattern);
}

void uph_midi_note_input_update_shortcuts(Uph_MidiEditorData *editor)
{
	if (!editor->panel_hovered || editor->note_input.drag.active || editor->note_input.marquee.active)
		return;

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
		return;

	Uph_MidiPattern *pattern = uph_note_pattern(uph_state.shared.selected_resource.index);
	if (!pattern)
		return;

	const bool ctrl = uph_ui_ctrl_down();
	if (naui_key_pressed(NAUI_KEY_DELETE))
		uph_note_delete_selected(uph_state.shared.selected_resource.index);

	else if (ctrl && naui_key_pressed(NAUI_KEY_A))
		uph_note_select_all(pattern, true);

	else if (ctrl && naui_key_pressed(NAUI_KEY_D))
		uph_note_duplicate_selected(uph_state.shared.selected_resource.index);

	else if (naui_key_pressed(NAUI_KEY_ESCAPE))
		uph_note_select_all(pattern, false);
}
