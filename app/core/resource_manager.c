#pragma region Helpers
static void uph_resources_link_track_list(Naui_List(Uph_Track) tracks, Uph_Track *parent)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		tracks[i].parent = parent;
		tracks[i].index = i;
		uph_resources_link_track_list(tracks[i].subtracks, &tracks[i]);
	}
}

static void uph_resources_remove_track_children(Uph_Track *track)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		uph_resources_remove_track_children(&track->subtracks[i]);
		uph_plugin_unload(&track->subtracks[i].instrument);
		naui_list_free(track->subtracks[i].blocks);
	}
	naui_list_clear(track->subtracks);
}

void uph_resources_clear_tracks_recursive(Naui_List(Uph_Track) list)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
	{
		Uph_Track *track = &list[i];
		uph_resources_clear_tracks_recursive(track->subtracks);
		uph_plugin_unload(&track->instrument);

		for (uint32_t e = 0; e < (uint32_t)naui_list_len(track->effects); e++)
			uph_plugin_unload(&track->effects[e].plugin);

		naui_list_free(track->effects);
		naui_list_free(track->blocks);
	}

	naui_list_free(list);
}

static void uph_resources_clear_timeline_blocks_with_resource(Uph_ResourceType track_type, Uph_ResourceIndex resource_index)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
	{
		if (uph_state.project.tracks[i].type != track_type)
			continue;

		Naui_List(Uph_TimelineBlock) blocks = uph_state.project.tracks[i].blocks;
		for (uint32_t j = 0; j < (uint32_t)naui_list_len(blocks); j++)
		{
			if (blocks[j].resource_index == resource_index)
			{
				naui_list_uremove(blocks, j);
				j--;
			}
			else if (blocks[j].resource_index > resource_index)
			{
				blocks[j].resource_index--;
			}
		}

		if (naui_list_len(blocks) == 0)
			uph_state.project.tracks[i].type = UPH_RESOURCE_NONE;
	}
}

static void uph_resources_unload_plugins_recursive(Naui_List(Uph_Track) list)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
	{
		Uph_Track *track = &list[i];
		uph_resources_unload_plugins_recursive(track->subtracks);
		uph_plugin_unload(&track->instrument);

		for (uint32_t e = 0; e < (uint32_t)naui_list_len(track->effects); e++)
			uph_plugin_unload(&track->effects[e].plugin);
	}
}

#pragma endregion

#pragma region Public API

void uph_resources_unload_all_plugins(void)
{
	uph_resources_unload_plugins_recursive(uph_state.project.tracks);
}

void uph_resources_link_tracks(Naui_List(Uph_Track) tracks)
{
	uph_resources_link_track_list(tracks, NULL);
}

void uph_resources_add_track(Naui_String name, int32_t color_index)
{
	Uph_Track track = {
		.name = name,
		.color_index = color_index,
		.volume = 1.0f,
		.index = naui_list_len(uph_state.project.tracks)
	};
	naui_list_push(uph_state.project.tracks, track);
}

void uph_resources_add_automation_track(Uph_Track *parent, Naui_String name, Uph_PluginParam *param)
{
	Uph_Track track = {
		.name = name,
		.type = UPH_RESOURCE_AUTOMATION,
		.color_index = 0,
		.index = naui_list_len(parent->subtracks),
		.parent = parent,
		.automation_param = param
	};
	naui_list_push(parent->subtracks, track);
}

void uph_resources_init_master_track(void)
{
	uph_state.project.master_track = (Uph_MasterTrack){
		.volume = 1.0f
	};
}

static void uph_resources_toggle_track_state(Uph_Track *track, Uph_TrackState flag, const char *action)
{
	Uph_ActionTrackState data = {
		.track = uph_action_track_ref(track),
		.old_state = track->state,
		.new_state = track->state ^ flag
	};
	naui_action_execute_stack(action, data);
}

static void uph_resources_toggle_master_state(Uph_TrackState flag, const char *action)
{
	Uph_ActionTrackState data = {
		.track = uph_action_track_ref_master(),
		.old_state = uph_state.project.master_track.state,
		.new_state = uph_state.project.master_track.state ^ flag
	};
	naui_action_execute_stack(action, data);
}

