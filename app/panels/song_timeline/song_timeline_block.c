static bool _uph_song_timeline_block_geometry_equal(const Uph_TimelineBlock *a, const Uph_TimelineBlock *b)
{
	return a->start_beat == b->start_beat && a->length_beats == b->length_beats && a->start_offset_beats == b->start_offset_beats;
}

static Uph_TimelineBlock *_uph_song_timeline_group_block(const Uph_DraggingBlockState *drag, uint32_t k)
{
	const bool relocated = drag->group_row_offset != 0 && k < (uint32_t)naui_list_len(drag->group_moves);
	Uph_Track *track = relocated ? drag->group_moves[k].dst_track : uph_action_track_resolve(drag->group_blocks[k].track);
	const uint32_t index = relocated ? drag->group_moves[k].current_index : drag->group_blocks[k].index;
	if (!track || index >= (uint32_t)naui_list_len(track->blocks))
		return NULL;

	return &track->blocks[index];
}

static void _uph_song_timeline_collect_rows(Naui_List(Uph_Track) tracks, Naui_List(Uph_TrackPtr) *rows)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		Uph_Track *track = &tracks[i];
		naui_list_push(*rows, track);

		if (!(track->type == UPH_RESOURCE_GROUP && (track->state & UPH_TRACK_TIMELINE_COLLAPSED)))
			_uph_song_timeline_collect_rows(track->subtracks, rows);
	}
}

static int32_t _uph_song_timeline_row_of(Naui_List(Uph_TrackPtr) rows, const Uph_Track *track)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(rows); i++)
	{
		if (rows[i] == track)
			return (int32_t)i;
	}

	return -1;
}

static Uph_ResourceType _uph_song_timeline_home_type(const Uph_DraggingBlockState *drag, const Uph_Track *track)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->type_backups); i++)
	{
		if (drag->type_backups[i].track == track)
			return drag->type_backups[i].type;
	}

	return track->type;
}

static void _uph_song_timeline_set_track_type(Uph_DraggingBlockState *drag, Uph_Track *track, Uph_ResourceType type)
{
	if (track->type == type)
		return;

	bool backed_up = false;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->type_backups); i++)
		backed_up |= drag->type_backups[i].track == track;

	if (!backed_up)
	{
		Uph_TrackTypeBackup backup = { .track = track, .type = track->type };
		naui_list_push(drag->type_backups, backup);
	}

	track->type = type;
}

static Uph_Track *_uph_song_timeline_group_target(const Uph_DraggingBlockState *drag, Naui_List(Uph_TrackPtr) rows, uint32_t k, int32_t offset)
{
	const Uph_Track *source = uph_action_track_resolve(drag->group_blocks[k].track);
	const int32_t row = source ? _uph_song_timeline_row_of(rows, source) : -1;
	if (row < 0 || row + offset < 0 || row + offset >= (int32_t)naui_list_len(rows))
		return NULL;

	return rows[row + offset];
}

static bool _uph_song_timeline_group_offset_valid(const Uph_DraggingBlockState *drag, Naui_List(Uph_TrackPtr) rows, int32_t offset)
{
	const uint32_t count = (uint32_t)naui_list_len(drag->group_blocks);
	for (uint32_t k = 0; k < count; k++)
	{
		const Uph_Track *target = _uph_song_timeline_group_target(drag, rows, k, offset);
		if (!target)
			return false;

		const Uph_ResourceType block_type = drag->group_blocks[k].block.type;
		const Uph_ResourceType target_type = _uph_song_timeline_home_type(drag, target);

		if (block_type == UPH_RESOURCE_AUTOMATION)
		{
			if (target_type != UPH_RESOURCE_AUTOMATION)
				return false;

			continue;
		}

		if (target_type == block_type)
			continue;

		if (target_type != UPH_RESOURCE_NONE)
			return false;

		for (uint32_t j = 0; j < k; j++)
		{
			if (_uph_song_timeline_group_target(drag, rows, j, offset) == target && drag->group_blocks[j].block.type != block_type)
				return false;
		}
	}

	return true;
}

static void _uph_song_timeline_group_revert(Uph_DraggingBlockState *drag)
{
	for (uint32_t k = (uint32_t)naui_list_len(drag->group_moves); k > 0; k--)
	{
		const Uph_GroupBlockMove *move = &drag->group_moves[k - 1];
		const Uph_TimelineBlock block = move->dst_track->blocks[move->dst_index];
		naui_list_remove(move->dst_track->blocks, move->dst_index);
		naui_list_insert(move->src_track->blocks, block, move->src_index);
	}

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->type_backups); i++)
		drag->type_backups[i].track->type = drag->type_backups[i].type;

	naui_list_clear(drag->group_moves);
	naui_list_clear(drag->type_backups);
	drag->group_row_offset = 0;
}

