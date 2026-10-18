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
	uph_song_timeline_invalidate_tracks();
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

static void uph_resources_collapse_mixer_selection(void)
{
	naui_list_clear(uph_state.shared.selected_mixer_tracks);
	if (uph_state.shared.current_mixer_track)
		naui_list_push(uph_state.shared.selected_mixer_tracks, uph_state.shared.current_mixer_track);
}

#pragma endregion

static int uph_resources_delete_order(const void *a, const void *b)
{
	return uph_action_track_ref_compare(*(const Uph_ActionTrackRef *)b, *(const Uph_ActionTrackRef *)a);
}

void uph_resources_delete_tracks(Naui_List(Uph_TrackPtr) tracks)
{
	const uint32_t count = (uint32_t)naui_list_len(tracks);
	if (count == 0)
		return;

	Uph_ActionTrackRef *refs = malloc(sizeof(Uph_ActionTrackRef) * count);
	if (!refs)
		return;

	uint32_t ref_count = 0;
	for (uint32_t i = 0; i < count; i++)
	{
		const Uph_ActionTrackRef ref = uph_action_track_ref(tracks[i]);
		if (uph_action_track_ref_valid(ref))
			refs[ref_count++] = ref;
	}

	qsort(refs, ref_count, sizeof(Uph_ActionTrackRef), uph_resources_delete_order);

	const bool grouped = ref_count > 1;
	if (grouped)
		naui_action_group_start(UPH_ACTION_TRACKS_DELETE_GROUP);

	for (uint32_t i = 0; i < ref_count; i++)
	{
		Uph_ActionTrackDelete data = { .track = refs[i] };
		naui_action_execute_stack(UPH_ACTION_TRACK_DELETE, data);
	}

	if (grouped)
		naui_action_group_end();

	free(refs);
}

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

static Uph_ActionTrackRef *uph_resources_slot_refs(const Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT], const Uph_ActionTrackRef *extra, uint32_t extra_count, uint32_t reserve)
{
	Uph_ActionTrackRef *refs = malloc(sizeof(Uph_ActionTrackRef) * (UPH_TRACK_POINTER_SLOT_COUNT + extra_count + reserve));
	if (!refs)
		return NULL;

	for (uint32_t i = 0; i < UPH_TRACK_POINTER_SLOT_COUNT; i++)
		refs[i] = slots[i].ref;
	for (uint32_t i = 0; i < extra_count; i++)
		refs[UPH_TRACK_POINTER_SLOT_COUNT + i] = extra[i];
	return refs;
}

static void uph_resources_restore_pointer_slots(const Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT], const Uph_ActionTrackRef *refs)
{
	for (uint32_t i = 0; i < UPH_TRACK_POINTER_SLOT_COUNT; i++)
		*slots[i].slot = uph_action_track_resolve(refs[i]);
}

static void uph_resources_restructure_finished(void)
{
	uph_resources_link_tracks(uph_state.project.tracks);
	uph_resources_refresh_param_usage(uph_state.project.tracks);
	uph_song_timeline_invalidate_tracks();
	naui_action_clear_history();
}

static uint32_t uph_resources_track_height(const Uph_Track *track)
{
	uint32_t height = 0;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		const uint32_t child_height = uph_resources_track_height(&track->subtracks[i]);
		if (child_height > height)
			height = child_height;
	}
	return height + 1;
}

static int uph_resources_ref_order(const void *a, const void *b)
{
	return uph_action_track_ref_compare(*(const Uph_ActionTrackRef *)a, *(const Uph_ActionTrackRef *)b);
}

static uint32_t uph_resources_sort_outermost_refs(Uph_ActionTrackRef *refs, uint32_t count)
{
	qsort(refs, count, sizeof(Uph_ActionTrackRef), uph_resources_ref_order);

	uint32_t kept = 0;
	for (uint32_t i = 0; i < count; i++)
	{
		if (kept > 0 && uph_action_track_ref_has_prefix(refs[i], refs[kept - 1]))
			continue;

		refs[kept++] = refs[i];
	}
	return kept;
}

