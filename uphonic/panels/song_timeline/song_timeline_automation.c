#pragma region Automation Point Editing

#define UPH_SONG_TIMELINE_AUTOMATION_HANDLE_SIZE		6
#define UPH_SONG_TIMELINE_AUTOMATION_HANDLE_MIN_SPAN	18
#define UPH_SONG_TIMELINE_AUTOMATION_FINE_SCALE		0.1f

static double uph__song_timeline_automation_beat_at(float x, Naui_Vec2 position, double start_offset)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const double raw = ((double)x - position.x) / state->zoom.x + start_offset;
	return uph_snap_beat_round(raw, state->snap_resolution, state->zoom.x);
}

static bool uph__song_timeline_automation_handle_position(const Uph_Automation *automation, uint32_t segment, Naui_Vec2 position, Naui_Vec2 size, double start_offset, Naui_Vec2 *out)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	if (segment + 1 >= (uint32_t)naui_list_len(automation->points))
		return false;

	const Uph_AutomationPoint *from = &automation->points[segment];
	const Uph_AutomationPoint *to = &automation->points[segment + 1];
	if (from->curve == UPH_AUTOMATION_CURVE_STEP)
		return false;

	const float zoom_x = state->zoom.x;
	if ((float)(to->beat - from->beat) * zoom_x < NAUI_DPI(UPH_SONG_TIMELINE_AUTOMATION_HANDLE_MIN_SPAN))
		return false;

	const double mid_beat = (from->beat + to->beat) * 0.5;
	*out = (Naui_Vec2) {
		position.x + (float)((mid_beat - start_offset) * zoom_x),
		position.y + size.y - uph_automation_segment_value(from, to, mid_beat) * size.y
	};
	return true;
}

static void uph__song_timeline_finish_point_edit(const Uph_Automation *automation)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_AutomationEditState *edit = &state->automation_edit;
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
	else if (point.beat != edit->initial_point.beat || point.value != edit->initial_point.value || point.tension != edit->initial_point.tension)
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
	edit->dragging_tension = false;
}

static void uph__song_timeline_update_tension_drag(Naui_Vec2 size, Uph_Automation *automation, Uph_ResourceIndex automation_index)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_AutomationEditState *edit = &state->automation_edit;
	const int32_t idx = edit->dragging_point_index;
	if ((uint32_t)idx + 1 >= (uint32_t)naui_list_len(automation->points))
	{
		edit->dragging_point_index = -1;
		edit->dragging_tension = false;
		return;
	}

	Uph_AutomationPoint *point = &automation->points[idx];
	const Uph_AutomationPoint *next = &automation->points[idx + 1];
	const float direction = (point->curve == UPH_AUTOMATION_CURVE_SINGLE && next->value < point->value) ? -1.0f : 1.0f;
	const float scale = uph_ui_shift_down() ? UPH_SONG_TIMELINE_AUTOMATION_FINE_SCALE : 1.0f;
	edit->tension_drag_offset += ((float)naui_mouse_y() - edit->tension_last_mouse_y) * scale;
	edit->tension_last_mouse_y = (float)naui_mouse_y();
	point->tension = NAUI_CLAMP(edit->initial_point.tension - edit->tension_drag_offset / (fmaxf(size.y, 1.0f) * 0.5f) * direction, -1.0f, 1.0f);

	edit->hovered_automation_index = automation_index;
	edit->hovered_handle_index = idx;
	naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

	if (naui_mouse_down(NAUI_MOUSE_LEFT))
		return;

	if (naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
		point->tension = 0.0f;

	uph__song_timeline_finish_point_edit(automation);
}