bool uph_plugin_owns_param(const Uph_Plugin *plugin, const Uph_PluginParam *param)
{
	const uint64_t count = naui_list_len(plugin->params);
	return param && count > 0 && param >= plugin->params && param < plugin->params + count;
}

bool uph_resources_param_owner_index(const Uph_Track *track, const Uph_PluginParam *param, int32_t *out_effect_index)
{
	if (!param)
		return false;

	if (uph_plugin_owns_param(&track->instrument, param))
	{
		*out_effect_index = -1;
		return true;
	}

	const uint64_t effect_count = naui_list_len(track->effects);
	for (uint64_t e = 0; e < effect_count; e++)
	{
		if (uph_plugin_owns_param(&track->effects[e].plugin, param))
		{
			*out_effect_index = (int32_t)e;
			return true;
		}
	}

	return false;
}

Uph_PluginParam *uph_resources_find_param(Uph_Track *track, int32_t effect_index, uint64_t param_id)
{
	Uph_Plugin *plugin;
	if (effect_index < 0)
	{
		plugin = &track->instrument;
	}
	else
	{
		if ((uint64_t)effect_index >= (uint64_t)naui_list_len(track->effects))
			return NULL;

		plugin = &track->effects[effect_index].plugin;
	}

	const uint64_t param_count = naui_list_len(plugin->params);
	for (uint64_t i = 0; i < param_count; i++)
	{
		if (plugin->params[i].id == param_id)
			return &plugin->params[i];
	}

	return NULL;
}

static void uph_resources_clear_plugin_usage(Uph_Plugin *plugin)
{
	const uint64_t param_count = naui_list_len(plugin->params);
	for (uint64_t i = 0; i < param_count; i++)
		plugin->params[i].used = false;
}

void uph_resources_refresh_param_usage(Naui_List(Uph_Track) tracks)
{
	const uint64_t track_count = naui_list_len(tracks);
	for (uint64_t t = 0; t < track_count; t++)
	{
		Uph_Track *track = &tracks[t];

		uph_resources_clear_plugin_usage(&track->instrument);
		const uint64_t effect_count = naui_list_len(track->effects);
		for (uint64_t e = 0; e < effect_count; e++)
			uph_resources_clear_plugin_usage(&track->effects[e].plugin);

		const uint64_t lane_count = naui_list_len(track->subtracks);
		for (uint64_t s = 0; s < lane_count; s++)
		{
			Uph_Track *lane = &track->subtracks[s];
			if (lane->type != UPH_RESOURCE_AUTOMATION)
				continue;

			int32_t effect_index;
			if (uph_resources_param_owner_index(track, lane->automation_param, &effect_index))
				lane->automation_param->used = true;
		}

		if (track->type == UPH_RESOURCE_GROUP)
			uph_resources_refresh_param_usage(track->subtracks);
	}
}

static bool uph_resources_subtree_contains(const Uph_Track *root, const Uph_Track *needle)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(root->subtracks); i++)
	{
		if (&root->subtracks[i] == needle || uph_resources_subtree_contains(&root->subtracks[i], needle))
			return true;
	}
	return false;
}

void uph_resources_remove_track(Uph_Track *track)
{
	Naui_List(Uph_Track) list = track->parent ? track->parent->subtracks : uph_state.project.tracks;
	uint32_t removed_index = track->index;

	if (uph_state.shared.current_mixer_track == track)
		uph_state.shared.current_mixer_track = NULL;
	naui_list_clear(uph_state.shared.selected_mixer_tracks);
	if (uph_state.project.soloed_track == track || uph_resources_subtree_contains(track, uph_state.project.soloed_track))
		uph_state.project.soloed_track = NULL;

	uph_plugin_unload(&track->instrument);
	for (uint32_t e = 0; e < (uint32_t)naui_list_len(track->effects); e++)
		uph_plugin_unload(&track->effects[e].plugin);

	uph_resources_remove_track_children(track);
	naui_list_free(track->blocks);
	naui_list_free(track->effects);
	naui_list_remove(list, removed_index);

	for (uint32_t i = removed_index; i < (uint32_t)naui_list_len(list); i++)
		list[i].index--;
}

