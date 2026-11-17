typedef struct
{
	uint32_t index;
	Uph_MidiNote note;
} Uph_NoteSnapshot;

typedef struct
{
	uint32_t resource_index;
	bool resource_exists;
} Uph_ActionResourceCreate;

typedef struct
{
	Uph_ResourceIndex resource_index;
	Naui_String old_name;
	Naui_String new_name;
} Uph_ActionResourceRename;

typedef struct
{
	Uph_ResourceIndex resource_index;
	Uph_MidiPattern pattern;
	Naui_List(Uph_ActionBlockSnapshot) blocks;
	bool deleted;
} Uph_ActionPatternDelete;

typedef struct
{
	Uph_ResourceIndex source_index;
	Uph_ResourceIndex new_index;
	bool resource_exists;
} Uph_ActionPatternDuplicate;

typedef struct
{
	Uph_ResourceIndex pattern_index;
	uint32_t note_index;
	Uph_MidiNote note;
	bool applied_live;
} Uph_ActionNoteCreate;

typedef struct
{
	Uph_ResourceIndex pattern_index;
	uint32_t note_index;
	Uph_MidiNote note;
} Uph_ActionNoteDelete;

typedef struct
{
	Uph_ResourceIndex pattern_index;
	uint32_t note_index;
	double old_start, old_length;
	double new_start, new_length;
	uint8_t old_key, new_key;
	bool applied_live;
} Uph_ActionNoteTransform;

typedef struct
{
	Uph_ResourceIndex pattern_index;
	uint32_t note_index;
	uint8_t old_velocity, new_velocity;
	bool applied_live;
} Uph_ActionNoteVelocity;

typedef struct
{
	Uph_ResourceIndex pattern_index;
	Naui_List(Uph_MidiPedalEvent) old_pedals;
	Naui_List(Uph_MidiPedalEvent) new_pedals;
	bool applied_live;
} Uph_ActionPedalsEdit;

typedef struct
{
	Uph_ResourceIndex pattern_index;
	Naui_List(Uph_NoteSnapshot) erased;
	bool applied_live;
} Uph_ActionNotesErase;

#pragma region Pattern Create
bool uph__action_pattern_create_execute(void* userdata)
{
	Uph_ActionResourceCreate* data = userdata;
	uph_resources_add_pattern();
	data->resource_index = (uint32_t)naui_list_len(uph_state.project.midi_patterns) - 1;
	data->resource_exists = true;
	uph_action_resource_restored(UPH_RESOURCE_PATTERN, data->resource_index);
	return true;
}