void uph_song_timeline_update_automation_point_drag(Naui_Vec2 position, Naui_Vec2 size, Uph_Track *track, uint32_t block_index, double start_offset, Uph_ResourceIndex automation_index)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_AutomationEditState *edit = &state->automation_edit;
	Uph_Automation *automation = &uph_state.project.automations[automation_index];
	const float zoom_x = state->zoom.x;
	const int32_t hit_radius = NAUI_DPI(10);
	const bool is_target_block = (edit->track == track && edit->block_index == block_index);

	if (is_target_block && edit->dragging_point_index >= 0 && edit->dragging_tension)
	{
		uph__song_timeline_update_tension_drag(size, automation, automation_index);
		return;
	}

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
		const float target_x = (float)naui_mouse_x() - edit->grab_offset.x;
		const float target_y = (float)naui_mouse_y() - edit->grab_offset.y;
		const float clamped_y = fminf(fmaxf(target_y, position.y), position.y + size.y);
		point->value = 1.0f - (double)((clamped_y - position.y) / size.y);
		point->value = fmax(0.0, fmin(1.0, point->value));

		if (idx != 0)
		{
			double snapped_beat = uph__song_timeline_automation_beat_at(target_x, position, start_offset);
			const double prev_beat = automation->points[idx - 1].beat;
			snapped_beat = fmax(snapped_beat, prev_beat);

			if ((uint32_t)idx + 1 < point_count)
				snapped_beat = fmin(snapped_beat, automation->points[idx + 1].beat);

			point->beat = snapped_beat;
		}

		edit->hovered_automation_index = automation_index;
		edit->hovered_point_index = idx;
		naui_set_cursor(NAUI_CURSOR_HAND);

		if (naui_mouse_released(NAUI_MOUSE_LEFT))
			uph__song_timeline_finish_point_edit(automation);

		return;
	}

	const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();
	int32_t hovered_index = -1;
	Naui_Vec2 hovered_position = { 0 };
	float best_distance = (float)(hit_radius * hit_radius);
	for (uint32_t i = 0; i < point_count; i++)
	{
		const Uph_AutomationPoint *point = &automation->points[i];
		const float px = position.x + (float)((point->beat - start_offset) * zoom_x);
		const float py = position.y + size.y - (float)(point->value * size.y);
		const float dx = mouse_x - px;
		const float dy = mouse_y - py;
		const float distance = dx * dx + dy * dy;

		if (distance <= best_distance)
		{
			best_distance = distance;
			hovered_index = (int32_t)i;
			hovered_position = (Naui_Vec2) { px, py };
		}
	}

	int32_t hovered_handle = -1;
	if (hovered_index < 0)
	{
		best_distance = (float)(hit_radius * hit_radius);
		for (uint32_t i = 0; i + 1 < point_count; i++)
		{
			Naui_Vec2 handle;
			if (!uph__song_timeline_automation_handle_position(automation, i, position, size, start_offset, &handle))
				continue;

			const float dx = mouse_x - handle.x;
			const float dy = mouse_y - handle.y;
			const float distance = dx * dx + dy * dy;
			if (distance <= best_distance)
			{
				best_distance = distance;
				hovered_handle = (int32_t)i;
			}
		}
	}

	edit->hovered_automation_index = automation_index;
	if (hovered_handle >= 0)
	{
		edit->hovered_handle_index = hovered_handle;
		naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		{
			edit->track = track;
			edit->block_index = block_index;
			edit->automation_index = automation_index;
			edit->initial_point = automation->points[hovered_handle];
			edit->tension_drag_offset = 0.0f;
			edit->tension_last_mouse_y = mouse_y;
			edit->creating = false;
			edit->dragging_tension = true;
			edit->dragging_point_index = hovered_handle;
		}

		return;
	}

	const bool hovering_existing_point = hovered_index >= 0;
	if (hovering_existing_point)
	{
		edit->hovered_point_index = hovered_index;
		naui_set_cursor(NAUI_CURSOR_HAND);

		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		{
			edit->track = track;
			edit->block_index = block_index;
			edit->automation_index = automation_index;
			edit->initial_point = automation->points[hovered_index];
			edit->grab_offset = (Naui_Vec2) { mouse_x - hovered_position.x, mouse_y - hovered_position.y };
			edit->creating = false;
			edit->dragging_tension = false;
			edit->dragging_point_index = hovered_index;
		}
		else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
		{
			Uph_ActionAutomationPointDelete data = { .automation_index = automation_index, .point_index = (uint32_t)hovered_index };
			naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_DELETE, data);
		}
	}

	bool in_bounds = uph_song_timeline_vec4_contains_vec2((Naui_Vec4) { position.x, position.y, size.x, size.y }, (Naui_Vec2) { mouse_x, mouse_y });
	if (!hovering_existing_point && naui_mouse_pressed(NAUI_MOUSE_LEFT) && in_bounds)
	{
		double new_beat = uph__song_timeline_automation_beat_at(mouse_x, position, start_offset);
		new_beat = fmax(new_beat, 0.0);
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
		if (insert_index > 0)
			new_point.curve = automation->points[insert_index - 1].curve;

		naui_list_insert(automation->points, new_point, insert_index);
		edit->track = track;
		edit->block_index = block_index;
		edit->automation_index = automation_index;
		edit->initial_point = new_point;
		edit->grab_offset = (Naui_Vec2) { 0 };
		edit->creating = true;
		edit->dragging_tension = false;
		edit->dragging_point_index = (int32_t)insert_index;
	}
}

#pragma endregion
