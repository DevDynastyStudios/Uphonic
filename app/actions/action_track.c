typedef struct
{
	uint32_t resource_index;
	bool resource_exists;
} Uph_ActionTrackCreate;

typedef struct
{
	Uph_ActionTrackRef parent;
	Naui_String name;
	int32_t effect_index;
	uint64_t param_id;
	uint32_t index;
	bool resource_exists;
} Uph_ActionTrackAutomationCreate;

typedef struct
{
	Uph_ActionTrackRef track;
	Uph_Track removed;
	bool deleted;
} Uph_ActionTrackDelete;

typedef struct
{
	Uph_ActionTrackRef track;
	Naui_String old_name;
	Naui_String new_name;
} Uph_ActionTrackRename;

typedef struct
{
	Uph_ActionTrackRef track;
	int32_t old_color;
	int32_t new_color;
} Uph_ActionTrackColor;

typedef struct
{
	Uph_ActionTrackRef track;
	Uph_TrackState old_state;
	Uph_TrackState new_state;
} Uph_ActionTrackState;

typedef struct
{
	Uph_ActionTrackRef track;
	Uph_ActionTrackRef old_solo;
	Uph_ActionTrackRef new_solo;
} Uph_ActionTrackSolo;

typedef uint8_t Uph_ActionTrackValueKind;
enum
{
	UPH_ACTION_TRACK_VALUE_VOLUME,
	UPH_ACTION_TRACK_VALUE_PAN
};

typedef struct
{
	Uph_ActionTrackRef track;
	Uph_ActionTrackValueKind kind;
	float old_value;
	float new_value;
} Uph_ActionTrackValue;

#pragma region Track References

#define UPH_ACTION_TRACK_TOP          (-1)
#define UPH_ACTION_TRACK_MASTER_INDEX UINT32_MAX

static inline Uph_ActionTrackRef uph_action_track_ref_master(void)
{
	return (Uph_ActionTrackRef){ UPH_ACTION_TRACK_TOP, UPH_ACTION_TRACK_MASTER_INDEX };
}

static inline bool uph_action_track_ref_is_master(Uph_ActionTrackRef ref)
{
	return ref.parent == UPH_ACTION_TRACK_TOP && ref.index == UPH_ACTION_TRACK_MASTER_INDEX;
}

static Naui_List(Uph_Track) *uph_action_track_list_ptr(int32_t parent)
{
	Uph_Project *project = &uph_state.project;

	if (parent == UPH_ACTION_TRACK_TOP)
		return &project->tracks;

	if (parent >= 0 && (uint32_t)parent < (uint32_t)naui_list_len(project->tracks))
		return &project->tracks[parent].subtracks;

	return NULL;
}

Uph_ActionTrackRef uph_action_track_ref(const Uph_Track *track)
{
	const Uph_ActionTrackRef invalid = { UPH_ACTION_TRACK_INVALID, 0 };
	if (!track)
		return invalid;

	Naui_List(Uph_Track) tracks = uph_state.project.tracks;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		if (&tracks[i] == track)
			return (Uph_ActionTrackRef){ UPH_ACTION_TRACK_TOP, i };

		for (uint32_t s = 0; s < (uint32_t)naui_list_len(tracks[i].subtracks); s++)
		{
			if (&tracks[i].subtracks[s] == track)
				return (Uph_ActionTrackRef){ (int32_t)i, s };
		}
	}

	return invalid;
}


Uph_Track *uph_action_track_resolve(Uph_ActionTrackRef ref)
{
	if (ref.parent == UPH_ACTION_TRACK_INVALID)
		return NULL;

	if (uph_action_track_ref_is_master(ref))
		return NULL;

	Naui_List(Uph_Track) *list = uph_action_track_list_ptr(ref.parent);
	if (!list || ref.index >= (uint32_t)naui_list_len(*list))
		return NULL;

	return &(*list)[ref.index];
}

static Uph_TrackState *uph_action_track_state_ptr(Uph_ActionTrackRef ref)
{
	if (uph_action_track_ref_is_master(ref))
		return &uph_state.project.master_track.state;

	Uph_Track *track = uph_action_track_resolve(ref);
	return track ? &track->state : NULL;
}

static float *uph_action_track_value_ptr(Uph_ActionTrackRef ref, Uph_ActionTrackValueKind kind)
{
	if (uph_action_track_ref_is_master(ref))
	{
		Uph_MasterTrack *master = &uph_state.project.master_track;
		return kind == UPH_ACTION_TRACK_VALUE_PAN ? &master->pan : &master->volume;
	}

	Uph_Track *track = uph_action_track_resolve(ref);
	if (!track)
		return NULL;

	return kind == UPH_ACTION_TRACK_VALUE_PAN ? &track->pan : &track->volume;
}