bool uph__action_pattern_create_undo(void* userdata)
{
	Uph_ActionResourceCreate* data = userdata;
	if (!data->resource_exists || data->resource_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	uph_midi_pattern_free_events(&uph_state.project.midi_patterns[data->resource_index]);
	naui_list_remove(uph_state.project.midi_patterns, data->resource_index);
	uph_action_resource_removed(UPH_RESOURCE_PATTERN, data->resource_index);
	data->resource_exists = false;
	return true;
}

#pragma endregion

#pragma region Pattern Delete
bool uph__action_pattern_delete_execute(void* userdata)
{
	Uph_ActionPatternDelete* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	naui_list_free(data->blocks);
	data->blocks = NULL;

	data->pattern = uph_state.project.midi_patterns[data->resource_index];
	uph_block_unlink_resource(UPH_RESOURCE_PATTERN, data->resource_index, &data->blocks);
	uph_resource_list_emit_item_shockwave(UPH_RESOURCE_PATTERN, data->resource_index);
	naui_list_remove(uph_state.project.midi_patterns, data->resource_index);

	data->deleted = true;
	uph_state.shared.current_pattern_updated = true;
	uph_action_resource_removed(UPH_RESOURCE_PATTERN, data->resource_index);
	return true;
}

bool uph__action_pattern_delete_undo(void* userdata)
{
	Uph_ActionPatternDelete* data = userdata;
	if (!data->deleted || data->resource_index > (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	naui_list_insert(uph_state.project.midi_patterns, data->pattern, data->resource_index);
	uph_block_relink_resource(UPH_RESOURCE_PATTERN, data->resource_index, data->blocks);

	data->deleted = false;
	uph_state.shared.current_pattern_updated = true;
	uph_action_resource_restored(UPH_RESOURCE_PATTERN, data->resource_index);
	return true;
}

void uph__action_pattern_delete_destroy(void* userdata)
{
	Uph_ActionPatternDelete* data = userdata;
	if (data->deleted)
		uph_midi_pattern_free_events(&data->pattern);

	naui_list_free(data->blocks);
	data->blocks = NULL;
}

#pragma endregion

#pragma region Pattern Rename
bool uph__action_pattern_rename_execute(void* userdata)
{
	Uph_ActionResourceRename* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	uph_state.project.midi_patterns[data->resource_index].name = data->new_name;
	return true;
}

bool uph__action_pattern_rename_undo(void* userdata)
{
	Uph_ActionResourceRename* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	uph_state.project.midi_patterns[data->resource_index].name = data->old_name;
	return true;
}

#pragma endregion

#pragma region Pattern Duplicate
bool uph__action_pattern_duplicate_execute(void* userdata)
{
	Uph_ActionPatternDuplicate* data = userdata;
	if (data->source_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	uph_resources_copy_pattern(data->source_index);
	data->new_index = (uint32_t)naui_list_len(uph_state.project.midi_patterns) - 1;
	data->resource_exists = true;
	uph_action_resource_restored(UPH_RESOURCE_PATTERN, data->new_index);
	return true;
}

bool uph__action_pattern_duplicate_undo(void* userdata)
{
	Uph_ActionPatternDuplicate* data = userdata;
	if (!data->resource_exists || data->new_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	uph_midi_pattern_free_events(&uph_state.project.midi_patterns[data->new_index]);
	naui_list_remove(uph_state.project.midi_patterns, data->new_index);
	uph_action_resource_removed(UPH_RESOURCE_PATTERN, data->new_index);
	data->resource_exists = false;
	return true;
}

#pragma endregion

#pragma region Note Helpers
static Uph_MidiPattern* uph_note_pattern(Uph_ResourceIndex pattern_index)
{
	if (pattern_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return NULL;

	return &uph_state.project.midi_patterns[pattern_index];
}

static bool uph_note_list_insert(Uph_ResourceIndex pattern_index, uint32_t note_index, Uph_MidiNote note)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return false;

	if (note_index > (uint32_t)naui_list_len(pattern->notes))
		note_index = (uint32_t)naui_list_len(pattern->notes);

	naui_list_insert(pattern->notes, note, note_index);
	uph_state.shared.current_pattern_updated = true;
	return true;
}

static bool uph_note_list_take(Uph_ResourceIndex pattern_index, uint32_t note_index, Uph_MidiNote *out)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern || note_index >= (uint32_t)naui_list_len(pattern->notes))
		return false;

	*out = pattern->notes[note_index];
	naui_list_remove(pattern->notes, note_index);
	uph_state.shared.current_pattern_updated = true;
	return true;
}

static bool uph_note_apply(Uph_ResourceIndex pattern_index, uint32_t note_index, double start, double length, uint8_t key)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern || note_index >= (uint32_t)naui_list_len(pattern->notes))
		return false;

	Uph_MidiNote* note = &pattern->notes[note_index];
	note->start_beat = start;
	note->length_beats = length;
	note->key_number = key;
	uph_state.shared.current_pattern_updated = true;
	return true;
}

#pragma endregion

#pragma region Note Create
bool uph__action_note_create_execute(void* userdata)
{
	Uph_ActionNoteCreate* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_note_pattern(data->pattern_index) != NULL;
	}

	return uph_note_list_insert(data->pattern_index, data->note_index, data->note);
}

bool uph__action_note_create_undo(void* userdata)
{
	Uph_ActionNoteCreate* data = userdata;
	return uph_note_list_take(data->pattern_index, data->note_index, &data->note);
}

bool uph__action_note_create_redo(void* userdata)
{
	Uph_ActionNoteCreate* data = userdata;
	return uph_note_list_insert(data->pattern_index, data->note_index, data->note);
}

#pragma endregion

#pragma region Note Delete
bool uph__action_note_delete_execute(void* userdata)
{
	Uph_ActionNoteDelete* data = userdata;
	if (!uph_note_list_take(data->pattern_index, data->note_index, &data->note))
		return false;

	uph_midi_editor_emit_note_shockwave(data->pattern_index, &data->note);
	return true;
}

bool uph__action_note_delete_undo(void* userdata)
{
	Uph_ActionNoteDelete* data = userdata;
	return uph_note_list_insert(data->pattern_index, data->note_index, data->note);
}

#pragma endregion

#pragma region Notes Erase
static bool uph__action_notes_erase_apply(Uph_ActionNotesErase* data)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(data->erased); i++)
	{
		Uph_MidiNote removed;
		if (!uph_note_list_take(data->pattern_index, data->erased[i].index, &removed))
			return false;

		uph_midi_editor_emit_note_shockwave(data->pattern_index, &removed);
	}

	return true;
}

bool uph__action_notes_erase_execute(void* userdata)
{
	Uph_ActionNotesErase* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_note_pattern(data->pattern_index) != NULL;
	}

	return uph__action_notes_erase_apply(data);
}

