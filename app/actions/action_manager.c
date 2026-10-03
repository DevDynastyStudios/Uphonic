#define UPH_REGISTER_ACTION(name, prefix) \
	naui_register_action(name, \
	{ \
		.execute = prefix##_execute, \
		.undo = prefix##_undo, \
		.redo = prefix##_redo \
	})

#define UPH_REGISTER_ACTION_DESTROY(name, prefix) \
	naui_register_action(name, \
	{ \
		.execute = prefix##_execute, \
		.undo = prefix##_undo, \
		.redo = prefix##_redo, \
		.destroy = prefix##_destroy \
	})

static void uph_action_refresh_block_defaults(void)
{
	const Uph_ResourceType type = uph_state.shared.selected_resource.type;
	const Uph_ResourceIndex index = uph_state.shared.selected_resource.index;

	uph_state.shared.song_timeline_current_block_start_offset = 0.0;

	if (type == UPH_RESOURCE_PATTERN && index < (uint32_t)naui_list_len(uph_state.project.midi_patterns))
	{
		uph_state.shared.song_timeline_current_block_length = uph_calculate_pattern_length(&uph_state.project.midi_patterns[index]);
	}
	else if (type == UPH_RESOURCE_AUTOMATION && index < (uint32_t)naui_list_len(uph_state.project.automations))
	{
		uph_state.shared.song_timeline_current_block_length = uph_calculate_automation_length(&uph_state.project.automations[index]);
	}
	else if (type == UPH_RESOURCE_SAMPLE && index < (uint32_t)naui_list_len(uph_state.project.samples))
	{
		const Uph_Sample* sample = &uph_state.project.samples[index];
		if (sample->data_index < (uint32_t)naui_list_len(uph_state.project.sample_data))
		{
			const Uph_SampleData* data = &uph_state.project.sample_data[sample->data_index];
			uph_state.shared.song_timeline_current_block_length = ceil(uph_frames_to_beats(data->frame_count, uph_state.settings.audio.sample_rate, uph_state.project.bpm));
		}
	}
}

void uph_action_resource_removed(Uph_ResourceType type, Uph_ResourceIndex index)
{
	typeof(uph_state.shared.selected_resource)* selected = &uph_state.shared.selected_resource;
	selected->renaming = false;
	if (selected->type != type)
		return;

	uint32_t count = 0;
	if (type == UPH_RESOURCE_PATTERN)
		count = (uint32_t)naui_list_len(uph_state.project.midi_patterns);
	else if (type == UPH_RESOURCE_SAMPLE)
		count = (uint32_t)naui_list_len(uph_state.project.samples);
	else if (type == UPH_RESOURCE_AUTOMATION)
		count = (uint32_t)naui_list_len(uph_state.project.automations);

	if (selected->index > index)
		selected->index--;
	else if (selected->index == index)
	{
		if (count == 0)
			selected->type = UPH_RESOURCE_NONE;
		else if (index >= count)
			selected->index = count - 1;
	}

	uph_action_refresh_block_defaults();
}

void uph_action_resource_restored(Uph_ResourceType type, Uph_ResourceIndex index)
{
	uph_state.shared.selected_resource.type = type;
	uph_state.shared.selected_resource.index = index;
	uph_state.shared.selected_resource.renaming = false;
	uph_action_refresh_block_defaults();
}

void uph_action_initialize()
{
	UPH_REGISTER_ACTION(UPH_ACTION_PATTERN_CREATE, _uph_action_pattern_create);
	UPH_REGISTER_ACTION_DESTROY(UPH_ACTION_PATTERN_DELETE, _uph_action_pattern_delete);
	UPH_REGISTER_ACTION(UPH_ACTION_PATTERN_RENAME, _uph_action_pattern_rename);
	UPH_REGISTER_ACTION(UPH_ACTION_PATTERN_DUPLICATE, _uph_action_pattern_duplicate);

	UPH_REGISTER_ACTION_DESTROY(UPH_ACTION_SAMPLE_DELETE, _uph_action_sample_delete);
	UPH_REGISTER_ACTION(UPH_ACTION_SAMPLE_RENAME, _uph_action_sample_rename);
	UPH_REGISTER_ACTION(UPH_ACTION_SAMPLE_DUPLICATE, _uph_action_sample_duplicate);

	UPH_REGISTER_ACTION(UPH_ACTION_TRACK_CREATE, _uph_action_track_create);
	UPH_REGISTER_ACTION_DESTROY(UPH_ACTION_TRACK_DELETE, _uph_action_track_delete);
	UPH_REGISTER_ACTION(UPH_ACTION_TRACK_AUTOMATION_CREATE, _uph_action_track_automation_create);
	UPH_REGISTER_ACTION(UPH_ACTION_TRACK_RENAME, _uph_action_track_rename);
	UPH_REGISTER_ACTION(UPH_ACTION_TRACK_COLOR, _uph_action_track_color);
	UPH_REGISTER_ACTION(UPH_ACTION_TRACK_MUTE, _uph_action_track_state);
	UPH_REGISTER_ACTION(UPH_ACTION_TRACK_ARM, _uph_action_track_state);
	UPH_REGISTER_ACTION_DESTROY(UPH_ACTION_TRACK_SOLO, _uph_action_track_solo);
	UPH_REGISTER_ACTION(UPH_ACTION_TRACK_VOLUME, _uph_action_track_value);
	UPH_REGISTER_ACTION(UPH_ACTION_TRACK_PAN, _uph_action_track_value);

	UPH_REGISTER_ACTION(UPH_ACTION_AUTOMATION_CREATE, _uph_action_automation_create);
	UPH_REGISTER_ACTION_DESTROY(UPH_ACTION_AUTOMATION_DELETE, _uph_action_automation_delete);
	UPH_REGISTER_ACTION(UPH_ACTION_AUTOMATION_RENAME, _uph_action_automation_rename);
	UPH_REGISTER_ACTION(UPH_ACTION_AUTOMATION_DUPLICATE, _uph_action_automation_duplicate);
	UPH_REGISTER_ACTION(UPH_ACTION_AUTOMATION_POINT_CREATE, _uph_action_automation_point_create);
	UPH_REGISTER_ACTION(UPH_ACTION_AUTOMATION_POINT_DELETE, _uph_action_automation_point_delete);
	UPH_REGISTER_ACTION(UPH_ACTION_AUTOMATION_POINT_MOVE, _uph_action_automation_point_move);

	UPH_REGISTER_ACTION(UPH_ACTION_BLOCK_CREATE, _uph_action_block_create);
	UPH_REGISTER_ACTION(UPH_ACTION_BLOCK_DELETE, _uph_action_block_delete);
	UPH_REGISTER_ACTION(UPH_ACTION_BLOCK_MOVE, _uph_action_block_transform);
	UPH_REGISTER_ACTION(UPH_ACTION_BLOCK_RESIZE, _uph_action_block_transform);

	UPH_REGISTER_ACTION(UPH_ACTION_MIDI_NOTE_CREATE, _uph_action_note_create);
	UPH_REGISTER_ACTION(UPH_ACTION_MIDI_NOTE_DELETE, _uph_action_note_delete);
	UPH_REGISTER_ACTION(UPH_ACTION_MIDI_NOTE_MOVE, _uph_action_note_transform);
	UPH_REGISTER_ACTION(UPH_ACTION_MIDI_NOTE_RESIZE, _uph_action_note_transform);
}

void uph_action_shutdown()
{

}
