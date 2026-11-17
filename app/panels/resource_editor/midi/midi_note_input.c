#define UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH 9
#define UPH_MIDI_EDITOR_RESIZE_HANDLE_MAX_RATIO 0.33f
#define UPH_MIDI_EDITOR_CLICK_THRESHOLD 3

typedef uint8_t Uph_NoteInteractionMode;
enum
{
	UPH_NOTE_INTERACTION_NONE,
	UPH_NOTE_INTERACTION_MOVE,
	UPH_NOTE_INTERACTION_RESIZE_LEFT,
	UPH_NOTE_INTERACTION_RESIZE_RIGHT
};

typedef struct
{
	double initial_drag_beat_offset;
	double initial_start_beat;
	double initial_length_beats;
	uint8_t initial_key_number;
	int32_t initial_drag_key_offset;
	uint32_t note_index;
	uint32_t clone_start;
	uint32_t clone_count;
	Uph_ResourceIndex pattern_index;
	Naui_List(Uph_NoteSnapshot) group_notes;
	Naui_List(Uph_ActionBlockSnapshot) tail_blocks;
	Naui_Vec2 initial_mouse;
	bool active;
	bool moved;
	bool creating;
	bool group;
	bool additive;
	bool select_tool;
	Uph_NoteInteractionMode mode;
}
Uph_DraggingNoteState;

typedef struct
{
	uint32_t note_index;
	bool active;
}
Uph_HoveredNoteState;

typedef struct
{
	Naui_List(Uph_NoteSnapshot) erased;
	Naui_Vec2 last_mouse;
	uint64_t revision;
	Uph_ResourceIndex pattern_index;
	bool active;
}
Uph_NoteEraseState;

typedef struct
{
	Uph_DraggingNoteState drag;
	Uph_HoveredNoteState hovered_note;
	Uph_NoteMarqueeState marquee;
	Uph_NoteEraseState erase;
	double last_note_length;
	double last_note_velocity;
}
Uph_MidiNoteInput;

typedef uint8_t Uph_VelocityDragMode;
enum
{
	UPH_VELOCITY_DRAG_NONE,
	UPH_VELOCITY_DRAG_HANDLE,
	UPH_VELOCITY_DRAG_PAINT
};

typedef struct
{
	Naui_List(Uph_NoteSnapshot) originals;
	Uph_ResourceIndex pattern_index;
	uint32_t grabbed_index;
	int32_t grab_mouse_velocity;
	float last_mouse_x;
	Uph_VelocityDragMode mode;
}
Uph_VelocityDragState;

typedef struct
{
	Uph_VelocityDragState drag;
	float height_ratio;
	float area_height;
	float reveal;
	float resize_start_mouse_y;
	float resize_start_ratio;
	int32_t hovered_note;
	bool resizing;
	bool visible;
	bool hovered;
	bool splitter_hovered;
}
Uph_VelocityLane;

typedef uint8_t Uph_MidiLaneTab;
enum
{
	UPH_MIDI_LANE_TAB_VELOCITY,
	UPH_MIDI_LANE_TAB_PEDAL
};

typedef struct
{
	Naui_List(Uph_MidiPedalEvent) originals;
	Uph_ResourceIndex pattern_index;
	uint32_t index;
	double grab_beat_offset;
	Naui_Vec2 initial_mouse;
	bool active;
	bool moved;
	bool created;
}
Uph_PedalDragState;

typedef struct
{
	Uph_PedalDragState drag;
	Uph_MidiPedalType type;
	int32_t hovered_event;
	bool hovered;
}
Uph_PedalLane;