typedef struct
{
	Uph_ActionTrackRef mixer;
	Uph_ActionTrackRef plugin_list;
	Uph_ActionTrackRef soloed;
} Uph_ActionTrackPointers;

static Uph_ActionTrackPointers uph_action_track_pointers_capture(void)
{
	return (Uph_ActionTrackPointers){
		.mixer = uph_action_track_ref(uph_state.shared.current_mixer_track),
		.plugin_list = uph_action_track_ref(uph_state.shared.current_plugin_list_track),
		.soloed = uph_action_track_ref(uph_state.project.soloed_track)
	};
}

static Uph_Track *uph_action_track_pointer_shift(Uph_ActionTrackRef ref, Uph_ActionTrackRef changed, int32_t delta)
{
	if (ref.parent == UPH_ACTION_TRACK_INVALID)
		return NULL;

	if (delta < 0)
	{
		if (ref.parent == changed.parent && ref.index == changed.index)
			return NULL;
		if (changed.parent == UPH_ACTION_TRACK_TOP && ref.parent == (int32_t)changed.index)
			return NULL;
	}

	if (ref.parent == changed.parent)
	{
		if (delta > 0 ? ref.index >= changed.index : ref.index > changed.index)
			ref.index = (uint32_t)((int32_t)ref.index + delta);
	}
	else if (changed.parent == UPH_ACTION_TRACK_TOP && ref.parent >= 0)
	{
		if (delta > 0 ? ref.parent >= (int32_t)changed.index : ref.parent > (int32_t)changed.index)
			ref.parent += delta;
	}

	return uph_action_track_resolve(ref);
}

static void uph_action_tracks_changed(Uph_ActionTrackPointers before, Uph_ActionTrackRef changed, int32_t delta)
{
	uph_resources_link_tracks(uph_state.project.tracks);
	uph_resources_refresh_param_usage(uph_state.project.tracks);
	uph_state.shared.current_mixer_track = uph_action_track_pointer_shift(before.mixer, changed, delta);
	// The rest of the multi-selection can't be shifted (pointers), so it collapses to the current track.
	naui_list_clear(uph_state.shared.selected_mixer_tracks);
	if (uph_state.shared.current_mixer_track)
		naui_list_push(uph_state.shared.selected_mixer_tracks, uph_state.shared.current_mixer_track);
	uph_state.shared.current_plugin_list_track = uph_action_track_pointer_shift(before.plugin_list, changed, delta);
	uph_state.project.soloed_track = uph_action_track_pointer_shift(before.soloed, changed, delta);
	uph_song_timeline_invalidate_tracks();
}

static void uph_action_track_hide_plugins(Uph_Track *track)
{
	if (track->instrument.loaded)
		uph_plugin_hide_window(&track->instrument);

	for (uint32_t e = 0; e < (uint32_t)naui_list_len(track->effects); e++)
	{
		if (track->effects[e].plugin.loaded)
			uph_plugin_hide_window(&track->effects[e].plugin);
	}

	for (uint32_t s = 0; s < (uint32_t)naui_list_len(track->subtracks); s++)
		uph_action_track_hide_plugins(&track->subtracks[s]);
}

void uph_action_track_unload_plugin(Uph_Track *track, Uph_Plugin *plugin)
{
	const bool was_running = uph_audio_engine_device_running();
	if (was_running)
		uph_audio_engine_stop();

	bool removed_lane = false;
	for (int32_t s = (int32_t)naui_list_len(track->subtracks) - 1; s >= 0; s--)
	{
		Uph_Track *lane = &track->subtracks[s];
		if (lane->type != UPH_RESOURCE_AUTOMATION || !uph_plugin_owns_param(plugin, lane->automation_param))
			continue;

		const Uph_ActionTrackRef ref = uph_action_track_ref(lane);
		const Uph_ActionTrackPointers before = uph_action_track_pointers_capture();
		uph_resources_remove_track(lane);
		uph_action_tracks_changed(before, ref, -1);
		removed_lane = true;
	}

	if (removed_lane)
		naui_action_clear_history();

	uph_plugin_unload(plugin);

	if (was_running)
		uph_audio_engine_start();
}

