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

	uph_song_timeline_data.block_menu.view_width = bbox.width;

	if (!uph_song_timeline_data.tracks_hovered)
		return;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
	{
		bool is_dragging_this_block =
			drag->active &&
			drag->track == track &&
			drag->block_index == i;

		if (is_dragging_this_block)
		{
			const float threshold = NAUI_DPI(UPH_SONG_TIMELINE_CLICK_THRESHOLD);
			if (fabsf((float)naui_mouse_x() - drag->initial_mouse.x) > threshold || fabsf((float)naui_mouse_y() - drag->initial_mouse.y) > threshold)
				drag->moved = true;

			double mouse_beat = ((double)naui_mouse_x() - bbox.x + scroll_x) / zoom_x;

			if (drag->mode == UPH_BLOCK_INTERACTION_MOVE)
			{
				blocks[i].start_beat = fmax(0.0, uph_snap_beat_floor(mouse_beat + drag->initial_drag_beat_offset, uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x));
				naui_set_cursor(NAUI_CURSOR_HAND);
			}
			else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
			{
				const double division = uph_snap_division(uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x);
				double new_start = uph_snap_beat_round(mouse_beat, uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x);
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
				const double division = uph_snap_division(uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x);
				double raw_length = mouse_beat - blocks[i].start_beat;
				double snapped_end = uph_snap_beat_round(blocks[i].start_beat + raw_length, uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x);
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

			return;
		}

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

			const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };
			const bool in_hover_box = uph_song_timeline_vec4_contains_vec2(hover_box, mouse);
			const Uph_BlockInteractionMode hover_mode = uph_song_timeline_classify_hover(hover_box, mouse.x);
			const bool resize_hovered = in_hover_box && hover_mode != UPH_BLOCK_INTERACTION_MOVE;

			Naui_Vec4 menu_button;
			if (!resize_hovered && uph_song_timeline_block_menu_button_rect(block_left, zoom_x * (float)blocks[i].length_beats, bbox.x, bbox.x + bbox.width, &menu_button))
			{
				menu_button.y += bbox.y;
				if (uph_song_timeline_vec4_contains_vec2(menu_button, mouse))
				{
					uph_song_timeline_data.block_menu.next_hover = (Uph_BlockMenuHover){ .track = track, .block_index = i, .button_hovered = true, .active = true };
					uph_song_timeline_data.hovered_block.block_index = i;
					uph_song_timeline_data.hovered_block.track = track;
					uph_song_timeline_data.hovered_block.active = true;
					naui_set_cursor(NAUI_CURSOR_HAND);

					if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
						uph_song_timeline_request_block_menu(track, i);
					continue;
				}
			}

			if (in_hover_box)
			{
				if (!resize_hovered && !uph_song_timeline_data.block_menu.next_hover.button_hovered)
					uph_song_timeline_data.block_menu.next_hover = (Uph_BlockMenuHover){ .track = track, .block_index = i, .active = true };

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

	if (track->blocks[block_index].selected && uph_block_selected_count() > 1)
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

	const Uph_BlockMenuHover *menu_hover = &uph_song_timeline_data.block_menu.next_hover;
	if (menu_hover->button_hovered && menu_hover->track == track)
		return;

	if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_song_timeline_data.hovered_block.active && uph_song_timeline_data.hovered_block.track == track)
		_uph_song_timeline_delete_hovered_block(track);

	const bool mouse_over_track = uph_song_timeline_vec4_contains_vec2(
		(Naui_Vec4){ bbox.x, bbox.y, bbox.width, bbox.height },
		(Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() }
	);

	const Uph_ActionMode tool = uph_song_timeline_tool();
	if (tool != UPH_ACTION_SELECT && naui_mouse_pressed(NAUI_MOUSE_LEFT) && mouse_over_track && !uph_song_timeline_data.hovered_block.active && !uph_song_timeline_data.drag.active)
		uph_block_select_all(false);

	if (tool == UPH_ACTION_SELECT)
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
	else if (tool == UPH_ACTION_DRAW)
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
				uph_block_select_all(false);
				naui_list_free(uph_song_timeline_data.drag.group_blocks);
				
				uph_song_timeline_data.drag.group_blocks = NULL;
				uph_song_timeline_data.drag.active = true;
				uph_song_timeline_data.drag.creating = true;
				uph_song_timeline_data.drag.group = false;
				uph_song_timeline_data.drag.additive = false;
				uph_song_timeline_data.drag.select_tool = false;
				uph_song_timeline_data.drag.group_row_offset = 0;
				uph_song_timeline_data.drag.block_index = naui_list_len(track->blocks);
				uph_song_timeline_data.drag.track = track;
				uph_song_timeline_data.drag.mode = UPH_BLOCK_INTERACTION_MOVE;
				uph_song_timeline_data.drag.initial_drag_beat_offset = 0.0;
				naui_list_push(track->blocks, uph_song_timeline_block_create(uph_snap_beat_floor(beat, uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x), uph_state.shared.selected_resource.index, uph_state.shared.selected_resource.type));

				if (track->type == UPH_RESOURCE_NONE)
					track->type = uph_state.shared.selected_resource.type;
			}
		}
	}
	else if (tool == UPH_ACTION_CUT)
	{
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && uph_song_timeline_data.hovered_block.active && uph_song_timeline_data.hovered_block.track == track)
		{
			const uint32_t block_index = uph_song_timeline_data.hovered_block.block_index;
			const double mouse_beat = ((double)naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
			const double cut_beat = uph_snap_beat_round(mouse_beat, uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x);
			uph_block_cut(uph_action_track_ref(track), block_index, cut_beat, uph_snap_division(uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x));
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

static bool _uph_song_timeline_drop_compatible(const Uph_Track *track, Uph_ResourceType type)
{
	if (type == UPH_RESOURCE_AUTOMATION)
		return track->type == UPH_RESOURCE_AUTOMATION;

	return track->type == UPH_RESOURCE_NONE || track->type == type;
}

static bool _uph_song_timeline_resource_valid(const Uph_DragResource *resource)
{
	if (resource->type == UPH_RESOURCE_PATTERN)
		return resource->index < (uint32_t)naui_list_len(uph_state.project.midi_patterns);

	if (resource->type == UPH_RESOURCE_SAMPLE)
		return resource->index < (uint32_t)naui_list_len(uph_state.project.samples);

	if (resource->type == UPH_RESOURCE_AUTOMATION)
		return resource->index < (uint32_t)naui_list_len(uph_state.project.automations);

	return false;
}

static double _uph_song_timeline_resource_length(const Uph_DragResource *resource)
{
	if (resource->type == UPH_RESOURCE_PATTERN)
		return uph_calculate_pattern_length(&uph_state.project.midi_patterns[resource->index]);

	if (resource->type == UPH_RESOURCE_AUTOMATION)
		return uph_calculate_automation_length(&uph_state.project.automations[resource->index]);

	const Uph_Sample *sample = &uph_state.project.samples[resource->index];
	if (sample->data_index >= (uint32_t)naui_list_len(uph_state.project.sample_data))
		return 0.0;

	const Uph_SampleData *data = &uph_state.project.sample_data[sample->data_index];
	return ceil(uph_frames_to_beats(data->frame_count, uph_state.settings.audio.sample_rate, uph_state.project.bpm));
}

static double _uph_song_timeline_drop_beat(Leaf_BoundingBox bbox)
{
	const double beat = ((double)naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
	return uph_snap_beat_floor(fmax(beat, 0.0), uph_song_timeline_data.snap_resolution, uph_song_timeline_data.zoom.x);
}

static void _uph_song_timeline_place_dropped_block(Leaf_BoundingBox bbox, Uph_Track *track, Uph_ResourceType type, Uph_ResourceIndex index)
{
	uph_state.shared.selected_resource = (Uph_SelectedResourceState){ .type = type, .index = index };
	uph_action_refresh_block_defaults();
	uph_block_select_all(false);

	Uph_ActionBlockCreate data = {
		.track = uph_action_track_ref(track),
		.block_index = (uint32_t)naui_list_len(track->blocks),
		.block = uph_song_timeline_block_create(_uph_song_timeline_drop_beat(bbox), index, type)
	};
	naui_action_execute_stack(UPH_ACTION_BLOCK_CREATE, data);
}

static void _uph_song_timeline_render_drop_preview(Leaf_BoundingBox bbox, double length_beats)
{
	const Leaf_Color accent = naui_theme_color("naui_panel_tab_accent_color");
	const float zoom = uph_song_timeline_data.zoom.x;
	const float x = bbox.x + (float)_uph_song_timeline_drop_beat(bbox) * zoom - uph_song_timeline_data.scroll.x;
	const float width = length_beats > 0.0 ? (float)length_beats * zoom : NAUI_DPI(2.0f);

	naui_fill_rect((Naui_Vec2){ bbox.x, bbox.y }, (Naui_Vec2){ bbox.width, bbox.height }, leaf_rgba(accent.r, accent.g, accent.b, 24), 0, NAUI_CORNER_NONE);
	naui_fill_rect((Naui_Vec2){ x, bbox.y + 1.0f }, (Naui_Vec2){ width, bbox.height - 2.0f }, leaf_rgba(accent.r, accent.g, accent.b, 110), NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL);
}

static void _uph_song_timeline_update_drop_target(Leaf_BoundingBox bbox, Uph_Track *track)
{
	if (!naui_panel_is_hovered(uph_state.panels.song_timeline))
		return;

	if (_uph_song_timeline_drop_compatible(track, UPH_RESOURCE_SAMPLE))
	{
		if (uph_drag_hover(UPH_DRAG_TYPE_SAMPLE_FILE))
			_uph_song_timeline_render_drop_preview(bbox, 0.0);

		const Naui_Path *file = uph_drag_drop(UPH_DRAG_TYPE_SAMPLE_FILE);
		Uph_ResourceIndex sample_index;
		if (file && uph_sample_browser_add_to_project(*file, &sample_index))
			_uph_song_timeline_place_dropped_block(bbox, track, UPH_RESOURCE_SAMPLE, sample_index);
	}

	const Uph_DragResource *hovering = uph_drag_hover(UPH_DRAG_TYPE_RESOURCE);
	if (hovering && _uph_song_timeline_resource_valid(hovering) && _uph_song_timeline_drop_compatible(track, hovering->type))
		_uph_song_timeline_render_drop_preview(bbox, _uph_song_timeline_resource_length(hovering));

	const Uph_DragResource *dropped = uph_drag_drop(UPH_DRAG_TYPE_RESOURCE);
	if (dropped && _uph_song_timeline_resource_valid(dropped) && _uph_song_timeline_drop_compatible(track, dropped->type))
		_uph_song_timeline_place_dropped_block(bbox, track, dropped->type, dropped->index);
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
	naui_fill_rect(
		(Naui_Vec2){ bbox.x, bbox.y + bbox.height - 1.0f },
		(Naui_Vec2){ bbox.width, 1.0f },
		naui_theme_color("uph_timeline_row_border_color"),
		0.0f,
		NAUI_CORNER_NONE
	);

	if (mouse_over_track)
	{
		_uph_song_timeline_update_drop_target(bbox, track);
		track = uph_action_track_resolve(*track_ref);
		if (!track)
		{
			naui_pop_clip_rect();
			return;
		}
	}

	_uph_song_timeline_update_track_timeline_drag(bbox, track);
	uph_song_timeline_update_track_action_input(bbox, track);

	track = uph_action_track_resolve(*track_ref);
	if (!track)
	{
		naui_pop_clip_rect();
		return;
	}

	_uph_song_timeline_update_marquee(bbox, track);

	naui_pop_clip_rect();
}

static bool _uph_song_timeline_row_expanded(const Uph_Track *track)
{
	return naui_list_len(track->subtracks) > 0 && !(track->type == UPH_RESOURCE_GROUP && (track->state & UPH_TRACK_TIMELINE_COLLAPSED));
}

static Naui_List(Uph_Track) _uph_song_timeline_row_siblings(const Uph_Track *track)
{
	return track->parent ? track->parent->subtracks : uph_state.project.tracks;
}

static const Uph_Track *_uph_song_timeline_previous_row(const Uph_Track *track)
{
	if (track->index == 0)
		return track->parent;

	const Uph_Track *row = &_uph_song_timeline_row_siblings(track)[track->index - 1];
	while (_uph_song_timeline_row_expanded(row))
		row = &row->subtracks[naui_list_len(row->subtracks) - 1];
	return row;
}

static const Uph_Track *_uph_song_timeline_next_row(const Uph_Track *track)
{
	if (_uph_song_timeline_row_expanded(track))
		return &track->subtracks[0];

	for (const Uph_Track *row = track; row; row = row->parent)
	{
		Naui_List(Uph_Track) siblings = _uph_song_timeline_row_siblings(row);
		if (row->index + 1 < (uint32_t)naui_list_len(siblings))
			return &siblings[row->index + 1];
	}
	return NULL;
}

static bool _uph_song_timeline_row_has_timeline(const Uph_Track *track)
{
	return track && track->type != UPH_RESOURCE_GROUP;
}

void uph_song_timeline_render_track_timeline(Uph_Track *track)
{
	const uint32_t row_counter = uph_song_timeline_data.visual.row_counter;
	const Leaf_Color bg_color = row_counter & 1 ? naui_theme_color("uph_timeline_row_bg1_color") : naui_theme_color("uph_timeline_row_bg2_color");
	const Leaf_Color border_color = naui_theme_color("uph_song_timeline_border_color");

	const float row_height = NAUI_DPI(uph_song_timeline_data.zoom.y);
	const float row_y = uph_song_timeline_data.panel_bounding_box.y - uph_song_timeline_data.scroll.y + uph_song_timeline_data.visual.row_y;

	Uph_ActionTrackRef track_ref = uph_action_track_ref(track);
	const bool row_visible = row_y + row_height >= uph_song_timeline_data.panel_bounding_box.y && row_y <= uph_song_timeline_data.panel_bounding_box.y + uph_song_timeline_data.panel_bounding_box.height;

	leaf({
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FULL },
		.color = bg_color,
		.custom_draw_data = row_visible ? LEAF_DATA_SLICE(track_ref) : (Leaf_DataSlice){ 0 },
		.custom_draw = row_visible ? (Leaf_CustomDrawFn)_uph_song_timeline_render_track_timeline_overlay : NULL,
		.border = {
			.width = 1,
			.color = {border_color},
			.sides = LEAF_SIDE_LEFT | LEAF_SIDE_RIGHT |
				(_uph_song_timeline_row_has_timeline(_uph_song_timeline_previous_row(track)) ? 0 : LEAF_SIDE_TOP) |
				(_uph_song_timeline_row_has_timeline(_uph_song_timeline_next_row(track)) ? 0 : LEAF_SIDE_BOTTOM)
		},
		.clip_children = true
	})
	{
		if (row_visible)
			uph_song_timeline_render_track_timeline_blocks(track);
	}
}
