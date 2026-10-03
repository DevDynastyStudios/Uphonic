typedef struct
{
	Uph_ActionTrackRef track;
	uint32_t block_index;
	Uph_TimelineBlock block;
	bool applied_live;
} Uph_ActionBlockCreate;

typedef struct
{
	Uph_ActionTrackRef track;
	uint32_t block_index;
	Uph_TimelineBlock block;
} Uph_ActionBlockDelete;

typedef struct
{
	Uph_ActionTrackRef src_track;
	uint32_t src_index;
	Uph_ActionTrackRef dst_track;
	uint32_t dst_index;
	double old_start, old_length, old_offset;
	double new_start, new_length, new_offset;
	bool applied_live;
} Uph_ActionBlockTransform;

typedef struct
{
	Uph_ActionTrackRef track;
	uint32_t index;
	Uph_TimelineBlock block;
} Uph_ActionBlockSnapshot;

#pragma region Block Helpers
static void uph_block_refresh_track_type(Uph_Track *track)
{
	if (track->type == UPH_RESOURCE_AUTOMATION)
		return;

	if (naui_list_len(track->blocks) == 0 && !track->instrument.loaded)
		track->type = UPH_RESOURCE_NONE;
}

static void uph_block_list_insert(Uph_Track *track, uint32_t index, Uph_TimelineBlock block)
{
	if (index > (uint32_t)naui_list_len(track->blocks))
		index = (uint32_t)naui_list_len(track->blocks);

	naui_list_insert(track->blocks, block, index);
	if (track->type == UPH_RESOURCE_NONE)
		track->type = block.type;
}

static bool uph_block_list_take(Uph_Track *track, uint32_t index, Uph_TimelineBlock *out)
{
	if (!track || index >= (uint32_t)naui_list_len(track->blocks))
		return false;

	*out = track->blocks[index];
	naui_list_remove(track->blocks, index);
	uph_block_refresh_track_type(track);
	return true;
}

static bool uph_block_relocate(Uph_ActionTrackRef from, uint32_t from_index, Uph_ActionTrackRef to, uint32_t to_index, double start, double length, double offset)
{
	Uph_Track* source = uph_action_track_resolve(from);
	Uph_Track* target = uph_action_track_resolve(to);
	if (!source || !target)
		return false;

	Uph_TimelineBlock block;
	if (!uph_block_list_take(source, from_index, &block))
		return false;

	block.start_beat = start;
	block.length_beats = length;
	block.start_offset_beats = offset;
	uph_block_list_insert(target, to_index, block);
	return true;
}

static void uph_block_collect(Uph_Track *track, Uph_ActionTrackRef ref, Uph_ResourceType type, bool only_selected, Naui_List(Uph_ActionBlockSnapshot) *out, bool match_resource, Uph_ResourceIndex resource_index)
{
	for (uint32_t b = 0; b < (uint32_t)naui_list_len(track->blocks); b++)
	{
		const Uph_TimelineBlock* block = &track->blocks[b];
		if (only_selected && !block->selected)
			continue;

		if (match_resource && (block->type != type || block->resource_index != resource_index))
			continue;

		Uph_ActionBlockSnapshot snapshot = { .track = ref, .index = b, .block = *block };
		naui_list_push(*out, snapshot);
	}
}

static void uph_block_collect_all(Uph_ResourceType type, bool only_selected, bool match_resource, Uph_ResourceIndex resource_index, Naui_List(Uph_ActionBlockSnapshot) *out)
{
	for (uint32_t t = 0; t < (uint32_t)naui_list_len(uph_state.project.tracks); t++)
	{
		Uph_Track* track = &uph_state.project.tracks[t];
		uph_block_collect(track, (Uph_ActionTrackRef){ -1, t }, type, only_selected, out, match_resource, resource_index);

		for (uint32_t s = 0; s < (uint32_t)naui_list_len(track->subtracks); s++)
			uph_block_collect(&track->subtracks[s], (Uph_ActionTrackRef){ (int32_t)t, s }, type, only_selected, out, match_resource, resource_index);
	}
}

static void uph_block_unlink_resource(Uph_ResourceType type, Uph_ResourceIndex resource_index, Naui_List(Uph_ActionBlockSnapshot) *removed)
{
	uph_block_collect_all(type, false, true, resource_index, removed);
	for (uint32_t i = (uint32_t)naui_list_len(*removed); i > 0; i--)
	{
		const Uph_ActionBlockSnapshot* snapshot = &(*removed)[i - 1];
		Uph_TimelineBlock discarded;
		uph_block_list_take(uph_action_track_resolve(snapshot->track), snapshot->index, &discarded);
	}

	for (uint32_t t = 0; t < (uint32_t)naui_list_len(uph_state.project.tracks); t++)
	{
		Uph_Track* track = &uph_state.project.tracks[t];
		for (uint32_t b = 0; b < (uint32_t)naui_list_len(track->blocks); b++)
		{
			if (track->blocks[b].type == type && track->blocks[b].resource_index > resource_index)
				track->blocks[b].resource_index--;
		}

		for (uint32_t s = 0; s < (uint32_t)naui_list_len(track->subtracks); s++)
		{
			Uph_Track* sub = &track->subtracks[s];
			for (uint32_t b = 0; b < (uint32_t)naui_list_len(sub->blocks); b++)
			{
				if (sub->blocks[b].type == type && sub->blocks[b].resource_index > resource_index)
					sub->blocks[b].resource_index--;
			}
		}
	}
}