bool uph__action_notes_erase_undo(void* userdata)
{
	Uph_ActionNotesErase* data = userdata;
	for (uint32_t i = (uint32_t)naui_list_len(data->erased); i > 0; i--)
	{
		if (!uph_note_list_insert(data->pattern_index, data->erased[i - 1].index, data->erased[i - 1].note))
			return false;
	}

	return true;
}

bool uph__action_notes_erase_redo(void* userdata)
{
	return uph__action_notes_erase_apply(userdata);
}

void uph__action_notes_erase_destroy(void* userdata)
{
	Uph_ActionNotesErase* data = userdata;
	naui_list_free(data->erased);
	data->erased = NULL;
}

#pragma endregion

#pragma region Note Move / Resize
bool uph__action_note_transform_execute(void* userdata)
{
	Uph_ActionNoteTransform* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_note_pattern(data->pattern_index) != NULL;
	}

	return uph_note_apply(data->pattern_index, data->note_index, data->new_start, data->new_length, data->new_key);
}

bool uph__action_note_transform_undo(void* userdata)
{
	Uph_ActionNoteTransform* data = userdata;
	return uph_note_apply(data->pattern_index, data->note_index, data->old_start, data->old_length, data->old_key);
}

bool uph__action_note_transform_redo(void* userdata)
{
	Uph_ActionNoteTransform* data = userdata;
	return uph_note_apply(data->pattern_index, data->note_index, data->new_start, data->new_length, data->new_key);
}

#pragma endregion

#pragma region Note Velocity
static bool uph_note_apply_velocity(Uph_ResourceIndex pattern_index, uint32_t note_index, uint8_t velocity)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern || note_index >= (uint32_t)naui_list_len(pattern->notes))
		return false;

	pattern->notes[note_index].velocity = velocity;
	uph_state.shared.current_pattern_updated = true;
	return true;
}

bool uph__action_note_velocity_execute(void* userdata)
{
	Uph_ActionNoteVelocity* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_note_pattern(data->pattern_index) != NULL;
	}

	return uph_note_apply_velocity(data->pattern_index, data->note_index, data->new_velocity);
}

bool uph__action_note_velocity_undo(void* userdata)
{
	Uph_ActionNoteVelocity* data = userdata;
	return uph_note_apply_velocity(data->pattern_index, data->note_index, data->old_velocity);
}

bool uph__action_note_velocity_redo(void* userdata)
{
	Uph_ActionNoteVelocity* data = userdata;
	return uph_note_apply_velocity(data->pattern_index, data->note_index, data->new_velocity);
}

#pragma endregion

#pragma region Note Selection
static void uph_note_collect_selected(const Uph_MidiPattern *pattern, Naui_List(Uph_NoteSnapshot) *out)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
	{
		if (!pattern->notes[i].selected)
			continue;

		Uph_NoteSnapshot snapshot = { .index = i, .note = pattern->notes[i] };
		naui_list_push(*out, snapshot);
	}
}

static uint32_t uph_note_selected_count(const Uph_MidiPattern *pattern)
{
	uint32_t count = 0;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
		count += pattern->notes[i].selected;

	return count;
}

static void uph_note_select_all(Uph_MidiPattern *pattern, bool selected)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
		pattern->notes[i].selected = selected;
}

#pragma endregion

#pragma region Pattern Tail Blocks

static void uph_pattern_collect_tail_blocks(Uph_ResourceIndex pattern_index, Naui_List(Uph_ActionBlockSnapshot) *out)
{
	const Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return;

	const double pattern_length = uph_calculate_pattern_length(pattern);

	Naui_List(Uph_ActionBlockSnapshot) blocks = NULL;
	uph_block_collect_all(UPH_RESOURCE_PATTERN, false, true, pattern_index, &blocks);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
	{
		const Uph_TimelineBlock* block = &blocks[i].block;
		if (block->start_offset_beats + block->length_beats >= pattern_length - 1e-9)
			naui_list_push(*out, blocks[i]);
	}

	naui_list_free(blocks);
}

static Uph_TimelineBlock* uph_pattern_resolve_tail_block(const Uph_ActionBlockSnapshot *snapshot)
{
	Uph_Track* track = uph_action_track_resolve(snapshot->track);
	if (!track || snapshot->index >= (uint32_t)naui_list_len(track->blocks))
		return NULL;

	Uph_TimelineBlock* block = &track->blocks[snapshot->index];
	if (block->type != UPH_RESOURCE_PATTERN || block->resource_index != snapshot->block.resource_index)
		return NULL;

	return block;
}