static void _uph_song_timeline_group_apply(Uph_DraggingBlockState *drag, Naui_List(Uph_TrackPtr) rows, int32_t offset)
{
	const uint32_t count = (uint32_t)naui_list_len(drag->group_blocks);
	for (uint32_t k = 0; k < count; k++)
	{
		Uph_Track *source = uph_action_track_resolve(drag->group_blocks[k].track);
		uint32_t taken_before = 0;
		for (uint32_t j = 0; j < k; j++)
			taken_before += uph_action_track_ref_equal(drag->group_blocks[j].track, drag->group_blocks[k].track);

		Uph_GroupBlockMove move = {
			.src_track = source,
			.dst_track = _uph_song_timeline_group_target(drag, rows, k, offset),
			.src_index = drag->group_blocks[k].index - taken_before
		};

		const Uph_TimelineBlock block = source->blocks[move.src_index];
		naui_list_remove(source->blocks, move.src_index);
		move.dst_index = (uint32_t)naui_list_len(move.dst_track->blocks);
		naui_list_push(move.dst_track->blocks, block);
		naui_list_push(drag->group_moves, move);
	}

	for (uint32_t k = 0; k < count; k++)
	{
		Uph_GroupBlockMove *move = &drag->group_moves[k];
		uint32_t incoming = 0;
		uint32_t rank = 0;
		for (uint32_t j = 0; j < count; j++)
		{
			if (drag->group_moves[j].dst_track != move->dst_track)
				continue;

			incoming++;
			rank += j < k;
		}

		move->current_index = (uint32_t)naui_list_len(move->dst_track->blocks) - incoming + rank;
	}

	for (uint32_t k = 0; k < count; k++)
	{
		const Uph_GroupBlockMove *move = &drag->group_moves[k];
		if (drag->group_blocks[k].block.type == UPH_RESOURCE_AUTOMATION)
			continue;

		if (move->dst_track->type == UPH_RESOURCE_NONE)
			_uph_song_timeline_set_track_type(drag, move->dst_track, drag->group_blocks[k].block.type);

		if (naui_list_len(move->src_track->blocks) == 0 && !move->src_track->instrument.loaded)
			_uph_song_timeline_set_track_type(drag, move->src_track, UPH_RESOURCE_NONE);
	}

	drag->group_row_offset = offset;
	drag->track = drag->group_moves[drag->group_grabbed].dst_track;
	drag->block_index = drag->group_moves[drag->group_grabbed].current_index;
}

static void _uph_song_timeline_update_group_track_switch(Uph_DraggingBlockState *drag, Uph_Track *hovered)
{
	Naui_List(Uph_TrackPtr) rows = NULL;
	_uph_song_timeline_collect_rows(uph_state.project.tracks, &rows);

	const Uph_Track *home = uph_action_track_resolve(drag->source_track);
	const int32_t home_row = home ? _uph_song_timeline_row_of(rows, home) : -1;
	const int32_t hovered_row = _uph_song_timeline_row_of(rows, hovered);

	if (home_row >= 0 && hovered_row >= 0)
	{
		const int32_t offset = hovered_row - home_row;
		if (offset != drag->group_row_offset && _uph_song_timeline_group_offset_valid(drag, rows, offset))
		{
			_uph_song_timeline_group_revert(drag);
			if (offset != 0)
				_uph_song_timeline_group_apply(drag, rows, offset);
			else
			{
				drag->track = uph_action_track_resolve(drag->source_track);
				drag->block_index = drag->source_index;
			}
		}
	}

	naui_list_free(rows);
}

#pragma region Public API

Uph_TimelineBlock uph_song_timeline_block_create(double start_beat, uint32_t resource_index, Uph_ResourceType block_type)
{
	if (uph_state.shared.current_pattern_updated && block_type == UPH_RESOURCE_PATTERN)
	{
		uph_state.shared.song_timeline_current_block_start_offset = 0.0;
		uph_state.shared.song_timeline_current_block_length = uph_calculate_pattern_length(&uph_state.project.midi_patterns[resource_index]);
		uph_state.shared.current_pattern_updated = false;
	}

	Uph_TimelineBlock block = {
		.start_beat = start_beat,
		.start_offset_beats = uph_state.shared.song_timeline_current_block_start_offset,
		.length_beats = uph_state.shared.song_timeline_current_block_length,
		.resource_index = resource_index,
		.type = block_type
	};

	return block;
}