typedef struct
{
	Uph_MidiView view;
	Uph_NoteMap note_map;
	Uph_Piano piano;
	Uph_MidiNoteInput note_input;
	Uph_VelocityLane velocity_lane;
	Uph_PedalLane pedal_lane;
	Uph_MidiLaneTab lane_tab;
	Leaf_BoundingBox lanes_bounding_box;
	Uph_ParticleSystem particles;
	Uph_ActionMode current_action_mode;
	Uph_SnapResolution snap_resolution;
	Naui_Vec2 pan_last_mouse;
	Uph_ResourceIndex playing_pattern;
	Uph_LoopRegionDrag loop_drag;
	double playhead_drag_beat;
	int16_t audition_key;
	uint32_t scale_index;
	uint8_t scale_root;
	bool panning;
	bool playhead_dragging;
	bool record_was_playing;
	bool lanes_hovered;
	bool panel_hovered;
	bool piano_hovered;
	bool panel_focused;
}
Uph_MidiEditorState;

static Uph_ActionMode uph__note_input_tool(const Uph_MidiEditorState *editor)
{
	if (editor->current_action_mode == UPH_ACTION_DRAW && uph_ui_ctrl_down())
		return UPH_ACTION_SELECT;

	return editor->current_action_mode;
}

static Uph_NoteInteractionMode uph__note_input_classify_hover(Naui_Vec4 hover_box, float mouse_x)
{
	const float handle_width = fminf(NAUI_DPI(UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH), hover_box.z * UPH_MIDI_EDITOR_RESIZE_HANDLE_MAX_RATIO);

	if (mouse_x <= hover_box.x + handle_width)
		return UPH_NOTE_INTERACTION_RESIZE_LEFT;

	if (mouse_x >= hover_box.x + hover_box.z - handle_width)
		return UPH_NOTE_INTERACTION_RESIZE_RIGHT;
		
	return UPH_NOTE_INTERACTION_MOVE;
}

static bool uph__note_input_segment_hits_box(Naui_Vec2 a, Naui_Vec2 b, float left, float top, float right, float bottom)
{
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	const float p[4] = { -dx, dx, -dy, dy };
	const float q[4] = { a.x - left, right - a.x, a.y - top, bottom - a.y };

	float t_enter = 0.0f;
	float t_exit = 1.0f;
	for (int32_t i = 0; i < 4; i++)
	{
		if (p[i] == 0.0f)
		{
			if (q[i] < 0.0f)
				return false;

			continue;
		}

		const float t = q[i] / p[i];
		if (p[i] < 0.0f)
			t_enter = fmaxf(t_enter, t);
		else
			t_exit = fminf(t_exit, t);

		if (t_enter > t_exit)
			return false;
	}

	return true;
}

#pragma region Note Drag Actions

static void uph__note_input_discard_clones(Uph_DraggingNoteState *drag)
{
	Uph_MidiPattern *pattern = uph_note_pattern(drag->pattern_index);
	if (pattern && drag->clone_count > 0 && (uint32_t)naui_list_len(pattern->notes) == drag->clone_start + drag->clone_count)
	{
		for (uint32_t i = 0; i < drag->clone_count; i++)
			naui_list_pop(pattern->notes);

		uph_state.shared.current_pattern_updated = true;
	}

	drag->clone_count = 0;
}

static void uph__note_input_begin_clones(Uph_DraggingNoteState *drag, Uph_MidiPattern *pattern)
{
	Uph_MidiNote grabbed = pattern->notes[drag->note_index];
	grabbed.selected = false;

	drag->clone_start = (uint32_t)naui_list_len(pattern->notes);
	if (!drag->group)
		naui_list_push(pattern->notes, grabbed);

	for (uint32_t i = 0; drag->group && i < (uint32_t)naui_list_len(drag->group_notes); i++)
	{
		Uph_MidiNote clone = drag->group_notes[i].note;
		clone.selected = false;
		naui_list_push(pattern->notes, clone);
	}

	drag->clone_count = (uint32_t)naui_list_len(pattern->notes) - drag->clone_start;
}

static void uph__note_input_commit_clones(Uph_DraggingNoteState *drag, const Uph_MidiPattern *pattern)
{
	for (uint32_t i = 0; i < drag->clone_count; i++)
	{
		const uint32_t index = drag->clone_start + i;
		if (index >= (uint32_t)naui_list_len(pattern->notes))
			break;

		Uph_ActionNoteCreate data = {
			.pattern_index = drag->pattern_index,
			.note_index = index,
			.note = pattern->notes[index],
			.applied_live = true
		};
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_CREATE, data);
	}

	drag->clone_count = 0;
}