static bool uph_pattern_stretch_tail_blocks(Uph_ResourceIndex pattern_index, const Naui_List(Uph_ActionBlockSnapshot) tail)
{
	const Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return false;

	const double pattern_length = uph_calculate_pattern_length(pattern);
	bool stretched = false;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tail); i++)
	{
		Uph_TimelineBlock* block = uph_pattern_resolve_tail_block(&tail[i]);
		if (!block)
			continue;

		block->length_beats = fmax(tail[i].block.length_beats, pattern_length - tail[i].block.start_offset_beats);
		stretched |= block->length_beats != tail[i].block.length_beats;
	}

	return stretched;
}

static void uph_pattern_commit_tail_blocks(const Naui_List(Uph_ActionBlockSnapshot) tail)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tail); i++)
	{
		const Uph_TimelineBlock* block = uph_pattern_resolve_tail_block(&tail[i]);
		if (!block || block->length_beats == tail[i].block.length_beats)
			continue;

		Uph_ActionBlockTransform data = {
			.src_track = tail[i].track, .src_index = tail[i].index, .dst_track = tail[i].track, .dst_index = tail[i].index,
			.old_start = tail[i].block.start_beat, .old_length = tail[i].block.length_beats, .old_offset = tail[i].block.start_offset_beats,
			.new_start = block->start_beat, .new_length = block->length_beats, .new_offset = block->start_offset_beats,
			.applied_live = true
		};
		naui_action_execute_stack(UPH_ACTION_BLOCK_RESIZE, data);
	}
}

#pragma endregion

#pragma region Note Group Operations

static void uph_note_delete_selected(Uph_ResourceIndex pattern_index)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return;

	Naui_List(Uph_NoteSnapshot) selected = NULL;
	uph_note_collect_selected(pattern, &selected);
	if (naui_list_len(selected) == 0)
		return;

	naui_action_group_start(UPH_ACTION_MIDI_NOTES_DELETE_GROUP);

	for (uint32_t i = (uint32_t)naui_list_len(selected); i > 0; i--)
	{
		Uph_ActionNoteDelete data = { .pattern_index = pattern_index, .note_index = selected[i - 1].index };
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_DELETE, data);
	}

	naui_action_group_end();
	naui_list_free(selected);
}

static void uph_note_duplicate_selected(Uph_ResourceIndex pattern_index)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return;

	Naui_List(Uph_NoteSnapshot) selected = NULL;
	uph_note_collect_selected(pattern, &selected);
	if (naui_list_len(selected) == 0)
		return;

	double first_start = selected[0].note.start_beat;
	double last_end = selected[0].note.start_beat + selected[0].note.length_beats;
	for (uint32_t i = 1; i < (uint32_t)naui_list_len(selected); i++)
	{
		first_start = fmin(first_start, selected[i].note.start_beat);
		last_end = fmax(last_end, selected[i].note.start_beat + selected[i].note.length_beats);
	}

	const double shift = last_end - first_start;
	uph_note_select_all(pattern, false);

	Naui_List(Uph_ActionBlockSnapshot) tail = NULL;
	uph_pattern_collect_tail_blocks(pattern_index, &tail);

	naui_action_group_start(UPH_ACTION_MIDI_NOTES_DUPLICATE_GROUP);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		Uph_ActionNoteCreate data = { .pattern_index = pattern_index, .note_index = (uint32_t)naui_list_len(pattern->notes), .note = selected[i].note };
		data.note.start_beat += shift;
		data.note.selected = true;
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_CREATE, data);
	}

	uph_pattern_stretch_tail_blocks(pattern_index, tail);
	uph_pattern_commit_tail_blocks(tail);

	naui_action_group_end();
	naui_list_free(tail);
	naui_list_free(selected);
}

static Naui_List(Uph_MidiNote) uph__note_clipboard;

static bool uph_note_copy_selected(const Uph_MidiPattern *pattern)
{
	Naui_List(Uph_NoteSnapshot) selected = NULL;
	uph_note_collect_selected(pattern, &selected);
	if (naui_list_len(selected) == 0)
		return false;

	double first_start = selected[0].note.start_beat;
	for (uint32_t i = 1; i < (uint32_t)naui_list_len(selected); i++)
		first_start = fmin(first_start, selected[i].note.start_beat);

	naui_list_free(uph__note_clipboard);
	uph__note_clipboard = NULL;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		Uph_MidiNote note = selected[i].note;
		note.start_beat -= first_start;
		naui_list_push(uph__note_clipboard, note);
	}

	naui_list_free(selected);
	return true;
}

