static void _uph_song_timeline_update_track_timeline_drag(Leaf_BoundingBox bbox, Uph_Track *track)
{
	Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
	Uph_AutomationEditState *automation_edit = &uph_song_timeline_data.automation_edit;
	Naui_List(Uph_TimelineBlock) blocks = track->blocks;
	const float zoom_x = uph_song_timeline_data.zoom.x;
	const float scroll_x = uph_song_timeline_data.scroll.x;

	const float title_padding = NAUI_DPI(2.0f);
	const float font_size = NAUI_DPI(13.0f);
	const float title_height = font_size + title_padding * 2.0f;

	if (automation_edit->dragging_point_index >= 0 && automation_edit->track == track)
	{
		const uint32_t i = automation_edit->block_index;
		if (i >= (uint32_t)naui_list_len(blocks))
		{
			automation_edit->dragging_point_index = -1;
			automation_edit->creating = false;
			return;
		}

		const float block_left = bbox.x + zoom_x * blocks[i].start_beat - scroll_x;

		Naui_Vec2 automation_pos = { block_left, bbox.y + title_height };
		Naui_Vec2 automation_size = { zoom_x * blocks[i].length_beats, bbox.height - title_height };

		uph_song_timeline_update_automation_point_drag(
			automation_pos,
			automation_size,
			track,
			i,
			blocks[i].start_offset_beats,
			blocks[i].resource_index
		);
		automation_edit->active = true;
		return;
	}

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
	{
		bool is_dragging_this_block =
			drag->active &&
			drag->track == track &&
			drag->block_index == i;

		if (is_dragging_this_block)
		{
			double mouse_beat = ((double)naui_mouse_x() - bbox.x + scroll_x) / zoom_x;

			if (drag->mode == UPH_BLOCK_INTERACTION_MOVE)
			{
				blocks[i].start_beat = fmax(0.0, uph_snap_beat_floor(mouse_beat + drag->initial_drag_beat_offset, uph_song_timeline_data.snap_resolution));
				naui_set_cursor(NAUI_CURSOR_HAND);
			}
			else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
			{
				const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);
				double new_start = uph_snap_beat_round(mouse_beat, uph_song_timeline_data.snap_resolution);
				double end_beat = drag->initial_start_beat + drag->initial_length_beats;

				const double earliest_start_beat = drag->initial_start_beat - drag->initial_start_offset_beats;
				new_start = fmax(new_start, earliest_start_beat);
				new_start = fmin(new_start, end_beat - division);
				const double delta_beats = new_start - drag->initial_start_beat;

				blocks[i].start_beat = new_start;
				blocks[i].length_beats = end_beat - new_start;
				blocks[i].start_offset_beats = drag->initial_start_offset_beats + delta_beats;
				
				uph_state.shared.song_timeline_current_block_length = blocks[i].length_beats;
				uph_state.shared.song_timeline_current_block_start_offset = blocks[i].start_offset_beats;
				uph_state.shared.current_pattern_updated = false;

				naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
			}
			else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
			{
				const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);
				double raw_length = mouse_beat - blocks[i].start_beat;
				double snapped_end = uph_snap_beat_round(blocks[i].start_beat + raw_length, uph_song_timeline_data.snap_resolution);
				double new_length = snapped_end - blocks[i].start_beat;
				new_length = fmax(division, new_length);
				blocks[i].length_beats = new_length;

				uph_state.shared.song_timeline_current_block_length = blocks[i].length_beats;
				uph_state.shared.song_timeline_current_block_start_offset = blocks[i].start_offset_beats;
				uph_state.shared.current_pattern_updated = false;

				naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
			}

			if (drag->group)
				uph_song_timeline_apply_group_drag(&blocks[i]);

			if (naui_mouse_released(NAUI_MOUSE_LEFT))
				uph_song_timeline_finish_block_drag();

			return;
		}

		if (!uph_song_timeline_data.tracks_hovered)
			return;

		if (!drag->active)
		{
			const float block_left = bbox.x + zoom_x * blocks[i].start_beat - scroll_x;
			const float block_right = block_left + zoom_x * blocks[i].length_beats;

			const float clamped_left = fmaxf(bbox.x, block_left);
			const float clamped_right = fminf(bbox.x + bbox.width, block_right);

			if (clamped_right <= clamped_left)
				continue;

			Naui_Vec4 hover_box = (Naui_Vec4){
				clamped_left,
				bbox.y,
				clamped_right - clamped_left,
				blocks[i].type == UPH_RESOURCE_AUTOMATION ? title_height : bbox.height
			};

			if (uph_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
			{
				Uph_BlockInteractionMode hover_mode = uph_song_timeline_classify_hover(hover_box, (float)naui_mouse_x());
				if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT || uph_song_timeline_data.current_action_mode == UPH_ACTION_DRAW))
				{
					const double grab_beat = ((double)naui_mouse_x() - bbox.x + scroll_x) / zoom_x;
					uph_song_timeline_begin_block_drag(track, i, hover_mode, grab_beat);
				}

				naui_set_cursor(hover_mode == UPH_BLOCK_INTERACTION_MOVE ? NAUI_CURSOR_HAND : NAUI_CURSOR_RESIZE_EW);
				uph_song_timeline_data.hovered_block.block_index = i;
				uph_song_timeline_data.hovered_block.track = track;
				uph_song_timeline_data.hovered_block.active = true;
			}
			else if (blocks[i].type == UPH_RESOURCE_AUTOMATION)
			{
				hover_box.y += title_height;
				hover_box.w = bbox.height - title_height;

				if (uph_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
				{
					Naui_Vec2 automation_pos = { block_left, bbox.y + title_height };
					Naui_Vec2 automation_size = { zoom_x * blocks[i].length_beats, bbox.height - title_height };

					uph_song_timeline_update_automation_point_drag(
						automation_pos,
						automation_size,
						track,
						i,
						blocks[i].start_offset_beats,
						blocks[i].resource_index
					);
					automation_edit->active = true;
				}
			}
		}
	}
}

