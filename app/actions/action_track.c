typedef struct
{
	uint32_t resource_index;
	bool resource_exists;
} Uph_ActionTrackCreate;

typedef struct
{
	uint32_t resource_index;
	Naui_String old_name;
	Naui_String new_name;
} Uph_ActionTrackRename;

#pragma region Track Create
bool _uph_action_track_create_execute(void* userdata)
{
	Uph_ActionTrackCreate* data = userdata;
	uph_resources_add_track(naui_string_from_cstr(NAUI_TR("song_timeline.track.title")));
	data->resource_index = naui_list_len(uph_state.project.tracks) - 1;
	data->resource_exists = true;
	return true;
}

bool _uph_action_track_create_undo(void* userdata)
{
	Uph_ActionTrackCreate* data = userdata;
	if (!data->resource_exists)
		return false;

	uph_resources_remove_track(&uph_state.project.tracks[data->resource_index]);
	data->resource_exists = false;
	return true;
}

bool _uph_action_track_create_redo(void* userdata)
{
	return _uph_action_track_create_execute(userdata);
}

#pragma endregion

#pragma region Track Rename

bool _uph_action_track_rename_execute(void* userdata)
{
	Uph_ActionTrackRename* data = userdata;
	data->resource_index = naui_list_len(uph_state.project.tracks) - 1;

	Uph_Track* track = &uph_state.project.tracks[data->resource_index];
	data->old_name = track->name;
	track->name = data->new_name;
	return true;
}

bool _uph_action_track_rename_undo(void* userdata)
{
	Uph_ActionTrackRename* data = userdata;
	data->resource_index = naui_list_len(uph_state.project.tracks) - 1;

	Uph_Track* track = &uph_state.project.tracks[data->resource_index];
	track->name = data->old_name;
	return true;
}

bool _uph_action_track_rename_redo(void* userdata)
{
	return _uph_action_track_rename_execute(userdata);
}

#pragma endregion