void uph_song_timeline_update_drag_track_switch(void)
{
	Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
	if (!drag->active)
		return;

	if (drag->mode != UPH_BLOCK_INTERACTION_MOVE)
		return;

	Uph_Track *new_track = uph_song_timeline_data.current_hovered_track;

	if (!new_track || new_track == drag->track)
		return;

	if (drag->group)
	{
		_uph_song_timeline_update_group_track_switch(drag, new_track);
		return;
	}

	Uph_Track *old_track = drag->track;

	if (new_track->type != UPH_RESOURCE_NONE && new_track->type != old_track->type)
		return;
	
	if (old_track->type == UPH_RESOURCE_AUTOMATION && new_track->type != UPH_RESOURCE_AUTOMATION)
		return;

	Uph_TimelineBlock moved = old_track->blocks[drag->block_index];
	naui_list_remove(old_track->blocks, drag->block_index);
	naui_list_push(new_track->blocks, moved);

	if (old_track->type != UPH_RESOURCE_AUTOMATION)
	{
		if (new_track->type == UPH_RESOURCE_NONE)
			new_track->type = moved.type;
		if (naui_list_len(old_track->blocks) == 0 && !old_track->instrument.loaded)
			old_track->type = UPH_RESOURCE_NONE;
	}

	drag->track = new_track;
	drag->block_index = (uint32_t)naui_list_len(new_track->blocks) - 1;
}

void uph_song_timeline_begin_block_drag(Uph_Track *track, uint32_t block_index, Uph_BlockInteractionMode mode, double mouse_beat)
{
	Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
	Uph_TimelineBlock *block = &track->blocks[block_index];
	naui_list_free(drag->group_blocks);
	drag->group_blocks = NULL;
	drag->active = true;
	drag->creating = false;
	drag->group = false;
	drag->additive = false;
	drag->moved = false;
	drag->group_row_offset = 0;
	drag->group_grabbed = 0;
	naui_list_clear(drag->group_moves);
	naui_list_clear(drag->type_backups);
	drag->initial_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
	drag->track = track;
	drag->block_index = block_index;
	drag->source_track = uph_action_track_ref(track);
	drag->source_index = block_index;
	drag->mode = mode;
	drag->initial_start_beat = block->start_beat;
	drag->initial_length_beats = block->length_beats;
	drag->initial_start_offset_beats = block->start_offset_beats;
	drag->initial_drag_beat_offset = block->start_beat - mouse_beat;

	drag->select_tool = uph_song_timeline_tool() == UPH_ACTION_SELECT;
	drag->additive = drag->select_tool && (uph_ui_ctrl_down() || uph_ui_shift_down());
	if (drag->additive)
	{
		block->selected = !block->selected;
		if (!block->selected)
		{
			drag->active = false;
			drag->mode = UPH_BLOCK_INTERACTION_NONE;
			return;
		}
	}
	else if (!block->selected)
	{
		uph_block_select_all(false);
		block->selected = drag->select_tool;
	}

	if (block->selected)
	{
		drag->group = true;
		uph_block_collect_all(UPH_RESOURCE_NONE, true, false, 0, &drag->group_blocks);
		for (uint32_t k = 0; k < (uint32_t)naui_list_len(drag->group_blocks); k++)
		{
			if (drag->group_blocks[k].index == block_index && uph_action_track_ref_equal(drag->group_blocks[k].track, drag->source_track))
				drag->group_grabbed = k;
		}
	}

	uph_state.shared.selected_resource.type = block->type;
	uph_state.shared.selected_resource.index = block->resource_index;
	uph_state.shared.song_timeline_current_block_length = block->length_beats;
	uph_state.shared.song_timeline_current_block_start_offset = block->start_offset_beats;
	uph_state.shared.current_pattern_updated = false;
}

void uph_song_timeline_apply_group_drag(const Uph_TimelineBlock *grabbed)
{
	const Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
	const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);
	double delta_start = grabbed->start_beat - drag->initial_start_beat;
	double delta_length = grabbed->length_beats - drag->initial_length_beats;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_blocks); i++)
	{
		const Uph_TimelineBlock *initial = &drag->group_blocks[i].block;
		if (drag->mode == UPH_BLOCK_INTERACTION_MOVE)
		{
			delta_start = fmax(delta_start, -initial->start_beat);
		}
		else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
		{
			delta_start = fmax(delta_start, -initial->start_offset_beats);
			delta_start = fmin(delta_start, initial->length_beats - division);
		}
		else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
		{
			delta_length = fmax(delta_length, division - initial->length_beats);
		}
	}

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_blocks); i++)
	{
		const Uph_ActionBlockSnapshot *snapshot = &drag->group_blocks[i];
		Uph_TimelineBlock *block = _uph_song_timeline_group_block(drag, i);
		if (!block)
			continue;

		if (drag->mode == UPH_BLOCK_INTERACTION_MOVE)
		{
			block->start_beat = snapshot->block.start_beat + delta_start;
		}
		else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
		{
			block->start_beat = snapshot->block.start_beat + delta_start;
			block->length_beats = snapshot->block.length_beats - delta_start;
			block->start_offset_beats = snapshot->block.start_offset_beats + delta_start;
		}
		else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
		{
			block->length_beats = snapshot->block.length_beats + delta_length;
		}
	}
}