static void _uph_song_timeline_delete_hovered_block(Uph_Track *track)
{
	const uint32_t block_index = uph_song_timeline_data.hovered_block.block_index;
	if (block_index >= (uint32_t)naui_list_len(track->blocks))
		return;

	if (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT && track->blocks[block_index].selected && uph_block_selected_count() > 1)
	{
		uph_block_delete_selected();
		return;
	}

	Uph_ActionBlockDelete data = { .track = uph_action_track_ref(track), .block_index = block_index };
	naui_action_execute_stack(UPH_ACTION_BLOCK_DELETE, data);
}

static void uph_song_timeline_update_track_action_input(Leaf_BoundingBox bbox, Uph_Track *track)
{
	if (!uph_song_timeline_data.tracks_hovered)
		return;

	if (uph_song_timeline_data.automation_edit.active)
		return;

	if (!uph_song_timeline_data.panel_hovered)
		return;

	if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_song_timeline_data.hovered_block.active && uph_song_timeline_data.hovered_block.track == track)
		_uph_song_timeline_delete_hovered_block(track);

	const bool mouse_over_track = uph_song_timeline_vec4_contains_vec2(
		(Naui_Vec4){ bbox.x, bbox.y, bbox.width, bbox.height },
		(Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() }
	);

	if (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT)
	{
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && mouse_over_track && !uph_song_timeline_data.hovered_block.active && !uph_song_timeline_data.drag.active)
		{
			Uph_MarqueeState *marquee = &uph_song_timeline_data.marquee;
			marquee->active = true;
			marquee->start_beat = ((double)naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
			marquee->start_content_y = (float)naui_mouse_y() - uph_song_timeline_data.panel_bounding_box.y + uph_song_timeline_data.scroll.y;
			uph_block_select_all(false);
		}
	}
	else if (uph_song_timeline_data.current_action_mode == UPH_ACTION_DRAW)
	{
		if (uph_state.shared.selected_resource.type == UPH_RESOURCE_NONE)
			return;

		else if (uph_state.shared.selected_resource.type == UPH_RESOURCE_SAMPLE && !uph_state.project.samples)
			return;

		else if (uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN && !uph_state.project.midi_patterns)
			return;

		if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && !uph_song_timeline_data.hovered_block.active &&
			(uph_state.shared.selected_resource.type == UPH_RESOURCE_AUTOMATION ? track->type == UPH_RESOURCE_AUTOMATION : (track->type == UPH_RESOURCE_NONE || track->type == uph_state.shared.selected_resource.type)))
		{
			if (mouse_over_track)
			{
				const float beat = (naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
				naui_list_free(uph_song_timeline_data.drag.group_blocks);
				
				uph_song_timeline_data.drag.group_blocks = NULL;
				uph_song_timeline_data.drag.active = true;
				uph_song_timeline_data.drag.creating = true;
				uph_song_timeline_data.drag.group = false;
				uph_song_timeline_data.drag.additive = false;
				uph_song_timeline_data.drag.block_index = naui_list_len(track->blocks);
				uph_song_timeline_data.drag.track = track;
				uph_song_timeline_data.drag.mode = UPH_BLOCK_INTERACTION_MOVE;
				uph_song_timeline_data.drag.initial_drag_beat_offset = 0.0;
				naui_list_push(track->blocks, uph_song_timeline_block_create(uph_snap_beat_floor(beat, uph_song_timeline_data.snap_resolution), uph_state.shared.selected_resource.index, uph_state.shared.selected_resource.type));

				if (track->type == UPH_RESOURCE_NONE)
					track->type = uph_state.shared.selected_resource.type;
			}
		}
	}
	else if (uph_song_timeline_data.current_action_mode == UPH_ACTION_CUT)
	{
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && uph_song_timeline_data.hovered_block.active && uph_song_timeline_data.hovered_block.track == track)
		{
			const uint32_t block_index = uph_song_timeline_data.hovered_block.block_index;
			const double mouse_beat = ((double)naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
			const double cut_beat = uph_snap_beat_round(mouse_beat, uph_song_timeline_data.snap_resolution);
			uph_block_cut(uph_action_track_ref(track), block_index, cut_beat, uph_snap_division(uph_song_timeline_data.snap_resolution));
		}
	}
}

