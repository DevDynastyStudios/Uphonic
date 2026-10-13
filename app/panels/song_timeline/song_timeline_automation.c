#pragma region Automation Point Editing

static double _uph_song_timeline_automation_mouse_beat(Naui_Vec2 position, double start_offset)
{
	const double raw = ((double)naui_mouse_x() - position.x) / uph_song_timeline_data.zoom.x + start_offset;
	return uph_snap_beat_round(raw, uph_song_timeline_data.snap_resolution);
}

static void _uph_song_timeline_finish_point_edit(const Uph_Automation *automation)
{
	Uph_AutomationEditState *edit = &uph_song_timeline_data.automation_edit;
	const int32_t index = edit->dragging_point_index;
	edit->dragging_point_index = -1;

	if (index < 0 || (uint32_t)index >= (uint32_t)naui_list_len(automation->points))
	{
		edit->creating = false;
		return;
	}

	const Uph_AutomationPoint point = automation->points[index];
	if (edit->creating)
	{
		Uph_ActionAutomationPointCreate data = {
			.automation_index = edit->automation_index,
			.point_index = (uint32_t)index,
			.point = point,
			.applied_live = true
		};
		naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_CREATE, data);
	}
	else if (point.beat != edit->initial_point.beat || point.value != edit->initial_point.value)
	{
		Uph_ActionAutomationPointMove data = {
			.automation_index = edit->automation_index,
			.point_index = (uint32_t)index,
			.old_point = edit->initial_point,
			.new_point = point,
			.applied_live = true
		};
		naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_MOVE, data);
	}

	edit->creating = false;
}

void uph_song_timeline_update_automation_point_drag(Naui_Vec2 position, Naui_Vec2 size, Uph_Track *track, uint32_t block_index, double start_offset, Uph_ResourceIndex automation_index)
{
	Uph_AutomationEditState *edit = &uph_song_timeline_data.automation_edit;
	Uph_Automation *automation = &uph_state.project.automations[automation_index];
	const float zoom_x = uph_song_timeline_data.zoom.x;
	const float scroll_x = uph_song_timeline_data.scroll.x;
	const int32_t point_size = NAUI_DPI(6);
	const int32_t hit_radius = point_size;
	const bool is_target_block = (edit->track == track && edit->block_index == block_index);

	if (is_target_block && edit->dragging_point_index >= 0)
	{
		const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
		const int32_t idx = edit->dragging_point_index;
		if ((uint32_t)idx >= point_count)
		{
			edit->dragging_point_index = -1;
			edit->creating = false;
			return;
		}

		Uph_AutomationPoint *point = &automation->points[idx];
		const float mouse_y = (float)naui_mouse_y();
		const float clamped_y = fminf(fmaxf(mouse_y, position.y), position.y + size.y);
		point->value = 1.0f - (double)((clamped_y - position.y) / size.y);
		point->value = fmax(0.0, fmin(1.0, point->value));

		if (idx != 0)
		{
			double snapped_beat = _uph_song_timeline_automation_mouse_beat(position, start_offset);
			const double prev_beat = automation->points[idx - 1].beat;
			snapped_beat = fmax(snapped_beat, prev_beat);

			if ((uint32_t)idx + 1 < point_count)
				snapped_beat = fmin(snapped_beat, automation->points[idx + 1].beat);

			point->beat = snapped_beat;
		}

		naui_set_cursor(NAUI_CURSOR_HAND);

		if (naui_mouse_released(NAUI_MOUSE_LEFT))
			_uph_song_timeline_finish_point_edit(automation);

		return;
	}

	const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
	bool hovering_existing_point = false;
	for (uint32_t i = 0; i < point_count; i++)
	{
		Uph_AutomationPoint *point = &automation->points[i];
		const float px = position.x + (float)((point->beat - start_offset) * zoom_x);
		const float py = position.y + size.y - (float)(point->value * size.y);
		const float dx = (float)naui_mouse_x() - px;
		const float dy = (float)naui_mouse_y() - py;

		if (dx * dx + dy * dy <= (float)(hit_radius * hit_radius))
		{
			hovering_existing_point = true;
			naui_set_cursor(NAUI_CURSOR_HAND);

			if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			{
				edit->track = track;
				edit->block_index = block_index;
				edit->automation_index = automation_index;
				edit->initial_point = *point;
				edit->creating = false;
				edit->dragging_point_index = (int32_t)i;
			}
			else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
			{
				Uph_ActionAutomationPointDelete data = { .automation_index = automation_index, .point_index = i };
				naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_DELETE, data);
				break;
			}

			break;
		}
	}

	bool in_bounds = uph_song_timeline_vec4_contains_vec2((Naui_Vec4) { position.x, position.y, size.x, size.y }, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() });
	if (!hovering_existing_point && naui_mouse_pressed(NAUI_MOUSE_LEFT) && in_bounds)
	{
		double new_beat = _uph_song_timeline_automation_mouse_beat(position, start_offset);
		new_beat = fmax(new_beat, 0.0);
		const float mouse_y = (float)naui_mouse_y();
		const float clamped_y = fminf(fmaxf(mouse_y, position.y), position.y + size.y);
		double new_value = 1.0f - (double)((clamped_y - position.y) / size.y);
		new_value = fmax(0.0, fmin(1.0, new_value));

		uint32_t insert_index = point_count;
		for (uint32_t i = 0; i < point_count; i++)
		{
			if (new_beat < automation->points[i].beat)
			{
				insert_index = i;
				break;
			}
		}

		Uph_AutomationPoint new_point = { .beat = new_beat, .value = new_value };
		naui_list_insert(automation->points, new_point, insert_index);
		edit->track = track;
		edit->block_index = block_index;
		edit->automation_index = automation_index;
		edit->initial_point = new_point;
		edit->creating = true;
		edit->dragging_point_index = (int32_t)insert_index;
	}
}

#pragma endregion