static void uph_note_paste(Uph_ResourceIndex pattern_index, double beat)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern || naui_list_len(uph__note_clipboard) == 0)
		return;

	uph_note_select_all(pattern, false);

	Naui_List(Uph_ActionBlockSnapshot) tail = NULL;
	uph_pattern_collect_tail_blocks(pattern_index, &tail);

	naui_action_group_start(UPH_ACTION_MIDI_NOTES_PASTE_GROUP);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph__note_clipboard); i++)
	{
		Uph_ActionNoteCreate data = { .pattern_index = pattern_index, .note_index = (uint32_t)naui_list_len(pattern->notes), .note = uph__note_clipboard[i] };
		data.note.start_beat += beat;
		data.note.selected = true;
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_CREATE, data);
	}

	uph_pattern_stretch_tail_blocks(pattern_index, tail);
	uph_pattern_commit_tail_blocks(tail);

	naui_action_group_end();
	naui_list_free(tail);
}

static void uph_note_invert_selection(Uph_MidiPattern *pattern)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
		pattern->notes[i].selected = !pattern->notes[i].selected;
}

static void uph_note_shift_selected(Uph_ResourceIndex pattern_index, double delta_beats, int32_t delta_key)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return;

	Naui_List(Uph_NoteSnapshot) selected = NULL;
	uph_note_collect_selected(pattern, &selected);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		const int32_t key = (int32_t)selected[i].note.key_number + delta_key;
		if (key < 0 || key > INT8_MAX)
			delta_key = 0;

		delta_beats = fmax(delta_beats, -selected[i].note.start_beat);
	}

	if (naui_list_len(selected) == 0 || (delta_beats == 0.0 && delta_key == 0))
	{
		naui_list_free(selected);
		return;
	}

	Naui_List(Uph_ActionBlockSnapshot) tail = NULL;
	uph_pattern_collect_tail_blocks(pattern_index, &tail);

	naui_action_group_start(UPH_ACTION_MIDI_NOTES_MOVE_GROUP);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		const Uph_MidiNote *note = &selected[i].note;
		Uph_ActionNoteTransform data = {
			.pattern_index = pattern_index, .note_index = selected[i].index,
			.old_start = note->start_beat, .old_length = note->length_beats, .old_key = note->key_number,
			.new_start = note->start_beat + delta_beats, .new_length = note->length_beats, .new_key = (uint8_t)((int32_t)note->key_number + delta_key)
		};
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_MOVE, data);
	}

	uph_pattern_stretch_tail_blocks(pattern_index, tail);
	uph_pattern_commit_tail_blocks(tail);

	naui_action_group_end();
	naui_list_free(tail);
	naui_list_free(selected);
}

static bool uph_note_quantize(Uph_ResourceIndex pattern_index, Uph_SnapResolution resolution, float zoom_x)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return false;

	const bool only_selected = uph_note_selected_count(pattern) > 0;
	const double division = uph_snap_division(resolution, zoom_x);

	Naui_List(Uph_ActionBlockSnapshot) tail = NULL;
	uph_pattern_collect_tail_blocks(pattern_index, &tail);

	bool changed = false;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
	{
		const Uph_MidiNote *note = &pattern->notes[i];
		if (only_selected && !note->selected)
			continue;

		const double new_start = fmax(0.0, uph_snap_beat_round(note->start_beat, resolution, zoom_x));
		const double new_length = fmax(division, uph_snap_beat_round(note->start_beat + note->length_beats, resolution, zoom_x) - new_start);

		if (new_start == note->start_beat && new_length == note->length_beats)
			continue;

		if (!changed)
			naui_action_group_start(UPH_ACTION_MIDI_NOTES_QUANTIZE_GROUP);

		Uph_ActionNoteTransform data = {
			.pattern_index = pattern_index, .note_index = i,
			.old_start = note->start_beat, .old_length = note->length_beats, .old_key = note->key_number,
			.new_start = new_start, .new_length = new_length, .new_key = note->key_number
		};
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_MOVE, data);
		changed = true;
	}

	if (changed)
	{
		uph_pattern_stretch_tail_blocks(pattern_index, tail);
		uph_pattern_commit_tail_blocks(tail);
		naui_action_group_end();
	}

	naui_list_free(tail);
	return changed;
}

#define UPH_NOTE_TOOL_EPSILON 1e-9
#define UPH_NOTE_STRUM_GRID_RATIO 0.25
#define UPH_NOTE_STRUM_MAX_LENGTH_RATIO 0.5

static int uph__note_snapshot_compare_start(const void *a, const void *b)
{
	const Uph_NoteSnapshot *left = a;
	const Uph_NoteSnapshot *right = b;
	if (left->note.start_beat != right->note.start_beat)
		return left->note.start_beat < right->note.start_beat ? -1 : 1;

	if (left->note.key_number != right->note.key_number)
		return left->note.key_number < right->note.key_number ? -1 : 1;

	return (left->index > right->index) - (left->index < right->index);
}