void uph_resources_clear_tracks(void)
{
	uph_resources_clear_tracks_recursive(uph_state.project.tracks);
	uph_state.project.tracks = NULL;
	uph_state.project.soloed_track = NULL;
	uph_state.shared.current_mixer_track = NULL;
	naui_list_clear(uph_state.shared.selected_mixer_tracks);
}

#pragma region Mixer Selection

bool uph_resources_mixer_track_selected(const Uph_Track *track)
{
	const uint32_t count = (uint32_t)naui_list_len(uph_state.shared.selected_mixer_tracks);
	for (uint32_t i = 0; i < count; i++)
	{
		if (uph_state.shared.selected_mixer_tracks[i] == track)
			return true;
	}
	return false;
}

void uph_resources_clear_mixer_selection(void)
{
	naui_list_clear(uph_state.shared.selected_mixer_tracks);
	uph_state.shared.current_mixer_track = NULL;
	uph_state.shared.selected_mixer_master = false;
}

void uph_resources_select_mixer_track(Uph_Track *track)
{
	naui_list_clear(uph_state.shared.selected_mixer_tracks);
	uph_state.shared.selected_mixer_master = false;
	uph_state.shared.current_mixer_track = track;

	if (track)
		naui_list_push(uph_state.shared.selected_mixer_tracks, track);
}

void uph_resources_select_mixer_master(void)
{
	naui_list_clear(uph_state.shared.selected_mixer_tracks);
	uph_state.shared.current_mixer_track = NULL;
	uph_state.shared.selected_mixer_master = true;
}

void uph_resources_toggle_mixer_track(Uph_Track *track)
{
	if (!track)
		return;

	uph_state.shared.selected_mixer_master = false;

	Naui_List(Uph_TrackPtr) selected = uph_state.shared.selected_mixer_tracks;
	const uint32_t count = (uint32_t)naui_list_len(selected);
	for (uint32_t i = 0; i < count; i++)
	{
		if (selected[i] != track)
			continue;

		naui_list_remove(uph_state.shared.selected_mixer_tracks, i);

		if (uph_state.shared.current_mixer_track == track)
		{
			const uint32_t remaining = (uint32_t)naui_list_len(uph_state.shared.selected_mixer_tracks);
			uph_state.shared.current_mixer_track = remaining > 0 ? uph_state.shared.selected_mixer_tracks[remaining - 1] : NULL;
		}
		return;
	}

	naui_list_push(uph_state.shared.selected_mixer_tracks, track);
	uph_state.shared.current_mixer_track = track;
}

void uph_resources_select_mixer_track_range(Uph_Track *to)
{
	if (!to)
		return;

	Uph_Track *from = uph_state.shared.selected_mixer_master ? NULL : uph_state.shared.current_mixer_track;
	if (!from || from->parent != to->parent)
	{
		uph_resources_select_mixer_track(to);
		return;
	}

	Naui_List(Uph_Track) list = to->parent ? to->parent->subtracks : uph_state.project.tracks;
	const uint32_t low = from->index < to->index ? from->index : to->index;
	const uint32_t high = from->index < to->index ? to->index : from->index;

	naui_list_clear(uph_state.shared.selected_mixer_tracks);
	uph_state.shared.selected_mixer_master = false;
	for (uint32_t i = low; i <= high && i < (uint32_t)naui_list_len(list); i++)
		naui_list_push(uph_state.shared.selected_mixer_tracks, &list[i]);
}

#pragma endregion

#pragma region Track Groups

#define UPH_TRACK_POINTER_SLOT_COUNT 4

typedef struct
{
	Uph_Track **slot;
	Uph_ActionTrackRef ref;
}
Uph_TrackPointerSlot;

static void uph_resources_capture_pointer_slots(Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT])
{
	slots[0].slot = &uph_state.shared.current_mixer_track;
	slots[1].slot = &uph_state.shared.current_plugin_list_track;
	slots[2].slot = &uph_state.shared.current_automation_list_track;
	slots[3].slot = &uph_state.project.soloed_track;

	for (uint32_t i = 0; i < UPH_TRACK_POINTER_SLOT_COUNT; i++)
		slots[i].ref = uph_action_track_ref(*slots[i].slot);
}