static Uph_ActionTrackRef uph_resources_move_subtree(Uph_ActionTrackRef src, Uph_ActionTrackRef dest_owner, uint32_t dest_index, Uph_ActionTrackRef *refs, uint32_t ref_count)
{
	const Uph_ActionTrackRef src_owner = uph_action_track_ref_owner(src);
	const uint32_t src_index = uph_action_track_ref_index(src);

	Naui_List(Uph_Track) *src_list = uph_action_track_list_ptr(src_owner);
	const Uph_Track moved = (*src_list)[src_index];
	naui_list_remove(*src_list, src_index);

	dest_owner = uph_action_track_ref_after_remove(dest_owner, src);
	if (uph_action_track_ref_equal(dest_owner, src_owner) && src_index < dest_index)
		dest_index--;

	Naui_List(Uph_Track) *dest_list = uph_action_track_list_ptr(dest_owner);
	naui_list_insert(*dest_list, moved, dest_index);

	const Uph_ActionTrackRef dst = uph_action_track_ref_child(dest_owner, dest_index);
	for (uint32_t i = 0; i < ref_count; i++)
		refs[i] = uph_action_track_ref_after_move(refs[i], src, dst);
	return dst;
}

bool uph_resources_can_group_track(const Uph_Track *track)
{
	if (!track || track->type == UPH_RESOURCE_AUTOMATION)
		return false;

	const Uph_ActionTrackRef ref = uph_action_track_ref(track);
	return ref.depth > 0 && ref.depth + uph_resources_track_height(track) <= UPH_ACTION_TRACK_MAX_DEPTH;
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

static Uph_ActionTrackRef *uph_resources_collect_group_refs(Naui_List(Uph_TrackPtr) tracks, uint32_t *out_count)
{
	*out_count = 0;

	const uint32_t selected_count = (uint32_t)naui_list_len(tracks);
	if (selected_count == 0 || !uph_resources_selection_shares_parent(tracks))
		return NULL;

	Uph_ActionTrackRef *refs = malloc(sizeof(Uph_ActionTrackRef) * selected_count);
	if (!refs)
		return NULL;

	uint32_t count = 0;
	for (uint32_t i = 0; i < selected_count; i++)
	{
		if (uph_resources_can_group_track(tracks[i]))
			refs[count++] = uph_action_track_ref(tracks[i]);
	}

	count = uph_resources_sort_outermost_refs(refs, count);
	if (count == 0)
	{
		free(refs);
		return NULL;
	}

	*out_count = count;
	return refs;
}

uint32_t uph_resources_count_groupable_tracks(Naui_List(Uph_TrackPtr) tracks)
{
	uint32_t count;
	free(uph_resources_collect_group_refs(tracks, &count));
	return count;
}

Uph_Track *uph_resources_group_tracks(Naui_List(Uph_TrackPtr) tracks, const char *name, int32_t color_index)
{
	uint32_t count;
	Uph_ActionTrackRef *grouped = uph_resources_collect_group_refs(tracks, &count);
	if (!grouped)
		return NULL;

	Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT];
	uph_resources_capture_pointer_slots(slots);

	const uint32_t group_slot = UPH_TRACK_POINTER_SLOT_COUNT + count;
	const uint32_t ref_count = group_slot + 1;
	Uph_ActionTrackRef *refs = uph_resources_slot_refs(slots, grouped, count, 1);
	const Uph_ActionTrackRef group_ref = grouped[0];
	free(grouped);
	if (!refs)
		return NULL;

	const bool was_running = uph_audio_engine_device_running();
	if (was_running)
		uph_audio_engine_stop();

	Uph_Track group = {
		.name = naui_string_from_cstr(name),
		.type = UPH_RESOURCE_GROUP,
		.color_index = color_index,
		.volume = 1.0f
	};

	Naui_List(Uph_Track) *owner_list = uph_action_track_list_ptr(uph_action_track_ref_owner(group_ref));
	naui_list_insert(*owner_list, group, uph_action_track_ref_index(group_ref));

	for (uint32_t i = 0; i < group_slot; i++)
		refs[i] = uph_action_track_ref_after_insert(refs[i], group_ref);
	refs[group_slot] = group_ref;

	for (uint32_t k = 0; k < count; k++)
		uph_resources_move_subtree(refs[UPH_TRACK_POINTER_SLOT_COUNT + k], refs[group_slot], k, refs, ref_count);

	uph_resources_restructure_finished();
	uph_resources_restore_pointer_slots(slots, refs);

	uph_resources_collapse_mixer_selection();

	Uph_Track *result = uph_action_track_resolve(refs[group_slot]);
	free(refs);

	if (was_running)
		uph_audio_engine_start();

	return result;
}