static int uph__note_snapshot_compare_key(const void *a, const void *b)
{
	const Uph_NoteSnapshot *left = a;
	const Uph_NoteSnapshot *right = b;
	if (left->note.key_number != right->note.key_number)
		return left->note.key_number < right->note.key_number ? -1 : 1;

	return uph__note_snapshot_compare_start(a, b);
}

static int uph__note_index_compare_descending(const void *a, const void *b)
{
	const uint32_t left = *(const uint32_t *)a;
	const uint32_t right = *(const uint32_t *)b;
	return (left < right) - (left > right);
}

static void uph_note_collect_targets(const Uph_MidiPattern *pattern, Naui_List(Uph_NoteSnapshot) *out, int (*compare)(const void *, const void *))
{
	const bool only_selected = uph_note_selected_count(pattern) > 0;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
	{
		if (only_selected && !pattern->notes[i].selected)
			continue;

		Uph_NoteSnapshot snapshot = { .index = i, .note = pattern->notes[i] };
		naui_list_push(*out, snapshot);
	}

	if (naui_list_len(*out) > 1)
		qsort(*out, (size_t)naui_list_len(*out), sizeof(Uph_NoteSnapshot), compare);
}

static Naui_List(Uph_NoteSnapshot) uph_note_snapshots_copy(const Naui_List(Uph_NoteSnapshot) source)
{
	Naui_List(Uph_NoteSnapshot) copy = NULL;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(source); i++)
		naui_list_push(copy, source[i]);

	return copy;
}

static bool uph_note_commit_transforms(Uph_ResourceIndex pattern_index, const char *group_name, const Naui_List(Uph_NoteSnapshot) before, const Naui_List(Uph_NoteSnapshot) after)
{
	Naui_List(Uph_ActionBlockSnapshot) tail = NULL;
	uph_pattern_collect_tail_blocks(pattern_index, &tail);

	bool changed = false;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(before); i++)
	{
		const Uph_MidiNote *old_note = &before[i].note;
		const Uph_MidiNote *new_note = &after[i].note;
		if (old_note->start_beat == new_note->start_beat && old_note->length_beats == new_note->length_beats && old_note->key_number == new_note->key_number)
			continue;

		if (!changed)
			naui_action_group_start(group_name);

		Uph_ActionNoteTransform data = {
			.pattern_index = pattern_index, .note_index = before[i].index,
			.old_start = old_note->start_beat, .old_length = old_note->length_beats, .old_key = old_note->key_number,
			.new_start = new_note->start_beat, .new_length = new_note->length_beats, .new_key = new_note->key_number
		};
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_MOVE, data);
		changed = true;
	}

	if (changed)
	{
		uph_pattern_stretch_tail_blocks(pattern_index, tail);
		uph_pattern_commit_tail_blocks(tail);
		naui_action_group_end();
	}

	naui_list_free(tail);
	return changed;
}

static bool uph_note_legato(Uph_ResourceIndex pattern_index)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return false;

	Naui_List(Uph_NoteSnapshot) before = NULL;
	uph_note_collect_targets(pattern, &before, uph__note_snapshot_compare_start);
	Naui_List(Uph_NoteSnapshot) after = uph_note_snapshots_copy(before);

	const uint32_t count = (uint32_t)naui_list_len(before);
	double last_end = 0.0;
	for (uint32_t i = count; i > 0 && before[i - 1].note.start_beat >= before[count - 1].note.start_beat - UPH_NOTE_TOOL_EPSILON; i--)
		last_end = fmax(last_end, before[i - 1].note.start_beat + before[i - 1].note.length_beats);

	const double bar_length = uph_time_signature_bar_length();
	const double bar_end = ceil(last_end / bar_length - UPH_NOTE_TOOL_EPSILON) * bar_length;

	bool has_next = false;
	double next_start = 0.0;
	for (uint32_t i = count; i > 0; i--)
	{
		const double start = before[i - 1].note.start_beat;
		if (i < count && before[i].note.start_beat > start + UPH_NOTE_TOOL_EPSILON)
		{
			next_start = before[i].note.start_beat;
			has_next = true;
		}

		after[i - 1].note.length_beats = (has_next ? next_start : bar_end) - start;
	}

	const bool changed = uph_note_commit_transforms(pattern_index, UPH_ACTION_MIDI_NOTES_LEGATO_GROUP, before, after);
	naui_list_free(before);
	naui_list_free(after);
	return changed;
}