static void uph_resources_restructure_finished(void)
{
	uph_resources_link_tracks(uph_state.project.tracks);
	uph_resources_refresh_param_usage(uph_state.project.tracks);
	uph_song_timeline_invalidate_tracks();
	naui_action_clear_history();
}

typedef struct
{
	const uint32_t *indices;
	uint32_t count;
	uint32_t first;
}
Uph_GroupRemap;

static Uph_ActionTrackRef uph_resources_group_remap(Uph_ActionTrackRef ref, const Uph_GroupRemap *map)
{
	const Uph_ActionTrackRef invalid = { UPH_ACTION_TRACK_INVALID, 0 };
	if (ref.parent == UPH_ACTION_TRACK_INVALID)
		return ref;

	const bool top_level = ref.parent == UPH_ACTION_TRACK_TOP;
	const uint32_t old_top = top_level ? ref.index : (uint32_t)ref.parent;

	uint32_t grouped_before = 0;
	for (uint32_t k = 0; k < map->count; k++)
	{
		if (map->indices[k] < old_top)
		{
			grouped_before++;
		}
		else if (map->indices[k] == old_top)
		{
			if (!top_level)
				return invalid;
			return (Uph_ActionTrackRef){ (int32_t)map->first, k };
		}
	}

	const uint32_t new_top = old_top - grouped_before + (old_top > map->first ? 1 : 0);
	if (top_level)
		ref.index = new_top;
	else
		ref.parent = (int32_t)new_top;
	return ref;
}

typedef struct
{
	uint32_t group_index;
	uint32_t child_count;
}
Uph_UngroupRemap;

static Uph_ActionTrackRef uph_resources_ungroup_remap(Uph_ActionTrackRef ref, const Uph_UngroupRemap *map)
{
	const Uph_ActionTrackRef invalid = { UPH_ACTION_TRACK_INVALID, 0 };
	if (ref.parent == UPH_ACTION_TRACK_INVALID)
		return ref;

	if (ref.parent == UPH_ACTION_TRACK_TOP)
	{
		if (ref.index == map->group_index)
			return invalid;
		if (ref.index > map->group_index)
			ref.index += map->child_count - 1;
		return ref;
	}

	const uint32_t parent = (uint32_t)ref.parent;
	if (parent == map->group_index)
		return (Uph_ActionTrackRef){ UPH_ACTION_TRACK_TOP, map->group_index + ref.index };
	if (parent > map->group_index)
		ref.parent = (int32_t)(parent + map->child_count - 1);
	return ref;
}

bool uph_resources_can_group_track(const Uph_Track *track)
{
	if (!track)
		return false;
	if (track->type == UPH_RESOURCE_GROUP || track->type == UPH_RESOURCE_AUTOMATION)
		return false;
	if (naui_list_len(track->subtracks) > 0)
		return false;

	const Uph_ActionTrackRef ref = uph_action_track_ref(track);
	return ref.parent == UPH_ACTION_TRACK_TOP;
}

static bool uph_resources_selection_shares_parent(Naui_List(Uph_TrackPtr) tracks)
{
	const uint32_t count = (uint32_t)naui_list_len(tracks);
	if (count == 0 || !tracks[0])
		return false;

	for (uint32_t i = 1; i < count; i++)
	{
		if (!tracks[i] || tracks[i]->parent != tracks[0]->parent)
			return false;
	}

	return true;
}

static uint32_t *uph_resources_collect_group_indices(Naui_List(Uph_TrackPtr) tracks, uint32_t *out_count)
{
	*out_count = 0;

	const uint32_t top_count = (uint32_t)naui_list_len(uph_state.project.tracks);
	const uint32_t selected_count = (uint32_t)naui_list_len(tracks);
	if (top_count == 0 || selected_count == 0)
		return NULL;

	if (!uph_resources_selection_shares_parent(tracks))
		return NULL;

	bool *marked = calloc(top_count, sizeof(bool));
	uint32_t *indices = malloc(sizeof(uint32_t) * top_count);
	if (!marked || !indices)
	{
		free(marked);
		free(indices);
		return NULL;
	}

	for (uint32_t i = 0; i < selected_count; i++)
	{
		if (!uph_resources_can_group_track(tracks[i]))
			continue;

		marked[uph_action_track_ref(tracks[i]).index] = true;
	}

	uint32_t count = 0;
	for (uint32_t i = 0; i < top_count; i++)
	{
		if (marked[i])
			indices[count++] = i;
	}
	free(marked);

	if (count == 0)
	{
		free(indices);
		return NULL;
	}

	*out_count = count;
	return indices;
}