bool uph_resources_ungroup_track(Uph_Track *group, Naui_List(Uph_TrackPtr) *out_children)
{
	if (!group || group->type != UPH_RESOURCE_GROUP)
		return false;

	const Uph_ActionTrackRef group_ref = uph_action_track_ref(group);
	if (group_ref.depth == 0)
		return false;

	const uint32_t child_count = (uint32_t)naui_list_len(group->subtracks);

	Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT];
	uph_resources_capture_pointer_slots(slots);

	const uint32_t group_slot = UPH_TRACK_POINTER_SLOT_COUNT;
	const uint32_t ref_count = group_slot + 1;
	Uph_ActionTrackRef *refs = uph_resources_slot_refs(slots, &group_ref, 1, 0);
	if (!refs)
		return false;

	const bool was_running = uph_audio_engine_device_running();
	if (was_running)
		uph_audio_engine_stop();

	const Uph_ActionTrackRef owner = uph_action_track_ref_owner(group_ref);
	const uint32_t group_index = uph_action_track_ref_index(group_ref);
	for (uint32_t i = 0; i < child_count; i++)
		uph_resources_move_subtree(uph_action_track_ref_child(refs[group_slot], 0), owner, group_index + 1 + i, refs, ref_count);

	uph_resources_link_tracks(uph_state.project.tracks);
	uph_resources_remove_track(uph_action_track_resolve(refs[group_slot]));
	for (uint32_t i = 0; i < group_slot; i++)
		refs[i] = uph_action_track_ref_after_remove(refs[i], refs[group_slot]);

	uph_resources_restructure_finished();
	uph_resources_restore_pointer_slots(slots, refs);
	free(refs);

	uph_resources_collapse_mixer_selection();
	for (uint32_t i = 0; out_children && i < child_count; i++)
		naui_list_push(*out_children, uph_action_track_resolve(uph_action_track_ref_child(owner, group_index + i)));

	if (was_running)
		uph_audio_engine_start();

	return true;
}

static Uph_ActionTrackRef *uph_resources_collect_move_refs(Naui_List(Uph_TrackPtr) tracks, Uph_ActionTrackRef dest_owner, uint32_t *out_count)
{
	*out_count = 0;

	const uint32_t selected_count = (uint32_t)naui_list_len(tracks);
	if (selected_count == 0)
		return NULL;

	Uph_ActionTrackRef *refs = malloc(sizeof(Uph_ActionTrackRef) * selected_count);
	if (!refs)
		return NULL;

	uint32_t count = 0;
	for (uint32_t i = 0; i < selected_count; i++)
	{
		const Uph_Track *track = tracks[i];
		if (!track || track->type == UPH_RESOURCE_AUTOMATION)
			continue;

		const Uph_ActionTrackRef ref = uph_action_track_ref(track);
		if (ref.depth == 0 || uph_action_track_ref_has_prefix(dest_owner, ref))
			continue;

		if (dest_owner.depth + uph_resources_track_height(track) > UPH_ACTION_TRACK_MAX_DEPTH)
			continue;

		refs[count++] = ref;
	}

	count = uph_resources_sort_outermost_refs(refs, count);
	if (count == 0)
	{
		free(refs);
		return NULL;
	}

	*out_count = count;
	return refs;
}