void uph_midi_note_input_cancel_drag(Uph_MidiEditorState *editor)
{
	Uph_DraggingNoteState *drag = &editor->note_input.drag;
	uph__note_input_discard_clones(drag);
	naui_list_free(drag->group_notes);
	drag->group_notes = NULL;
	naui_list_free(drag->tail_blocks);
	drag->tail_blocks = NULL;
	drag->active = false;
	drag->creating = false;
	drag->group = false;
	drag->mode = UPH_NOTE_INTERACTION_NONE;
}

static bool uph__note_input_geometry_equal(const Uph_MidiNote *a, const Uph_MidiNote *b)
{
	return a->start_beat == b->start_beat && a->length_beats == b->length_beats && a->key_number == b->key_number;
}

static void uph__note_input_begin_drag(Uph_MidiEditorState *editor, Uph_MidiPattern *pattern, uint32_t note_index, Uph_NoteInteractionMode mode, Leaf_BoundingBox box)
{
	Uph_MidiNoteInput *input = &editor->note_input;
	Uph_DraggingNoteState *drag = &input->drag;
	Uph_MidiNote *note = &pattern->notes[note_index];

	naui_list_free(drag->group_notes);
	drag->group_notes = NULL;
	naui_list_free(drag->tail_blocks);
	drag->tail_blocks = NULL;
	uph_pattern_collect_tail_blocks(uph_state.shared.selected_resource.index, &drag->tail_blocks);

	drag->active = true;
	drag->creating = false;
	drag->group = false;
	drag->additive = false;
	drag->moved = false;
	drag->initial_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
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

	drag->select_tool = uph__note_input_tool(editor) == UPH_ACTION_SELECT;
	drag->additive = drag->select_tool && (uph_ui_ctrl_down() || uph_ui_shift_down());
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
		note->selected = drag->select_tool;
	}

	if (note->selected)
	{
		drag->group = true;
		uph_note_collect_selected(pattern, &drag->group_notes);
	}

	input->last_note_length = note->length_beats;
	input->last_note_velocity = note->velocity;

	if (mode == UPH_NOTE_INTERACTION_MOVE && !drag->select_tool && uph_ui_shift_down())
		uph__note_input_begin_clones(drag, pattern);

	uph_state.shared.current_pattern_updated = true;
}