static bool uph_note_flip(Uph_ResourceIndex pattern_index, bool vertical)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return false;

	Naui_List(Uph_NoteSnapshot) before = NULL;
	uph_note_collect_targets(pattern, &before, uph__note_snapshot_compare_start);
	Naui_List(Uph_NoteSnapshot) after = uph_note_snapshots_copy(before);

	const uint32_t count = (uint32_t)naui_list_len(before);
	if (count > 0)
	{
		double first_start = before[0].note.start_beat;
		double last_end = before[0].note.start_beat + before[0].note.length_beats;
		int32_t low_key = before[0].note.key_number;
		int32_t high_key = before[0].note.key_number;
		for (uint32_t i = 1; i < count; i++)
		{
			const Uph_MidiNote *note = &before[i].note;
			first_start = fmin(first_start, note->start_beat);
			last_end = fmax(last_end, note->start_beat + note->length_beats);
			low_key = NAUI_MIN(low_key, (int32_t)note->key_number);
			high_key = NAUI_MAX(high_key, (int32_t)note->key_number);
		}

		for (uint32_t i = 0; i < count; i++)
		{
			Uph_MidiNote *note = &after[i].note;
			if (vertical)
				note->key_number = (uint8_t)(low_key + high_key - (int32_t)note->key_number);
			else
				note->start_beat = fmax(0.0, first_start + last_end - (note->start_beat + note->length_beats));
		}
	}

	const bool changed = uph_note_commit_transforms(pattern_index, UPH_ACTION_MIDI_NOTES_FLIP_GROUP, before, after);
	naui_list_free(before);
	naui_list_free(after);
	return changed;
}

static bool uph_note_strum(Uph_ResourceIndex pattern_index, double division)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return false;

	Naui_List(Uph_NoteSnapshot) before = NULL;
	uph_note_collect_targets(pattern, &before, uph__note_snapshot_compare_start);
	Naui_List(Uph_NoteSnapshot) after = uph_note_snapshots_copy(before);

	const uint32_t count = (uint32_t)naui_list_len(before);
	uint32_t chord_start = 0;
	while (chord_start < count)
	{
		uint32_t chord_end = chord_start + 1;
		double shortest = before[chord_start].note.length_beats;
		while (chord_end < count && before[chord_end].note.start_beat <= before[chord_start].note.start_beat + UPH_NOTE_TOOL_EPSILON)
		{
			shortest = fmin(shortest, before[chord_end].note.length_beats);
			chord_end++;
		}

		const uint32_t chord_size = chord_end - chord_start;
		if (chord_size > 1)
		{
			const double step = fmin(division * UPH_NOTE_STRUM_GRID_RATIO, shortest * UPH_NOTE_STRUM_MAX_LENGTH_RATIO / (double)(chord_size - 1));
			for (uint32_t i = 0; i < chord_size; i++)
			{
				Uph_MidiNote *note = &after[chord_start + i].note;
				note->start_beat += step * (double)i;
				note->length_beats -= step * (double)i;
			}
		}

		chord_start = chord_end;
	}

	const bool changed = uph_note_commit_transforms(pattern_index, UPH_ACTION_MIDI_NOTES_STRUM_GROUP, before, after);
	naui_list_free(before);
	naui_list_free(after);
	return changed;
}

static bool uph_note_glue(Uph_ResourceIndex pattern_index)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return false;

	Naui_List(Uph_NoteSnapshot) targets = NULL;
	uph_note_collect_targets(pattern, &targets, uph__note_snapshot_compare_key);

	Naui_List(uint32_t) absorbed = NULL;
	bool changed = false;
	const uint32_t count = (uint32_t)naui_list_len(targets);
	uint32_t run_start = 0;
	while (run_start < count)
	{
		const Uph_MidiNote *first = &targets[run_start].note;
		double run_end_beat = first->start_beat + first->length_beats;
		uint32_t run_end = run_start + 1;
		while (run_end < count && targets[run_end].note.key_number == first->key_number && targets[run_end].note.start_beat <= run_end_beat + UPH_NOTE_TOOL_EPSILON)
		{
			run_end_beat = fmax(run_end_beat, targets[run_end].note.start_beat + targets[run_end].note.length_beats);
			naui_list_push(absorbed, targets[run_end].index);
			run_end++;
		}

		if (run_end - run_start > 1)
		{
			if (!changed)
				naui_action_group_start(UPH_ACTION_MIDI_NOTES_GLUE_GROUP);

			Uph_ActionNoteTransform data = {
				.pattern_index = pattern_index, .note_index = targets[run_start].index,
				.old_start = first->start_beat, .old_length = first->length_beats, .old_key = first->key_number,
				.new_start = first->start_beat, .new_length = run_end_beat - first->start_beat, .new_key = first->key_number
			};
			naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_RESIZE, data);
			changed = true;
		}

		run_start = run_end;
	}

	if (changed)
	{
		qsort(absorbed, (size_t)naui_list_len(absorbed), sizeof(uint32_t), uph__note_index_compare_descending);
		for (uint32_t i = 0; i < (uint32_t)naui_list_len(absorbed); i++)
		{
			Uph_ActionNoteDelete data = { .pattern_index = pattern_index, .note_index = absorbed[i] };
			naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_DELETE, data);
		}

		naui_action_group_end();
	}

	naui_list_free(absorbed);
	naui_list_free(targets);
	return changed;
}