static void uph_block_relink_resource(Uph_ResourceType type, Uph_ResourceIndex resource_index, const Naui_List(Uph_ActionBlockSnapshot) removed)
{
	for (uint32_t t = 0; t < (uint32_t)naui_list_len(uph_state.project.tracks); t++)
	{
		Uph_Track* track = &uph_state.project.tracks[t];
		for (uint32_t b = 0; b < (uint32_t)naui_list_len(track->blocks); b++)
		{
			if (track->blocks[b].type == type && track->blocks[b].resource_index >= resource_index)
				track->blocks[b].resource_index++;
		}

		for (uint32_t s = 0; s < (uint32_t)naui_list_len(track->subtracks); s++)
		{
			Uph_Track* sub = &track->subtracks[s];
			for (uint32_t b = 0; b < (uint32_t)naui_list_len(sub->blocks); b++)
			{
				if (sub->blocks[b].type == type && sub->blocks[b].resource_index >= resource_index)
					sub->blocks[b].resource_index++;
			}
		}
	}

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(removed); i++)
	{
		Uph_Track* track = uph_action_track_resolve(removed[i].track);
		if (track)
			uph_block_list_insert(track, removed[i].index, removed[i].block);
	}
}

#pragma endregion

#pragma region Block Selection

static uint32_t uph_block_selected_count(void)
{
	uint32_t count = 0;
	for (uint32_t t = 0; t < (uint32_t)naui_list_len(uph_state.project.tracks); t++)
	{
		const Uph_Track* track = &uph_state.project.tracks[t];
		for (uint32_t b = 0; b < (uint32_t)naui_list_len(track->blocks); b++)
			count += track->blocks[b].selected;

		for (uint32_t s = 0; s < (uint32_t)naui_list_len(track->subtracks); s++)
		{
			const Uph_Track* sub = &track->subtracks[s];
			for (uint32_t b = 0; b < (uint32_t)naui_list_len(sub->blocks); b++)
				count += sub->blocks[b].selected;
		}
	}

	return count;
}

static void uph_block_select_all(bool selected)
{
	for (uint32_t t = 0; t < (uint32_t)naui_list_len(uph_state.project.tracks); t++)
	{
		Uph_Track* track = &uph_state.project.tracks[t];
		for (uint32_t b = 0; b < (uint32_t)naui_list_len(track->blocks); b++)
			track->blocks[b].selected = selected;

		for (uint32_t s = 0; s < (uint32_t)naui_list_len(track->subtracks); s++)
		{
			Uph_Track* sub = &track->subtracks[s];
			for (uint32_t b = 0; b < (uint32_t)naui_list_len(sub->blocks); b++)
				sub->blocks[b].selected = selected;
		}
	}
}

#pragma endregion

#pragma region Block Create
bool _uph_action_block_create_execute(void* userdata)
{
	Uph_ActionBlockCreate* data = userdata;
	Uph_Track* track = uph_action_track_resolve(data->track);
	if (!track)
		return false;

	if (data->applied_live)
	{
		data->applied_live = false;
		return true;
	}

	uph_block_list_insert(track, data->block_index, data->block);
	return true;
}

bool _uph_action_block_create_undo(void* userdata)
{
	Uph_ActionBlockCreate* data = userdata;
	return uph_block_list_take(uph_action_track_resolve(data->track), data->block_index, &data->block);
}

bool _uph_action_block_create_redo(void* userdata)
{
	Uph_ActionBlockCreate* data = userdata;
	Uph_Track* track = uph_action_track_resolve(data->track);
	if (!track)
		return false;

	uph_block_list_insert(track, data->block_index, data->block);
	return true;
}

#pragma endregion

#pragma region Block Delete
bool _uph_action_block_delete_execute(void* userdata)
{
	Uph_ActionBlockDelete* data = userdata;
	return uph_block_list_take(uph_action_track_resolve(data->track), data->block_index, &data->block);
}

bool _uph_action_block_delete_undo(void* userdata)
{
	Uph_ActionBlockDelete* data = userdata;
	Uph_Track* track = uph_action_track_resolve(data->track);
	if (!track)
		return false;

	uph_block_list_insert(track, data->block_index, data->block);
	return true;
}

