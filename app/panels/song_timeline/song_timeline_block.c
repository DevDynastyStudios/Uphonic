static bool _uph_song_timeline_block_geometry_equal(const Uph_TimelineBlock *a, const Uph_TimelineBlock *b)
{
	return a->start_beat == b->start_beat && a->length_beats == b->length_beats && a->start_offset_beats == b->start_offset_beats;
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

	if (drag->mode != UPH_BLOCK_INTERACTION_MOVE || drag->group)
		return;

	Uph_Track *new_track = uph_song_timeline_data.current_hovered_track;

	if (!new_track || new_track == drag->track)
		return;

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
	drag->track = track;
	drag->block_index = block_index;
	drag->source_track = uph_action_track_ref(track);
	drag->source_index = block_index;
	drag->mode = mode;
	drag->initial_start_beat = block->start_beat;
	drag->initial_length_beats = block->length_beats;
	drag->initial_start_offset_beats = block->start_offset_beats;
	drag->initial_drag_beat_offset = block->start_beat - mouse_beat;

	if (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT)
	{
		drag->additive = uph_ui_ctrl_down() || uph_ui_shift_down();
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
			block->selected = true;
		}

		drag->group = true;
		uph_block_collect_all(UPH_RESOURCE_NONE, true, false, 0, &drag->group_blocks);
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
		Uph_Track *track = uph_action_track_resolve(snapshot->track);
		if (!track || snapshot->index >= (uint32_t)naui_list_len(track->blocks))
			continue;

		Uph_TimelineBlock *block = &track->blocks[snapshot->index];
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
		naui_action_group_start(drag->mode == UPH_BLOCK_INTERACTION_MOVE ? UPH_ACTION_BLOCKS_MOVE_GROUP : UPH_ACTION_BLOCKS_RESIZE_GROUP);

		for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_blocks); i++)
		{
			const Uph_ActionBlockSnapshot *snapshot = &drag->group_blocks[i];
			Uph_Track *track = uph_action_track_resolve(snapshot->track);
			if (!track || snapshot->index >= (uint32_t)naui_list_len(track->blocks))
				continue;

			const Uph_TimelineBlock *block = &track->blocks[snapshot->index];
			if (_uph_song_timeline_block_geometry_equal(block, &snapshot->block))
				continue;

			Uph_ActionBlockTransform data = {
				.src_track = snapshot->track, .src_index = snapshot->index,
				.dst_track = snapshot->track, .dst_index = snapshot->index,
				.old_start = snapshot->block.start_beat, .old_length = snapshot->block.length_beats, .old_offset = snapshot->block.start_offset_beats,
				.new_start = block->start_beat, .new_length = block->length_beats, .new_offset = block->start_offset_beats,
				.applied_live = true
			};
			naui_action_execute_stack(transform_action, data);
			changed = true;
		}

		naui_action_group_end();
		if (!changed && !drag->additive)
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
		const bool moved_track = destination.parent != drag->source_track.parent || destination.index != drag->source_track.index;

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
	drag->mode = UPH_BLOCK_INTERACTION_NONE;
}

#pragma endregion