static void _uph_song_timeline_update_marquee(Leaf_BoundingBox bbox, Uph_Track *track)
{
	const Uph_MarqueeState *marquee = &uph_song_timeline_data.marquee;
	if (!marquee->active)
		return;

	const float zoom_x = uph_song_timeline_data.zoom.x;
	const float scroll_x = uph_song_timeline_data.scroll.x;

	const float corner_x = bbox.x + (float)(marquee->start_beat * zoom_x) - scroll_x;
	const float corner_y = uph_song_timeline_data.panel_bounding_box.y - uph_song_timeline_data.scroll.y + marquee->start_content_y;

	const float left = fminf(corner_x, (float)naui_mouse_x());
	const float right = fmaxf(corner_x, (float)naui_mouse_x());
	const float top = fminf(corner_y, (float)naui_mouse_y());
	const float bottom = fmaxf(corner_y, (float)naui_mouse_y());
	const bool row_touched = bbox.y < bottom && bbox.y + bbox.height > top;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->blocks); i++)
	{
		Uph_TimelineBlock *block = &track->blocks[i];
		const float block_left = bbox.x + (float)(block->start_beat * zoom_x) - scroll_x;
		const float block_right = block_left + (float)(block->length_beats * zoom_x);
		block->selected = row_touched && block_left < right && block_right > left;
	}
}

static void _uph_song_timeline_render_track_timeline_overlay(Leaf_BoundingBox bbox, Uph_ActionTrackRef *track_ref)
{
	Uph_Track *track = uph_action_track_resolve(*track_ref);
	if (!track)
		return;

	const bool mouse_over_track = uph_song_timeline_vec4_contains_vec2(
		(Naui_Vec4){ bbox.x, bbox.y, bbox.width, bbox.height },
		(Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() }
	);

	if (mouse_over_track)
		uph_song_timeline_data.current_hovered_track = track;

	naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);
	uph_song_timeline_render_ruler(bbox, uph_song_timeline_data.zoom.x, uph_song_timeline_data.scroll.x);

	_uph_song_timeline_update_track_timeline_drag(bbox, track);
	uph_song_timeline_update_track_action_input(bbox, track);

	track = uph_action_track_resolve(*track_ref);
	if (!track)
	{
		naui_pop_clip_rect();
		return;
	}

	_uph_song_timeline_update_marquee(bbox, track);
	uph_song_timeline_render_track_timeline_blocks(bbox, track);

	naui_pop_clip_rect();
}

void uph_song_timeline_render_track_timeline(Uph_Track *track)
{
	const uint32_t row_counter = uph_song_timeline_data.visual_row_counter;
	const Leaf_Color bg_color = row_counter & 1 ? naui_theme_color("uph_timeline_row_bg1_color") : naui_theme_color("uph_timeline_row_bg2_color");
	const Leaf_Color border_color = naui_theme_color("uph_song_timeline_border_color");

	const float row_height = NAUI_DPI(uph_song_timeline_data.zoom.y);
	const float row_y = uph_song_timeline_data.panel_bounding_box.y - uph_song_timeline_data.scroll.y + (float)row_counter * row_height;

	Uph_ActionTrackRef track_ref = uph_action_track_ref(track);
	const bool row_visible = row_y + row_height >= uph_song_timeline_data.panel_bounding_box.y && row_y <= uph_song_timeline_data.panel_bounding_box.y + uph_song_timeline_data.panel_bounding_box.height;

	leaf({
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FULL },
		.color = bg_color,
		.border = {
			.width = 1,
			.color = {border_color},
			.sides = LEAF_SIDE_TOP | LEAF_SIDE_BOTTOM
		},
		.custom_draw_data = row_visible ? LEAF_DATA_SLICE(track_ref) : (Leaf_DataSlice){ 0 },
		.custom_draw = row_visible ? (Leaf_CustomDrawFn)_uph_song_timeline_render_track_timeline_overlay : NULL
	});
}