bool _uph_action_block_delete_redo(void* userdata)
{
	return _uph_action_block_delete_execute(userdata);
}

#pragma endregion

#pragma region Block Move / Resize
bool _uph_action_block_transform_execute(void* userdata)
{
	Uph_ActionBlockTransform* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_action_track_resolve(data->dst_track) != NULL;
	}

	return uph_block_relocate(data->src_track, data->src_index, data->dst_track, data->dst_index, data->new_start, data->new_length, data->new_offset);
}

bool _uph_action_block_transform_undo(void* userdata)
{
	Uph_ActionBlockTransform* data = userdata;
	return uph_block_relocate(data->dst_track, data->dst_index, data->src_track, data->src_index, data->old_start, data->old_length, data->old_offset);
}

bool _uph_action_block_transform_redo(void* userdata)
{
	Uph_ActionBlockTransform* data = userdata;
	return uph_block_relocate(data->src_track, data->src_index, data->dst_track, data->dst_index, data->new_start, data->new_length, data->new_offset);
}

#pragma endregion

#pragma region Block Group Operations
static void uph_block_delete_selected(void)
{
	Naui_List(Uph_ActionBlockSnapshot) selected = NULL;
	uph_block_collect_all(UPH_RESOURCE_NONE, true, false, 0, &selected);
	if (naui_list_len(selected) == 0)
		return;

	naui_action_group_start(UPH_ACTION_BLOCKS_DELETE_GROUP);
	for (uint32_t i = (uint32_t)naui_list_len(selected); i > 0; i--)
	{
		Uph_ActionBlockDelete data = { .track = selected[i - 1].track, .block_index = selected[i - 1].index };
		naui_action_execute_stack(UPH_ACTION_BLOCK_DELETE, data);
	}

	naui_action_group_end();
	naui_list_free(selected);
}

static void uph_block_duplicate_selected(void)
{
	Naui_List(Uph_ActionBlockSnapshot) selected = NULL;
	uph_block_collect_all(UPH_RESOURCE_NONE, true, false, 0, &selected);
	if (naui_list_len(selected) == 0)
		return;

	double first_start = selected[0].block.start_beat;
	double last_end = selected[0].block.start_beat + selected[0].block.length_beats;
	for (uint32_t i = 1; i < (uint32_t)naui_list_len(selected); i++)
	{
		first_start = fmin(first_start, selected[i].block.start_beat);
		last_end = fmax(last_end, selected[i].block.start_beat + selected[i].block.length_beats);
	}

	const double shift = last_end - first_start;
	uph_block_select_all(false);

	naui_action_group_start(UPH_ACTION_BLOCKS_DUPLICATE_GROUP);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		Uph_Track* track = uph_action_track_resolve(selected[i].track);
		if (!track)
			continue;

		Uph_ActionBlockCreate data = { .track = selected[i].track, .block_index = (uint32_t)naui_list_len(track->blocks), .block = selected[i].block };
		data.block.start_beat += shift;
		data.block.selected = true;
		data.block.visual_lifetime = 0.0f;
		naui_action_execute_stack(UPH_ACTION_BLOCK_CREATE, data);
	}

	naui_action_group_end();
	naui_list_free(selected);
}

static bool uph_block_cut(Uph_ActionTrackRef ref, uint32_t block_index, double cut_beat, double min_length)
{
	Uph_Track* track = uph_action_track_resolve(ref);
	if (!track || block_index >= (uint32_t)naui_list_len(track->blocks))
		return false;

	const Uph_TimelineBlock block = track->blocks[block_index];
	const double left_length = cut_beat - block.start_beat;
	const double right_length = block.length_beats - left_length;
	if (left_length < min_length || right_length < min_length)
		return false;

	naui_action_group_start(UPH_ACTION_BLOCK_CUT_GROUP);
	Uph_ActionBlockTransform shrink = {
		.src_track = ref, .src_index = block_index, .dst_track = ref, .dst_index = block_index,
		.old_start = block.start_beat, .old_length = block.length_beats, .old_offset = block.start_offset_beats,
		.new_start = block.start_beat, .new_length = left_length, .new_offset = block.start_offset_beats
	};
	naui_action_execute_stack(UPH_ACTION_BLOCK_RESIZE, shrink);

	Uph_ActionBlockCreate right = { .track = ref, .block_index = (uint32_t)naui_list_len(track->blocks), .block = block };
	right.block.start_beat = cut_beat;
	right.block.length_beats = right_length;
	right.block.start_offset_beats = block.start_offset_beats + left_length;
	naui_action_execute_stack(UPH_ACTION_BLOCK_CREATE, right);

	naui_action_group_end();
	return true;
}

#pragma endregion
