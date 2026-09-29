bool _uph_action_pattern_create_execute(void* userdata)
{
	Uph_ActionResourceCreate* data = userdata;
	uph_resources_add_pattern();
	data->resource_index = naui_list_len(uph_state.project.midi_patterns) - 1;
	data->resource_exists = true;
	return true;
}

bool _uph_action_pattern_create_undo(void* userdata)
{
	Uph_ActionResourceCreate* data = userdata;
	if (!data->resource_exists)
		return false;

	uph_resources_remove_pattern(data->resource_index);
	data->resource_exists = false;
	return true;
}

bool _uph_action_pattern_create_redo(void* userdata)
{
	return _uph_action_pattern_create_execute(userdata);
}