uint32_t uph_resources_count_groupable_tracks(Naui_List(Uph_TrackPtr) tracks)
{
	uint32_t count;
	uint32_t *indices = uph_resources_collect_group_indices(tracks, &count);
	free(indices);
	return count;
}

Uph_Track *uph_resources_group_tracks(Naui_List(Uph_TrackPtr) tracks, const char *name, int32_t color_index)
{
	uint32_t count;
	uint32_t *indices = uph_resources_collect_group_indices(tracks, &count);
	if (!indices)
		return NULL;

	const uint32_t first = indices[0];
	const bool was_running = uph_audio_engine_device_running();
	if (was_running)
		uph_audio_engine_stop();

	Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT];
	uph_resources_capture_pointer_slots(slots);

	Uph_Track group = {
		.name = naui_string_from_cstr(name),
		.type = UPH_RESOURCE_GROUP,
		.color_index = color_index,
		.volume = 1.0f
	};

	for (uint32_t k = 0; k < count; k++)
		naui_list_push(group.subtracks, uph_state.project.tracks[indices[k]]);
	for (uint32_t k = count; k-- > 0;)
		naui_list_remove(uph_state.project.tracks, indices[k]);

	naui_list_insert(uph_state.project.tracks, group, first);

	uph_resources_restructure_finished();

	const Uph_GroupRemap map = { .indices = indices, .count = count, .first = first };
	for (uint32_t i = 0; i < UPH_TRACK_POINTER_SLOT_COUNT; i++)
		*slots[i].slot = uph_action_track_resolve(uph_resources_group_remap(slots[i].ref, &map));

	free(indices);

	Uph_Track *result = &uph_state.project.tracks[first];
	uph_resources_select_mixer_track(result);

	if (was_running)
		uph_audio_engine_start();

	return result;
}

bool uph_resources_ungroup_track(Uph_Track *group)
{
	if (!group || group->type != UPH_RESOURCE_GROUP)
		return false;

	const Uph_ActionTrackRef group_ref = uph_action_track_ref(group);
	if (group_ref.parent != UPH_ACTION_TRACK_TOP)
		return false;

	const uint32_t child_count = (uint32_t)naui_list_len(group->subtracks);
	const bool was_running = uph_audio_engine_device_running();
	if (was_running)
		uph_audio_engine_stop();

	Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT];
	uph_resources_capture_pointer_slots(slots);

	Uph_Track *children = child_count > 0 ? malloc(sizeof(Uph_Track) * child_count) : NULL;
	for (uint32_t i = 0; i < child_count; i++)
		children[i] = group->subtracks[i];
	naui_list_free(group->subtracks);

	const uint32_t group_index = group_ref.index;
	uph_resources_remove_track(group);
	for (uint32_t i = 0; i < child_count; i++)
		naui_list_insert(uph_state.project.tracks, children[i], group_index + i);
	free(children);

	uph_resources_restructure_finished();

	const Uph_UngroupRemap map = { .group_index = group_index, .child_count = child_count };
	for (uint32_t i = 0; i < UPH_TRACK_POINTER_SLOT_COUNT; i++)
		*slots[i].slot = uph_action_track_resolve(uph_resources_ungroup_remap(slots[i].ref, &map));

	uph_resources_clear_mixer_selection();
	for (uint32_t i = 0; i < child_count; i++)
		uph_resources_toggle_mixer_track(&uph_state.project.tracks[group_index + i]);

	if (was_running)
		uph_audio_engine_start();

	return true;
}

static Uph_ActionTrackRef uph_resources_ref_after_remove(Uph_ActionTrackRef ref, int32_t list_parent, uint32_t index)
{
	if (ref.parent == UPH_ACTION_TRACK_INVALID)
		return ref;

	if (ref.parent == list_parent)
	{
		if (ref.index > index)
			ref.index--;
	}
	else if (list_parent == UPH_ACTION_TRACK_TOP && ref.parent >= 0 && (uint32_t)ref.parent > index)
	{
		ref.parent--;
	}
	return ref;
}