void uph_song_timeline_finish_block_drag(void)
{
	Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
	const char *transform_action = drag->mode == UPH_BLOCK_INTERACTION_MOVE ? UPH_ACTION_BLOCK_MOVE : UPH_ACTION_BLOCK_RESIZE;
	if (drag->creating)
	{
		if (drag->block_index < (uint32_t)naui_list_len(drag->track->blocks))
		{
			Uph_ActionBlockCreate data = {
				.track = uph_action_track_ref(drag->track),
				.block_index = drag->block_index,
				.block = drag->track->blocks[drag->block_index],
				.applied_live = true
			};
			naui_action_execute_stack(UPH_ACTION_BLOCK_CREATE, data);
		}
	}
	else if (drag->group)
	{
		bool changed = false;
		const bool relocated = drag->group_row_offset != 0;
		naui_action_group_start(drag->mode == UPH_BLOCK_INTERACTION_MOVE ? UPH_ACTION_BLOCKS_MOVE_GROUP : UPH_ACTION_BLOCKS_RESIZE_GROUP);

		for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_blocks); i++)
		{
			const Uph_ActionBlockSnapshot *snapshot = &drag->group_blocks[i];
			const Uph_TimelineBlock *block = _uph_song_timeline_group_block(drag, i);
			if (!block)
				continue;

			if (!relocated && _uph_song_timeline_block_geometry_equal(block, &snapshot->block))
				continue;

			const Uph_GroupBlockMove *move = relocated ? &drag->group_moves[i] : NULL;
			Uph_ActionBlockTransform data = {
				.src_track = move ? uph_action_track_ref(move->src_track) : snapshot->track, .src_index = move ? move->src_index : snapshot->index,
				.dst_track = move ? uph_action_track_ref(move->dst_track) : snapshot->track, .dst_index = move ? move->dst_index : snapshot->index,
				.old_start = snapshot->block.start_beat, .old_length = snapshot->block.length_beats, .old_offset = snapshot->block.start_offset_beats,
				.new_start = block->start_beat, .new_length = block->length_beats, .new_offset = block->start_offset_beats,
				.applied_live = true
			};
			naui_action_execute_stack(transform_action, data);
			changed = true;
		}

		naui_action_group_end();
		if (!changed && !drag->moved && !drag->additive && drag->select_tool)
		{
			Uph_Track *source = uph_action_track_resolve(drag->source_track);
			if (source && drag->source_index < (uint32_t)naui_list_len(source->blocks))
			{
				uph_block_select_all(false);
				source->blocks[drag->source_index].selected = true;
			}
		}
	}
	else if (drag->block_index < (uint32_t)naui_list_len(drag->track->blocks))
	{
		const Uph_TimelineBlock *block = &drag->track->blocks[drag->block_index];
		const Uph_ActionTrackRef destination = uph_action_track_ref(drag->track);
		const bool moved_track = !uph_action_track_ref_equal(destination, drag->source_track);

		const Uph_TimelineBlock initial = {
			.start_beat = drag->initial_start_beat,
			.length_beats = drag->initial_length_beats,
			.start_offset_beats = drag->initial_start_offset_beats
		};

		if (moved_track || !_uph_song_timeline_block_geometry_equal(block, &initial))
		{
			Uph_ActionBlockTransform data = {
				.src_track = drag->source_track, .src_index = drag->source_index,
				.dst_track = destination, .dst_index = drag->block_index,
				.old_start = initial.start_beat, .old_length = initial.length_beats, .old_offset = initial.start_offset_beats,
				.new_start = block->start_beat, .new_length = block->length_beats, .new_offset = block->start_offset_beats,
				.applied_live = true
			};
			naui_action_execute_stack(transform_action, data);
		}
	}

	naui_list_free(drag->group_blocks);
	drag->group_blocks = NULL;
	drag->active = false;
	drag->creating = false;
	drag->group = false;
	drag->group_row_offset = 0;
	naui_list_clear(drag->group_moves);
	naui_list_clear(drag->type_backups);
	drag->mode = UPH_BLOCK_INTERACTION_NONE;
}

#pragma endregion