void uph_action_master_unload_plugin(Uph_Plugin *plugin)
{
	const bool was_running = uph_audio_engine_device_running();
	if (was_running)
		uph_audio_engine_stop();

	uph_plugin_unload(plugin);

	if (was_running)
		uph_audio_engine_start();
}

#pragma endregion

#pragma region Track Create
bool _uph_action_track_create_execute(void* userdata)
{
	Uph_ActionTrackCreate* data = userdata;
	const Uph_ActionTrackPointers before = uph_action_track_pointers_capture();

	uph_resources_add_track(naui_string_from_cstr(NAUI_TR("song_timeline.track.title")), rand() % naui_list_len(naui_theme_color_list("uph_track_palette")));
	data->resource_index = (uint32_t)naui_list_len(uph_state.project.tracks) - 1;
	data->resource_exists = true;

	uph_action_tracks_changed(before, (Uph_ActionTrackRef){ -1, data->resource_index }, 1);
	return true;
}

bool _uph_action_track_create_undo(void* userdata)
{
	Uph_ActionTrackCreate* data = userdata;
	if (!data->resource_exists)
		return false;

	Uph_Track* track = uph_action_track_resolve((Uph_ActionTrackRef){ -1, data->resource_index });
	if (!track)
		return false;

	const Uph_ActionTrackPointers before = uph_action_track_pointers_capture();
	uph_resources_remove_track(track);
	data->resource_exists = false;

	uph_action_tracks_changed(before, (Uph_ActionTrackRef){ -1, data->resource_index }, -1);
	return true;
}

bool _uph_action_track_create_redo(void* userdata)
{
	return _uph_action_track_create_execute(userdata);
}

#pragma endregion

#pragma region Track Delete
bool _uph_action_track_delete_execute(void* userdata)
{
	Uph_ActionTrackDelete* data = userdata;
	if (uph_action_track_ref_is_master(data->track))
		return false;

	Uph_Track* track = uph_action_track_resolve(data->track);
	if (!track)
		return false;

	Naui_List(Uph_Track) *list = uph_action_track_list_ptr(data->track.parent);
	if (!list)
		return false;

	const Uph_ActionTrackPointers before = uph_action_track_pointers_capture();
	data->removed = *track;
	uph_action_track_hide_plugins(&data->removed);

	naui_list_remove(*list, data->track.index);

	data->deleted = true;
	uph_action_tracks_changed(before, data->track, -1);
	return true;
}


bool _uph_action_track_delete_undo(void* userdata)
{
	Uph_ActionTrackDelete* data = userdata;
	if (!data->deleted)
		return false;

	Naui_List(Uph_Track) *list = uph_action_track_list_ptr(data->track.parent);
	if (!list || data->track.index > (uint32_t)naui_list_len(*list))
		return false;

	const Uph_ActionTrackPointers before = uph_action_track_pointers_capture();
	naui_list_insert(*list, data->removed, data->track.index);

	data->deleted = false;
	uph_action_tracks_changed(before, data->track, 1);
	return true;
}

bool _uph_action_track_delete_redo(void* userdata)
{
	return _uph_action_track_delete_execute(userdata);
}

void _uph_action_track_delete_destroy(void* userdata)
{
	Uph_ActionTrackDelete* data = userdata;
	if (!data->deleted)
		return;

	Naui_List(Uph_Track) detached = NULL;
	naui_list_push(detached, data->removed);
	uph_resources_clear_tracks_recursive(detached);
	data->deleted = false;
}

#pragma endregion

#pragma region Track Rename
bool _uph_action_track_rename_execute(void* userdata)
{
	Uph_ActionTrackRename* data = userdata;
	Uph_Track* track = uph_action_track_resolve(data->track);
	if (!track)
		return false;

	track->name = data->new_name;
	return true;
}

bool _uph_action_track_rename_undo(void* userdata)
{
	Uph_ActionTrackRename* data = userdata;
	Uph_Track* track = uph_action_track_resolve(data->track);
	if (!track)
		return false;

	track->name = data->old_name;
	return true;
}

bool _uph_action_track_rename_redo(void* userdata)
{
	return _uph_action_track_rename_execute(userdata);
}

#pragma endregion

#pragma region Track Color
bool _uph_action_track_color_execute(void* userdata)
{
	Uph_ActionTrackColor* data = userdata;
	Uph_Track* track = uph_action_track_resolve(data->track);
	if (!track)
		return false;

	track->color_index = data->new_color;
	return true;
}