static void uph__note_input_apply_group_drag(Uph_MidiEditorState *editor, Uph_MidiPattern *pattern, const Uph_MidiNote *grabbed)
{
	const Uph_DraggingNoteState *drag = &editor->note_input.drag;
	const double division = uph_snap_division(editor->snap_resolution, editor->view.zoom.x);

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

static void uph__note_input_finish_drag(Uph_MidiEditorState *editor, Uph_MidiPattern *pattern)
{
	Uph_DraggingNoteState *drag = &editor->note_input.drag;
	const char *transform_action = drag->mode == UPH_NOTE_INTERACTION_MOVE ? UPH_ACTION_MIDI_NOTE_MOVE : UPH_ACTION_MIDI_NOTE_RESIZE;

	if (!drag->moved)
		uph__note_input_discard_clones(drag);

	const bool cloning = drag->clone_count > 0;
	const bool stretched = uph_pattern_stretch_tail_blocks(drag->pattern_index, drag->tail_blocks);
	const bool tail_group = (stretched || cloning) && !drag->group;
	if (tail_group)
	{
		naui_action_group_start(cloning ? UPH_ACTION_MIDI_NOTES_CLONE_GROUP : drag->creating ? UPH_ACTION_MIDI_NOTE_CREATE : transform_action);
		uph__note_input_commit_clones(drag, pattern);
	}

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
		bool changed = cloning;
		naui_action_group_start(cloning ? UPH_ACTION_MIDI_NOTES_CLONE_GROUP : drag->mode == UPH_NOTE_INTERACTION_MOVE ? UPH_ACTION_MIDI_NOTES_MOVE_GROUP : UPH_ACTION_MIDI_NOTES_RESIZE_GROUP);
		uph__note_input_commit_clones(drag, pattern);

		for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_notes); i++)
		{
			const Uph_NoteSnapshot *snapshot = &drag->group_notes[i];
			if (snapshot->index >= (uint32_t)naui_list_len(pattern->notes))
				continue;

			const Uph_MidiNote *note = &pattern->notes[snapshot->index];
			if (uph__note_input_geometry_equal(note, &snapshot->note))
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

		uph_pattern_commit_tail_blocks(drag->tail_blocks);
		naui_action_group_end();
		if (!changed && !drag->moved && !drag->additive)
		{
			uph_note_select_all(pattern, false);
			if (drag->select_tool && drag->note_index < (uint32_t)naui_list_len(pattern->notes))
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

	if (tail_group)
	{
		uph_pattern_commit_tail_blocks(drag->tail_blocks);
		naui_action_group_end();
	}

	uph_midi_note_input_cancel_drag(editor);
}

#pragma endregion

static void uph__note_input_update_drag(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
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
			const int32_t threshold = NAUI_DPI(UPH_MIDI_EDITOR_CLICK_THRESHOLD);
			if (fabsf((float)naui_mouse_x() - drag->initial_mouse.x) > threshold || fabsf((float)naui_mouse_y() - drag->initial_mouse.y) > threshold)
				drag->moved = true;

			double mouse_beat = uph_midi_view_x_to_beat(&editor->view, box, (float)naui_mouse_x());

			if (drag->mode == UPH_NOTE_INTERACTION_MOVE)
			{
				const double snapped = uph_snap_beat_floor(mouse_beat + drag->initial_drag_beat_offset, editor->snap_resolution, editor->view.zoom.x);
				note->start_beat = fmax(0.0, snapped);

				const int32_t mouse_row_now = uph_midi_view_y_to_row(&editor->view, box, (float)naui_mouse_y());
				const int32_t new_row = mouse_row_now + drag->initial_drag_key_offset;
				const int32_t new_key = uph_note_map_key_at_row(&editor->note_map, new_row);
				note->key_number = (uint8_t)uph_note_map_clamp_key(&editor->note_map, new_key);
				naui_set_cursor(NAUI_CURSOR_HAND);
			}
			else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_LEFT)
			{
				const double division = uph_snap_division(editor->snap_resolution, editor->view.zoom.x);

				double new_start = fmax(0.0, uph_snap_beat_round(mouse_beat, editor->snap_resolution, editor->view.zoom.x));
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
				const double division = uph_snap_division(editor->snap_resolution, editor->view.zoom.x);
				const double snapped_end = uph_snap_beat_round(note->start_beat + (mouse_beat - note->start_beat), editor->snap_resolution, editor->view.zoom.x);
				double new_length = snapped_end - note->start_beat;
				new_length = fmax(division, new_length);

				note->length_beats = new_length;
				input->last_note_length = note->length_beats;
				input->last_note_velocity = note->velocity;
				uph_state.shared.current_pattern_updated = true;
				naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
			}

			if (drag->group)
				uph__note_input_apply_group_drag(editor, pattern, note);

			uph_pattern_stretch_tail_blocks(drag->pattern_index, drag->tail_blocks);

			if (naui_mouse_released(NAUI_MOUSE_LEFT))
				uph__note_input_finish_drag(editor, pattern);

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
				Uph_NoteInteractionMode hover_mode = uph__note_input_classify_hover(hover_box, (float)naui_mouse_x());
				if (editor->current_action_mode == UPH_ACTION_SELECT || editor->current_action_mode == UPH_ACTION_DRAW)
				{
					if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
						uph__note_input_begin_drag(editor, pattern, i, hover_mode, box);

					naui_set_cursor(hover_mode == UPH_NOTE_INTERACTION_MOVE ? NAUI_CURSOR_HAND : NAUI_CURSOR_RESIZE_EW);
				}

				input->hovered_note.note_index = i;
				input->hovered_note.active = true;
			}
		}
	}
}