static Uph_ActionTrackRef uph_resources_ref_after_insert(Uph_ActionTrackRef ref, int32_t list_parent, uint32_t index)
{
	if (ref.parent == UPH_ACTION_TRACK_INVALID)
		return ref;

	if (ref.parent == list_parent)
	{
		if (ref.index >= index)
			ref.index++;
	}
	else if (list_parent == UPH_ACTION_TRACK_TOP && ref.parent >= 0 && (uint32_t)ref.parent >= index)
	{
		ref.parent++;
	}
	return ref;
}

bool uph_resources_move_track(Uph_Track *track, Uph_Track *dest_group, uint32_t dest_index)
{
	if (!track || track->type == UPH_RESOURCE_AUTOMATION)
		return false;

	const bool moving_group = track->type == UPH_RESOURCE_GROUP;
	if (moving_group && dest_group)
		return false;
	if (!moving_group && naui_list_len(track->subtracks) > 0)
		return false;

	const Uph_ActionTrackRef src = uph_action_track_ref(track);
	if (src.parent == UPH_ACTION_TRACK_INVALID)
		return false;
	if (moving_group && src.parent != UPH_ACTION_TRACK_TOP)
		return false;

	int32_t dest_parent = UPH_ACTION_TRACK_TOP;
	if (dest_group)
	{
		const Uph_ActionTrackRef group_ref = uph_action_track_ref(dest_group);
		if (dest_group->type != UPH_RESOURCE_GROUP || group_ref.parent != UPH_ACTION_TRACK_TOP)
			return false;
		dest_parent = (int32_t)group_ref.index;
	}

	Naui_List(Uph_Track) *dest_list = uph_action_track_list_ptr(dest_parent);
	if (!dest_list)
		return false;

	const uint32_t dest_len = (uint32_t)naui_list_len(*dest_list);
	if (dest_index > dest_len)
		dest_index = dest_len;

	if (src.parent == dest_parent && (dest_index == src.index || dest_index == src.index + 1))
		return false;

	const bool was_running = uph_audio_engine_device_running();
	if (was_running)
		uph_audio_engine_stop();

	Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT];
	uph_resources_capture_pointer_slots(slots);

	Uph_Track moved = *track;
	Naui_List(Uph_Track) *src_list = uph_action_track_list_ptr(src.parent);
	naui_list_remove(*src_list, src.index);

	int32_t new_parent = dest_parent;
	uint32_t new_index = dest_index;
	if (src.parent == dest_parent && src.index < dest_index)
		new_index--;
	if (src.parent == UPH_ACTION_TRACK_TOP && dest_parent != UPH_ACTION_TRACK_TOP && (uint32_t)dest_parent > src.index)
		new_parent--;

	Naui_List(Uph_Track) *target_list = uph_action_track_list_ptr(new_parent);
	naui_list_insert(*target_list, moved, new_index);

	uph_resources_restructure_finished();

	for (uint32_t i = 0; i < UPH_TRACK_POINTER_SLOT_COUNT; i++)
	{
		Uph_ActionTrackRef ref = slots[i].ref;
		if (ref.parent == src.parent && ref.index == src.index)
			ref = (Uph_ActionTrackRef){ new_parent, new_index };
		else if (moving_group && ref.parent == (int32_t)src.index)
			ref.parent = new_parent;
		else
			ref = uph_resources_ref_after_insert(uph_resources_ref_after_remove(ref, src.parent, src.index), new_parent, new_index);

		*slots[i].slot = uph_action_track_resolve(ref);
	}

	uph_resources_select_mixer_track(uph_action_track_resolve((Uph_ActionTrackRef){ new_parent, new_index }));

	if (was_running)
		uph_audio_engine_start();

	return true;
}

#pragma endregion

