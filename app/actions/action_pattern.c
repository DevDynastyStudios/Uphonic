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

#pragma region Pattern Create
bool _uph_action_pattern_create_execute(void* userdata)
{
	Uph_ActionResourceCreate* data = userdata;
	uph_resources_add_pattern();
	data->resource_index = (uint32_t)naui_list_len(uph_state.project.midi_patterns) - 1;
	data->resource_exists = true;
	uph_action_resource_restored(UPH_RESOURCE_PATTERN, data->resource_index);
	return true;
}

bool _uph_action_pattern_create_undo(void* userdata)
{
	Uph_ActionResourceCreate* data = userdata;
	if (!data->resource_exists || data->resource_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	naui_list_free(uph_state.project.midi_patterns[data->resource_index].notes);
	naui_list_remove(uph_state.project.midi_patterns, data->resource_index);
	uph_action_resource_removed(UPH_RESOURCE_PATTERN, data->resource_index);
	data->resource_exists = false;
	return true;
}

bool _uph_action_pattern_create_redo(void* userdata)
{
	return _uph_action_pattern_create_execute(userdata);
}

#pragma endregion

#pragma region Pattern Delete
bool _uph_action_pattern_delete_execute(void* userdata)
{
	Uph_ActionPatternDelete* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	naui_list_free(data->blocks);
	data->blocks = NULL;

	data->pattern = uph_state.project.midi_patterns[data->resource_index];
	uph_block_unlink_resource(UPH_RESOURCE_PATTERN, data->resource_index, &data->blocks);
	naui_list_remove(uph_state.project.midi_patterns, data->resource_index);

	data->deleted = true;
	uph_state.shared.current_pattern_updated = true;
	uph_action_resource_removed(UPH_RESOURCE_PATTERN, data->resource_index);
	return true;
}

bool _uph_action_pattern_delete_undo(void* userdata)
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

bool _uph_action_pattern_delete_redo(void* userdata)
{
	return _uph_action_pattern_delete_execute(userdata);
}

void _uph_action_pattern_delete_destroy(void* userdata)
{
	Uph_ActionPatternDelete* data = userdata;
	if (data->deleted)
		naui_list_free(data->pattern.notes);

	naui_list_free(data->blocks);
	data->blocks = NULL;
}

#pragma endregion

#pragma region Pattern Rename
bool _uph_action_pattern_rename_execute(void* userdata)
{
	Uph_ActionResourceRename* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	uph_state.project.midi_patterns[data->resource_index].name = data->new_name;
	return true;
}

bool _uph_action_pattern_rename_undo(void* userdata)
{
	Uph_ActionResourceRename* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	uph_state.project.midi_patterns[data->resource_index].name = data->old_name;
	return true;
}

bool _uph_action_pattern_rename_redo(void* userdata)
{
	return _uph_action_pattern_rename_execute(userdata);
}

#pragma endregion

#pragma region Pattern Duplicate
bool _uph_action_pattern_duplicate_execute(void* userdata)
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

bool _uph_action_pattern_duplicate_undo(void* userdata)
{
	Uph_ActionPatternDuplicate* data = userdata;
	if (!data->resource_exists || data->new_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
		return false;

	naui_list_free(uph_state.project.midi_patterns[data->new_index].notes);
	naui_list_remove(uph_state.project.midi_patterns, data->new_index);
	uph_action_resource_removed(UPH_RESOURCE_PATTERN, data->new_index);
	data->resource_exists = false;
	return true;
}

bool _uph_action_pattern_duplicate_redo(void* userdata)
{
	return _uph_action_pattern_duplicate_execute(userdata);
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
bool _uph_action_note_create_execute(void* userdata)
{
	Uph_ActionNoteCreate* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_note_pattern(data->pattern_index) != NULL;
	}

	return uph_note_list_insert(data->pattern_index, data->note_index, data->note);
}

bool _uph_action_note_create_undo(void* userdata)
{
	Uph_ActionNoteCreate* data = userdata;
	return uph_note_list_take(data->pattern_index, data->note_index, &data->note);
}

bool _uph_action_note_create_redo(void* userdata)
{
	Uph_ActionNoteCreate* data = userdata;
	return uph_note_list_insert(data->pattern_index, data->note_index, data->note);
}

#pragma endregion

#pragma region Note Delete
bool _uph_action_note_delete_execute(void* userdata)
{
	Uph_ActionNoteDelete* data = userdata;
	return uph_note_list_take(data->pattern_index, data->note_index, &data->note);
}

bool _uph_action_note_delete_undo(void* userdata)
{
	Uph_ActionNoteDelete* data = userdata;
	return uph_note_list_insert(data->pattern_index, data->note_index, data->note);
}

bool _uph_action_note_delete_redo(void* userdata)
{
	return _uph_action_note_delete_execute(userdata);
}

#pragma endregion

#pragma region Note Move / Resize
bool _uph_action_note_transform_execute(void* userdata)
{
	Uph_ActionNoteTransform* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_note_pattern(data->pattern_index) != NULL;
	}

	return uph_note_apply(data->pattern_index, data->note_index, data->new_start, data->new_length, data->new_key);
}

bool _uph_action_note_transform_undo(void* userdata)
{
	Uph_ActionNoteTransform* data = userdata;
	return uph_note_apply(data->pattern_index, data->note_index, data->old_start, data->old_length, data->old_key);
}

bool _uph_action_note_transform_redo(void* userdata)
{
	Uph_ActionNoteTransform* data = userdata;
	return uph_note_apply(data->pattern_index, data->note_index, data->new_start, data->new_length, data->new_key);
}

#pragma endregion

#pragma region Note Selection
typedef struct
{
	uint32_t index;
	Uph_MidiNote note;
} Uph_NoteSnapshot;

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

	naui_action_group_start(UPH_ACTION_MIDI_NOTES_DUPLICATE_GROUP);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		Uph_ActionNoteCreate data = { .pattern_index = pattern_index, .note_index = (uint32_t)naui_list_len(pattern->notes), .note = selected[i].note };
		data.note.start_beat += shift;
		data.note.selected = true;
		naui_action_execute_stack(UPH_ACTION_MIDI_NOTE_CREATE, data);
	}

	naui_action_group_end();
	naui_list_free(selected);
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