static void uph__note_input_update_draw(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
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
		.start_beat = fmax(0.0, uph_snap_beat_floor(beat, editor->snap_resolution, editor->view.zoom.x)),
		.length_beats = input->last_note_length,
		.key_number = key_number,
		.velocity = input->last_note_velocity
	};

	uph_note_select_all(pattern, false);
	naui_list_free(input->drag.group_notes);
	input->drag.group_notes = NULL;
	naui_list_free(input->drag.tail_blocks);
	input->drag.tail_blocks = NULL;
	uph_pattern_collect_tail_blocks(uph_state.shared.selected_resource.index, &input->drag.tail_blocks);
	input->drag.active = true;
	input->drag.creating = true;
	input->drag.group = false;
	input->drag.additive = false;
	input->drag.select_tool = false;
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

static void uph__note_input_update_select(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
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

static void uph__note_input_update_marquee(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
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

static void uph__note_input_update_cut(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	const Uph_MidiNoteInput *input = &editor->note_input;

	if (!editor->lanes_hovered)
		return;

	if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
		return;

	if (!input->hovered_note.active)
	{
		uph_note_select_all(pattern, false);
		return;
	}

	const double mouse_beat = uph_midi_view_x_to_beat(&editor->view, box, (float)naui_mouse_x());
	const double cut_beat = uph_snap_beat_round(mouse_beat, editor->snap_resolution, editor->view.zoom.x);

	uph_note_cut(uph_state.shared.selected_resource.index, input->hovered_note.note_index, cut_beat, uph_snap_division(editor->snap_resolution, editor->view.zoom.x));
}

#pragma region Note Erase

void uph_midi_note_input_cancel_erase(Uph_MidiEditorState *editor)
{
	Uph_NoteEraseState *erase = &editor->note_input.erase;
	naui_list_free(erase->erased);
	erase->erased = NULL;
	erase->active = false;
}

static bool uph__note_input_erase_note(Uph_NoteEraseState *erase, uint32_t note_index)
{
	Uph_NoteSnapshot snapshot = { .index = note_index };
	if (!uph_note_list_take(erase->pattern_index, note_index, &snapshot.note))
		return false;

	uph_midi_editor_emit_note_shockwave(erase->pattern_index, &snapshot.note);

	naui_list_push(erase->erased, snapshot);
	return true;
}

static void uph__note_input_finish_erase(Uph_MidiEditorState *editor)
{
	Uph_NoteEraseState *erase = &editor->note_input.erase;
	const bool history_untouched = naui_action_get_revision() == erase->revision;

	if (naui_list_len(erase->erased) > 0 && history_untouched)
	{
		Uph_ActionNotesErase data = { .pattern_index = erase->pattern_index, .erased = erase->erased, .applied_live = true };
		if (naui_action_execute_stack(UPH_ACTION_MIDI_NOTES_ERASE, data))
			erase->erased = NULL;
	}

	uph_midi_note_input_cancel_erase(editor);
}

static int32_t uph__note_input_note_at(const Uph_MidiEditorState *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern, Naui_Vec2 point)
{
	for (uint32_t i = (uint32_t)naui_list_len(pattern->notes); i > 0; i--)
	{
		const Naui_Vec4 note_box = uph_midi_view_note_box(&editor->view, &editor->note_map, box, &pattern->notes[i - 1]);
		if (uph_song_timeline_vec4_contains_vec2(note_box, point))
			return (int32_t)(i - 1);
	}

	return -1;
}

static void uph__note_input_erase_along(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern, Naui_Vec2 from, Naui_Vec2 to)
{
	Uph_NoteEraseState *erase = &editor->note_input.erase;

	for (uint32_t i = (uint32_t)naui_list_len(pattern->notes); i > 0; i--)
	{
		const Naui_Vec4 note_box = uph_midi_view_note_box(&editor->view, &editor->note_map, box, &pattern->notes[i - 1]);
		const float left = fmaxf(note_box.x, box.x);
		const float top = fmaxf(note_box.y, box.y);
		const float right = fminf(note_box.x + note_box.z, box.x + box.width);
		const float bottom = fminf(note_box.y + note_box.w, box.y + box.height);
		if (left >= right || top >= bottom)
			continue;

		if (uph__note_input_segment_hits_box(from, to, left, top, right, bottom))
			uph__note_input_erase_note(erase, i - 1);
	}
}

static void uph__note_input_begin_erase(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern, Naui_Vec2 mouse)
{
	Uph_NoteEraseState *erase = &editor->note_input.erase;
	erase->active = true;
	erase->revision = naui_action_get_revision();
	erase->pattern_index = uph_state.shared.selected_resource.index;
	erase->last_mouse = mouse;

	const int32_t pressed = uph__note_input_note_at(editor, box, pattern, mouse);
	if (pressed < 0)
	{
		uph_note_select_all(pattern, false);
		return;
	}

	uph__note_input_erase_note(erase, (uint32_t)pressed);
}

static void uph__note_input_update_erase(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	Uph_MidiNoteInput *input = &editor->note_input;
	Uph_NoteEraseState *erase = &input->erase;
	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };

	if (erase->active)
	{
		if (!naui_mouse_down(NAUI_MOUSE_RIGHT) || erase->pattern_index != uph_state.shared.selected_resource.index)
		{
			uph__note_input_finish_erase(editor);
			return;
		}

		if (mouse.x != erase->last_mouse.x || mouse.y != erase->last_mouse.y)
			uph__note_input_erase_along(editor, box, pattern, erase->last_mouse, mouse);

		erase->last_mouse = mouse;
	}
	else
	{
		const bool busy = input->drag.active || input->marquee.active || editor->velocity_lane.drag.mode != UPH_VELOCITY_DRAG_NONE || editor->pedal_lane.drag.active;
		if (!editor->lanes_hovered || busy || !naui_mouse_pressed(NAUI_MOUSE_RIGHT))
			return;

		uph__note_input_begin_erase(editor, box, pattern, mouse);
	}

	input->hovered_note.active = false;
}

#pragma endregion

void uph_midi_note_input_update(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
	if (editor->note_input.erase.active)
	{
		uph__note_input_update_erase(editor, box, pattern);
		return;
	}

	uph__note_input_update_drag(editor, box, pattern);

	const Uph_ActionMode tool = uph__note_input_tool(editor);
	if (tool == UPH_ACTION_SELECT)
		uph__note_input_update_select(editor, box, pattern);

	else if (tool == UPH_ACTION_DRAW)
		uph__note_input_update_draw(editor, box, pattern);

	else if (tool == UPH_ACTION_CUT)
		uph__note_input_update_cut(editor, box, pattern);

	uph__note_input_update_erase(editor, box, pattern);
	uph__note_input_update_marquee(editor, box, pattern);
}

void uph_midi_editor_quantize(const Uph_MidiEditorState *editor)
{
	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
		return;

	uph_note_quantize(uph_state.shared.selected_resource.index, editor->snap_resolution, editor->view.zoom.x);
}

static void uph__note_input_update_move_keys(const Uph_MidiEditorState *editor, Uph_ResourceIndex pattern_index)
{
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;
	const double step = uph_snap_division(editor->snap_resolution, editor->view.zoom.x);

	if (naui_keybind_pressed_repeat(keybinds->move_up))
		uph_note_shift_selected(pattern_index, 0.0, 1);

	else if (naui_keybind_pressed_repeat(keybinds->move_down))
		uph_note_shift_selected(pattern_index, 0.0, -1);

	else if (naui_keybind_pressed_repeat(keybinds->octave_up))
		uph_note_shift_selected(pattern_index, 0.0, UPH_NOTES_PER_OCTAVE);

	else if (naui_keybind_pressed_repeat(keybinds->octave_down))
		uph_note_shift_selected(pattern_index, 0.0, -UPH_NOTES_PER_OCTAVE);

	else if (naui_keybind_pressed_repeat(keybinds->move_left))
		uph_note_shift_selected(pattern_index, -step, 0);

	else if (naui_keybind_pressed_repeat(keybinds->move_right))
		uph_note_shift_selected(pattern_index, step, 0);
}

void uph_midi_editor_paste(const Uph_MidiEditorState *editor)
{
	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
		return;

	const double beat = uph_snap_beat_round(uph_midi_preview_position(), editor->snap_resolution, editor->view.zoom.x);
	uph_note_paste(uph_state.shared.selected_resource.index, fmax(0.0, beat));
}

static void uph__note_input_update_clipboard_keys(const Uph_MidiEditorState *editor, const Uph_MidiPattern *pattern, Uph_ResourceIndex pattern_index)
{
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;

	if (naui_keybind_pressed(keybinds->copy))
		uph_note_copy_selected(pattern);

	else if (naui_keybind_pressed(keybinds->cut) && uph_note_copy_selected(pattern))
		uph_note_delete_selected(pattern_index);

	else if (naui_keybind_pressed(keybinds->paste))
		uph_midi_editor_paste(editor);
}

static void uph__note_input_update_note_tool_keys(const Uph_MidiEditorState *editor, Uph_ResourceIndex pattern_index)
{
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;
	const double division = uph_snap_division(editor->snap_resolution, editor->view.zoom.x);

	if (naui_keybind_pressed(keybinds->quantize))
		uph_midi_editor_quantize(editor);

	else if (naui_keybind_pressed(keybinds->legato))
		uph_note_legato(pattern_index);

	else if (naui_keybind_pressed(keybinds->glue))
		uph_note_glue(pattern_index);

	else if (naui_keybind_pressed(keybinds->chop))
		uph_note_chop(pattern_index, division);

	else if (naui_keybind_pressed(keybinds->flip_horizontal))
		uph_note_flip(pattern_index, false);

	else if (naui_keybind_pressed(keybinds->flip_vertical))
		uph_note_flip(pattern_index, true);

	else if (naui_keybind_pressed(keybinds->strum))
		uph_note_strum(pattern_index, division);
}

static void uph__note_input_update_tool_keys(Uph_MidiEditorState *editor)
{
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;

	if (naui_keybind_pressed(keybinds->tool_select))
		editor->current_action_mode = UPH_ACTION_SELECT;

	else if (naui_keybind_pressed(keybinds->tool_draw))
		editor->current_action_mode = UPH_ACTION_DRAW;

	else if (naui_keybind_pressed(keybinds->tool_cut))
		editor->current_action_mode = UPH_ACTION_CUT;
}

void uph_midi_note_input_update_shortcuts(Uph_MidiEditorState *editor)
{
	if (naui_panel_focused() != uph_state.panels.resource_editor || editor->note_input.drag.active || editor->note_input.marquee.active || editor->note_input.erase.active)
		return;

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
		return;

	Uph_MidiPattern *pattern = uph_note_pattern(uph_state.shared.selected_resource.index);
	if (!pattern)
		return;

	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;
	if (naui_keybind_pressed(keybinds->delete_selected))
		uph_note_delete_selected(uph_state.shared.selected_resource.index);

	else if (naui_keybind_pressed(keybinds->select_all))
		uph_note_select_all(pattern, true);

	else if (naui_keybind_pressed(keybinds->duplicate))
		uph_note_duplicate_selected(uph_state.shared.selected_resource.index);

	else if (naui_keybind_pressed(keybinds->deselect))
		uph_note_select_all(pattern, false);

	else if (naui_keybind_pressed(keybinds->invert_selection))
		uph_note_invert_selection(pattern);

	const Uph_ResourceIndex pattern_index = uph_state.shared.selected_resource.index;
	uph__note_input_update_move_keys(editor, pattern_index);
	uph__note_input_update_clipboard_keys(editor, pattern, pattern_index);
	uph__note_input_update_note_tool_keys(editor, pattern_index);
	uph__note_input_update_tool_keys(editor);
}