Uph_ResourceIndex uph_resources_add_sample_data(Uph_SampleData data, Naui_String name)
{
	data.ref_count = 1;

	Uph_Sample sample = {
		.data_index = naui_list_len(uph_state.project.sample_data),
		.time_scale = 1.0f,
		.name = name
	};

	Uph_ResourceIndex sample_index = naui_list_len(uph_state.project.samples);

	naui_list_push(uph_state.project.sample_data, data);
	naui_list_push(uph_state.project.samples, sample);
	return sample_index;
}

bool uph_resources_add_sample_from_file(Naui_Path path)
{
	Uph_SampleData data = uph_audio_engine_load_sample_data(path);
	if (!uph_audio_engine_sample_data_valid(&data))
		return false;

	uph_resources_add_sample_data(data, naui_view_to_string(naui_file_stem(&path)));
	return true;
}

void uph_resources_copy_sample(Uph_ResourceIndex sample_index)
{
	Uph_Sample sample = uph_state.project.samples[sample_index];
	uph_state.project.sample_data[sample.data_index].ref_count++;
	naui_list_push(uph_state.project.samples, sample);
}

void uph_resources_remove_sample(Uph_ResourceIndex sample_index)
{
	Uph_Sample sample = uph_state.project.samples[sample_index];

	uph_resources_clear_timeline_blocks_with_resource(UPH_RESOURCE_SAMPLE, sample_index);

	if (--uph_state.project.sample_data[sample.data_index].ref_count == 0)
	{
		naui_list_remove(uph_state.project.sample_data, sample.data_index);

		for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.samples); i++)
		{
			if (uph_state.project.samples[i].data_index > sample.data_index)
				uph_state.project.samples[i].data_index--;
		}
	}

	naui_list_remove(uph_state.project.samples, sample_index);

	if (uph_state.shared.selected_resource.index == sample_index)
		uph_state.shared.selected_resource = (Uph_SelectedResourceState){0};
}

void uph_resources_add_pattern(void)
{
	Uph_MidiPattern pattern = {
		.name = naui_string_from_cstr(NAUI_TR("pattern.default.name"))
	};

	naui_list_push(uph_state.project.midi_patterns, pattern);
}

void uph_resources_copy_pattern(Uph_ResourceIndex pattern_index)
{
	Uph_MidiPattern pattern = uph_state.project.midi_patterns[pattern_index];
	pattern.notes = naui_list_clone(pattern.notes);
	naui_list_push(uph_state.project.midi_patterns, pattern);
}

void uph_resources_remove_pattern(Uph_ResourceIndex pattern_index)
{
	uph_resources_clear_timeline_blocks_with_resource(UPH_RESOURCE_PATTERN, pattern_index);
	naui_list_remove(uph_state.project.midi_patterns, pattern_index);
	if (uph_state.shared.selected_resource.index == pattern_index)
		uph_state.shared.selected_resource = (Uph_SelectedResourceState){0};
}

void uph_resources_add_automation(void)
{
	Uph_Automation automation = {
		.name = naui_string_from_cstr(NAUI_TR("automation.default.name"))
	};

	const Uph_AutomationPoint point1 = (Uph_AutomationPoint){.beat = 0.0, .value = 0.5f};
	const Uph_AutomationPoint point2 = (Uph_AutomationPoint){.beat = (double)uph_state.project.time_signature.denominator, .value = 0.5f};

	naui_list_push(automation.points, point1);
	naui_list_push(automation.points, point2);

	naui_list_push(uph_state.project.automations, automation);
}

void uph_resources_copy_automation(Uph_ResourceIndex automation_index)
{
	Uph_Automation automation = uph_state.project.automations[automation_index];
	automation.points = naui_list_clone(automation.points);
	naui_list_push(uph_state.project.automations, automation);
}

void uph_resources_remove_automation(Uph_ResourceIndex automation_index)
{
	uph_resources_clear_timeline_blocks_with_resource(UPH_RESOURCE_AUTOMATION, automation_index);
	naui_list_remove(uph_state.project.automations, automation_index);
	if (uph_state.shared.selected_resource.index == automation_index)
		uph_state.shared.selected_resource = (Uph_SelectedResourceState){0};
}

void uph_resources_remove_all_automation(void)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.automations); i++)
		naui_list_free(uph_state.project.automations[i].points);

	naui_list_clear(uph_state.project.automations);
}

#pragma endregion