static bool uph_note_chop(Uph_ResourceIndex pattern_index, double division)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern || division <= 0.0)
		return false;

	Naui_List(Uph_NoteSnapshot) targets = NULL;
	uph_note_collect_targets(pattern, &targets, uph__note_snapshot_compare_start);

	bool changed = false;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(targets); i++)
	{
		const Uph_MidiNote *note = &targets[i].note;
		const double end_beat = note->start_beat + note->length_beats;
		int64_t line = (int64_t)floor(note->start_beat / division + UPH_NOTE_TOOL_EPSILON) + 1;
		if ((double)line * division >= end_beat - UPH_NOTE_TOOL_EPSILON)
			continue;

		if (!changed)
			naui_action_group_start(UPH_ACTION_MIDI_NOTES_CHOP_GROUP);

		Uph_ActionNoteTransform shrink = {
			.pattern_index = pattern_index, .note_index = targets[i].index,
			.old_start = note->start_beat, .old_length = note->length_beats, .old_key = note->key_number,
			.new_start = note->start_beat, .new_length = (double)line * division - note->start_beat, .new_key = note->key_number
		};
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_RESIZE, shrink);

		for (; (double)line * division < end_beat - UPH_NOTE_TOOL_EPSILON; line++)
		{
			const double piece_start = (double)line * division;
			Uph_ActionNoteCreate piece = { .pattern_index = pattern_index, .note_index = (uint32_t)naui_list_len(pattern->notes), .note = *note };
			piece.note.start_beat = piece_start;
			piece.note.length_beats = fmin(division, end_beat - piece_start);
			naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_CREATE, piece);
		}

		changed = true;
	}

	if (changed)
		naui_action_group_end();

	naui_list_free(targets);
	return changed;
}

static bool uph_note_cut(Uph_ResourceIndex pattern_index, uint32_t note_index, double cut_beat, double min_length)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern || note_index >= (uint32_t)naui_list_len(pattern->notes))
		return false;

	const Uph_MidiNote note = pattern->notes[note_index];
	const double left_length = cut_beat - note.start_beat;
	const double right_length = note.length_beats - left_length;
	if (left_length < min_length || right_length < min_length)
		return false;

	naui_action_group_start(UPH_ACTION_MIDI_NOTE_CUT_GROUP);

	Uph_ActionNoteTransform shrink = {
		.pattern_index = pattern_index, .note_index = note_index,
		.old_start = note.start_beat, .old_length = note.length_beats, .old_key = note.key_number,
		.new_start = note.start_beat, .new_length = left_length, .new_key = note.key_number
	};
	naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_RESIZE, shrink);

	Uph_ActionNoteCreate right = { .pattern_index = pattern_index, .note_index = (uint32_t)naui_list_len(pattern->notes), .note = note };
	right.note.start_beat = cut_beat;
	right.note.length_beats = right_length;
	naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_CREATE, right);

	naui_action_group_end();
	return true;
}

#pragma endregion

#pragma region Pedals Edit
static bool uph_pedals_apply(Uph_ResourceIndex pattern_index, Naui_List(Uph_MidiPedalEvent) pedals)
{
	Uph_MidiPattern* pattern = uph_note_pattern(pattern_index);
	if (!pattern)
		return false;

	Naui_List(Uph_MidiPedalEvent) previous = pattern->pedals;
	pattern->pedals = naui_list_clone(pedals);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->pedals); i++)
		pattern->pedals[i].selected = false;

	naui_list_free(previous);
	uph_state.shared.current_pattern_updated = true;
	return true;
}

bool uph__action_pedals_edit_execute(void* userdata)
{
	Uph_ActionPedalsEdit* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_note_pattern(data->pattern_index) != NULL;
	}

	return uph_pedals_apply(data->pattern_index, data->new_pedals);
}

bool uph__action_pedals_edit_undo(void* userdata)
{
	Uph_ActionPedalsEdit* data = userdata;
	return uph_pedals_apply(data->pattern_index, data->old_pedals);
}

bool uph__action_pedals_edit_redo(void* userdata)
{
	Uph_ActionPedalsEdit* data = userdata;
	return uph_pedals_apply(data->pattern_index, data->new_pedals);
}

void uph__action_pedals_edit_destroy(void* userdata)
{
	Uph_ActionPedalsEdit* data = userdata;
	naui_list_free(data->old_pedals);
	naui_list_free(data->new_pedals);
}

#pragma endregion