bool _uph_action_track_color_undo(void* userdata)
{
	Uph_ActionTrackColor* data = userdata;
	Uph_Track* track = uph_action_track_resolve(data->track);
	if (!track)
		return false;

	track->color_index = data->old_color;
	return true;
}

bool _uph_action_track_color_redo(void* userdata)
{
	return _uph_action_track_color_execute(userdata);
}

#pragma endregion

#pragma region Track Mute / Arm
bool _uph_action_track_state_execute(void* userdata)
{
	Uph_ActionTrackState* data = userdata;
	Uph_TrackState* state = uph_action_track_state_ptr(data->track);
	if (!state)
		return false;

	*state = data->new_state;
	return true;
}

bool _uph_action_track_state_undo(void* userdata)
{
	Uph_ActionTrackState* data = userdata;
	Uph_TrackState* state = uph_action_track_state_ptr(data->track);
	if (!state)
		return false;

	*state = data->old_state;
	return true;
}

bool _uph_action_track_state_redo(void* userdata)
{
	return _uph_action_track_state_execute(userdata);
}

#pragma endregion

#pragma region Track Solo
bool _uph_action_track_solo_execute(void* userdata)
{
	Uph_ActionTrackSolo* data = userdata;
	Uph_Track* track = uph_action_track_resolve(data->track);
	// Groups and the tracks inside them can be soloed; automation lanes can't. There is only ever one solo
	// (project->soloed_track), so soloing something un-solos whatever was soloed before, in any group.
	if (!track || track->type == UPH_RESOURCE_AUTOMATION)
		return false;

	Uph_Project* project = &uph_state.project;
	data->old_solo = uph_action_track_ref(project->soloed_track);
	project->soloed_track = (project->soloed_track == track) ? NULL : track;
	data->new_solo = uph_action_track_ref(project->soloed_track);
	return true;
}

bool _uph_action_track_solo_undo(void* userdata)
{
	Uph_ActionTrackSolo* data = userdata;
	uph_state.project.soloed_track = uph_action_track_resolve(data->old_solo);
	return true;
}

bool _uph_action_track_solo_redo(void* userdata)
{
	Uph_ActionTrackSolo* data = userdata;
	uph_state.project.soloed_track = uph_action_track_resolve(data->new_solo);
	return true;
}

#pragma endregion

#pragma region Track Volume / Pan
static bool uph_action_track_value_set(Uph_ActionTrackValue* data, float value)
{
	float* target = uph_action_track_value_ptr(data->track, data->kind);
	if (!target)
		return false;

	*target = value;
	return true;
}

bool _uph_action_track_value_execute(void* userdata)
{
	Uph_ActionTrackValue* data = userdata;
	return uph_action_track_value_set(data, data->new_value);
}

bool _uph_action_track_value_undo(void* userdata)
{
	Uph_ActionTrackValue* data = userdata;
	return uph_action_track_value_set(data, data->old_value);
}

bool _uph_action_track_value_redo(void* userdata)
{
	return _uph_action_track_value_execute(userdata);
}

#pragma endregion

#pragma region Automation Track Create
bool _uph_action_track_automation_create_execute(void* userdata)
{
	Uph_ActionTrackAutomationCreate* data = userdata;
	Uph_Track* parent = uph_action_track_resolve(data->parent);
	if (!parent || data->parent.parent >= 0)
		return false;

	Uph_PluginParam* param = uph_resources_find_param(parent, data->effect_index, data->param_id);
	if (!param)
		return false;

	const Uph_ActionTrackPointers before = uph_action_track_pointers_capture();
	uph_resources_add_automation_track(parent, data->name, param);
	data->index = (uint32_t)naui_list_len(parent->subtracks) - 1;
	data->resource_exists = true;
	uph_action_tracks_changed(before, (Uph_ActionTrackRef){ (int32_t)data->parent.index, data->index }, 1);
	return true;
}

bool _uph_action_track_automation_create_undo(void* userdata)
{
	Uph_ActionTrackAutomationCreate* data = userdata;
	if (!data->resource_exists)
		return false;

	const Uph_ActionTrackRef ref = { (int32_t)data->parent.index, data->index };
	Uph_Track* track = uph_action_track_resolve(ref);
	if (!track)
		return false;

	const Uph_ActionTrackPointers before = uph_action_track_pointers_capture();
	uph_resources_remove_track(track);
	data->resource_exists = false;
	uph_action_tracks_changed(before, ref, -1);
	return true;
}

bool _uph_action_track_automation_create_redo(void* userdata)
{
	return _uph_action_track_automation_create_execute(userdata);
}

#pragma endregion