uint32_t uph_resources_count_movable_tracks(Naui_List(Uph_TrackPtr) tracks)
{
	uint32_t count;
	free(uph_resources_collect_move_refs(tracks, uph_action_track_ref_root(), &count));
	return count;
}

bool uph_resources_move_tracks(Naui_List(Uph_TrackPtr) tracks, Uph_Track *dest_group, uint32_t dest_index, Naui_List(Uph_TrackPtr) *out_moved)
{
	Uph_ActionTrackRef dest_owner = uph_action_track_ref_root();
	if (dest_group)
	{
		if (dest_group->type != UPH_RESOURCE_GROUP)
			return false;

		dest_owner = uph_action_track_ref(dest_group);
		if (dest_owner.depth == 0)
			return false;
	}

	Naui_List(Uph_Track) *dest_list = uph_action_track_list_ptr(dest_owner);
	if (!dest_list)
		return false;

	uint32_t count;
	Uph_ActionTrackRef *moving = uph_resources_collect_move_refs(tracks, dest_owner, &count);
	if (!moving)
		return false;

	const uint32_t dest_len = (uint32_t)naui_list_len(*dest_list);
	if (dest_index > dest_len)
		dest_index = dest_len;

	const uint32_t first_index = uph_action_track_ref_index(moving[0]);
	bool in_place = dest_index >= first_index && dest_index <= first_index + count;
	for (uint32_t i = 0; in_place && i < count; i++)
	{
		in_place =
			uph_action_track_ref_equal(uph_action_track_ref_owner(moving[i]), dest_owner) &&
			uph_action_track_ref_index(moving[i]) == first_index + i;
	}

	if (in_place)
	{
		free(moving);
		return false;
	}

	Uph_TrackPointerSlot slots[UPH_TRACK_POINTER_SLOT_COUNT];
	uph_resources_capture_pointer_slots(slots);

	const uint32_t ref_count = UPH_TRACK_POINTER_SLOT_COUNT + count;
	Uph_ActionTrackRef *refs = uph_resources_slot_refs(slots, moving, count, 0);
	free(moving);
	if (!refs)
		return false;

	const bool was_running = uph_audio_engine_device_running();
	if (was_running)
		uph_audio_engine_stop();

	Uph_ActionTrackRef cursor_owner = dest_owner;
	uint32_t cursor_index = dest_index;
	for (uint32_t i = 0; i < count; i++)
	{
		const Uph_ActionTrackRef placed = uph_resources_move_subtree(refs[UPH_TRACK_POINTER_SLOT_COUNT + i], cursor_owner, cursor_index, refs, ref_count);
		cursor_owner = uph_action_track_ref_owner(placed);
		cursor_index = uph_action_track_ref_index(placed) + 1;
	}

	uph_resources_restructure_finished();
	uph_resources_restore_pointer_slots(slots, refs);

	uph_resources_collapse_mixer_selection();
	for (uint32_t i = 0; out_moved && i < count; i++)
		naui_list_push(*out_moved, uph_action_track_resolve(refs[UPH_TRACK_POINTER_SLOT_COUNT + i]));

	free(refs);

	if (was_running)
		uph_audio_engine_start();

	return true;
}

bool uph_resources_move_track(Uph_Track *track, Uph_Track *dest_group, uint32_t dest_index)
{
	Naui_List(Uph_TrackPtr) tracks = NULL;
	naui_list_push(tracks, track);
	const bool moved = uph_resources_move_tracks(tracks, dest_group, dest_index, NULL);
	naui_list_free(tracks);
	return moved;
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