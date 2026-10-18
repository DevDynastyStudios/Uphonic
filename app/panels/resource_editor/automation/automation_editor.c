#define UPH_AUTOMATION_EDITOR_RULER_HEIGHT		24
#define UPH_AUTOMATION_EDITOR_GUTTER_WIDTH		44.0f
#define UPH_AUTOMATION_EDITOR_LANE_PADDING		14.0f
#define UPH_AUTOMATION_EDITOR_TOOLBAR_FIELD_GAP	16

#define UPH_AUTOMATION_EDITOR_POINT_SIZE		8.0f
#define UPH_AUTOMATION_EDITOR_HANDLE_SIZE		7.0f
#define UPH_AUTOMATION_EDITOR_HIT_RADIUS		8.0f
#define UPH_AUTOMATION_EDITOR_LINE_WIDTH		2.0f

#define UPH_AUTOMATION_EDITOR_ZOOM_MIN			4.0f
#define UPH_AUTOMATION_EDITOR_ZOOM_MAX			2048.0f
#define UPH_AUTOMATION_EDITOR_ZOOM_SPEED		0.1f
#define UPH_AUTOMATION_EDITOR_SCROLL_SPEED		40.0f

#define UPH_AUTOMATION_EDITOR_FINE_SCALE		0.1f
#define UPH_AUTOMATION_EDITOR_NUDGE				0.01f
#define UPH_AUTOMATION_EDITOR_FINE_NUDGE		0.001f
#define UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS	4

typedef uint8_t Uph_AutomationDragMode;
enum
{
	UPH_AUTOMATION_DRAG_NONE,
	UPH_AUTOMATION_DRAG_POINT,
	UPH_AUTOMATION_DRAG_TENSION
};

typedef struct
{
	Uph_MidiView view;
	Leaf_BoundingBox lane_box;
	Uph_SnapResolution snap_resolution;

	Uph_ResourceIndex automation_index;
	bool has_automation;
	bool fit;

	int32_t selected_point;
	int32_t hovered_point;
	int32_t hovered_handle;
	int32_t menu_segment;
	int32_t menu_point;

	float clipboard_value;
	bool has_clipboard;

	Naui_String entry_text;
	int32_t entry_index;
	bool entry_active;

	Uph_AutomationDragMode drag_mode;
	int32_t drag_index;
	Uph_AutomationPoint drag_initial;
	Naui_Vec2 drag_last_mouse;
	Naui_Vec2 drag_offset;
	bool drag_creating;

	int32_t field_index;
	Uph_AutomationPoint field_initial;
	bool field_editing;

	Naui_Vec2 pan_last_mouse;
	bool panning;
	bool lane_hovered;
	bool panel_hovered;
}
Uph_AutomationEditorData;

static Uph_AutomationEditorData uph_automation_editor_data;

static const char *_uph_automation_editor_curve_names[UPH_AUTOMATION_CURVE_COUNT] = {
	"Curve",
	"S-Curve",
	"Step"
};

#pragma region Helpers

static inline bool _uph_automation_editor_fine(void)
{
	return uph_ui_shift_down();
}

static inline bool _uph_automation_editor_snap_disabled(void)
{
	return uph_ui_shift_down() || naui_key_down(NAUI_KEY_LALT) || naui_key_down(NAUI_KEY_RALT);
}

static inline float _uph_automation_editor_value_top(Leaf_BoundingBox box)
{
	return box.y + NAUI_DPI(UPH_AUTOMATION_EDITOR_LANE_PADDING);
}

static inline float _uph_automation_editor_value_height(Leaf_BoundingBox box)
{
	return fmaxf(1.0f, box.height - NAUI_DPI(UPH_AUTOMATION_EDITOR_LANE_PADDING) * 2.0f);
}

static inline float _uph_automation_editor_value_to_y(Leaf_BoundingBox box, float value)
{
	return _uph_automation_editor_value_top(box) + (1.0f - value) * _uph_automation_editor_value_height(box);
}

static inline float _uph_automation_editor_y_to_value(Leaf_BoundingBox box, float y)
{
	return NAUI_CLAMP01(1.0f - (y - _uph_automation_editor_value_top(box)) / _uph_automation_editor_value_height(box));
}

static inline Naui_Vec2 _uph_automation_editor_point_position(const Uph_AutomationEditorData *editor, Leaf_BoundingBox box, const Uph_AutomationPoint *point)
{
	return (Naui_Vec2) {
		uph_midi_view_beat_to_x(&editor->view, box, point->beat),
		_uph_automation_editor_value_to_y(box, point->value)
	};
}

static Uph_Automation *_uph_automation_editor_current(const Uph_AutomationEditorData *editor)
{
	if (!editor->has_automation || editor->automation_index >= (Uph_ResourceIndex)naui_list_len(uph_state.project.automations))
		return NULL;

	return &uph_state.project.automations[editor->automation_index];
}

static bool _uph_automation_editor_handle_position(const Uph_AutomationEditorData *editor, Leaf_BoundingBox box, const Uph_Automation *automation, uint32_t segment, Naui_Vec2 *out)
{
	if (segment + 1 >= (uint32_t)naui_list_len(automation->points))
		return false;

	const Uph_AutomationPoint *from = &automation->points[segment];
	const Uph_AutomationPoint *to = &automation->points[segment + 1];
	if (from->curve == UPH_AUTOMATION_CURVE_STEP)
		return false;

	const float x0 = uph_midi_view_beat_to_x(&editor->view, box, from->beat);
	const float x1 = uph_midi_view_beat_to_x(&editor->view, box, to->beat);
	if (x1 - x0 < NAUI_DPI(UPH_AUTOMATION_EDITOR_HANDLE_SIZE * 3.0f))
		return false;

	const double mid_beat = (from->beat + to->beat) * 0.5;
	*out = (Naui_Vec2) {
		uph_midi_view_beat_to_x(&editor->view, box, mid_beat),
		_uph_automation_editor_value_to_y(box, uph_automation_segment_value(from, to, mid_beat))
	};
	return true;
}

static int32_t _uph_automation_editor_segment_at(const Uph_Automation *automation, double beat)
{
	const uint32_t count = (uint32_t)naui_list_len(automation->points);
	for (uint32_t i = 0; i + 1 < count; i++)
	{
		if (beat >= automation->points[i].beat && beat < automation->points[i + 1].beat)
			return (int32_t)i;
	}
	return -1;
}

static double _uph_automation_editor_clamp_beat(const Uph_Automation *automation, uint32_t index, double beat)
{
	if (index == 0)
		return automation->points[0].beat;

	beat = fmax(beat, automation->points[index - 1].beat);
	if (index + 1 < (uint32_t)naui_list_len(automation->points))
		beat = fmin(beat, automation->points[index + 1].beat);

	return fmax(beat, 0.0);
}

static double _uph_automation_editor_snap(const Uph_AutomationEditorData *editor, double beat)
{
	if (_uph_automation_editor_snap_disabled())
		return beat;

	return uph_snap_beat_round(beat, editor->snap_resolution);
}

static bool _uph_automation_editor_fields_text_active(void)
{
	return uph_ui_textfield_active(leaf_id("uph_automation_editor_beat"))
		|| uph_ui_textfield_active(leaf_id("uph_automation_editor_value"))
		|| uph_ui_textfield_active(leaf_id("uph_automation_editor_tension"));
}

#pragma endregion

#pragma region Edits

static void _uph_automation_editor_commit_move(Uph_ResourceIndex automation_index, uint32_t point_index, Uph_AutomationPoint old_point, Uph_AutomationPoint new_point)
{
	if (old_point.beat == new_point.beat && old_point.value == new_point.value && old_point.tension == new_point.tension && old_point.curve == new_point.curve)
		return;

	Uph_ActionAutomationPointMove data = {
		.automation_index = automation_index,
		.point_index = point_index,
		.old_point = old_point,
		.new_point = new_point,
		.applied_live = true
	};
	naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_MOVE, data);
}

static void _uph_automation_editor_delete_point(Uph_AutomationEditorData *editor, uint32_t index)
{
	Uph_ActionAutomationPointDelete data = { .automation_index = editor->automation_index, .point_index = index };
	naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_DELETE, data);

	if (editor->selected_point == (int32_t)index)
		editor->selected_point = -1;
	else if (editor->selected_point > (int32_t)index)
		editor->selected_point--;

	editor->hovered_point = -1;
	editor->hovered_handle = -1;
}

static void _uph_automation_editor_set_curve(Uph_AutomationEditorData *editor, Uph_Automation *automation, uint32_t index, Uph_AutomationCurve curve, float tension)
{
	Uph_AutomationPoint *point = &automation->points[index];
	const Uph_AutomationPoint old_point = *point;
	point->curve = curve;
	point->tension = tension;
	_uph_automation_editor_commit_move(editor->automation_index, index, old_point, *point);
}

static void _uph_automation_editor_set_value(Uph_AutomationEditorData *editor, Uph_Automation *automation, uint32_t index, float value)
{
	Uph_AutomationPoint *point = &automation->points[index];
	const Uph_AutomationPoint old_point = *point;
	point->value = NAUI_CLAMP01(value);
	_uph_automation_editor_commit_move(editor->automation_index, index, old_point, *point);
}

static void _uph_automation_editor_open_entry(Uph_AutomationEditorData *editor, uint32_t index)
{
	editor->entry_active = true;
	editor->entry_index = (int32_t)index;
	editor->entry_text = naui_string_from_cstr("");
}

static void _uph_automation_editor_close_entry(Uph_AutomationEditorData *editor)
{
	editor->entry_active = false;
	editor->entry_index = -1;
}

static void _uph_automation_editor_begin_drag(Uph_AutomationEditorData *editor, const Uph_Automation *automation, uint32_t index, Uph_AutomationDragMode mode, bool creating)
{
	editor->drag_mode = mode;
	editor->drag_index = (int32_t)index;
	editor->drag_initial = automation->points[index];
	editor->drag_creating = creating;
	editor->drag_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
	editor->drag_offset = (Naui_Vec2) { 0.0f, 0.0f };
}

static void _uph_automation_editor_finish_drag(Uph_AutomationEditorData *editor)
{
	const Uph_AutomationDragMode mode = editor->drag_mode;
	const int32_t index = editor->drag_index;
	const bool creating = editor->drag_creating;
	editor->drag_mode = UPH_AUTOMATION_DRAG_NONE;
	editor->drag_index = -1;
	editor->drag_creating = false;

	Uph_Automation *automation = _uph_automation_editor_current(editor);
	if (mode == UPH_AUTOMATION_DRAG_NONE || !automation || index < 0 || (uint32_t)index >= (uint32_t)naui_list_len(automation->points))
		return;

	const Uph_AutomationPoint point = automation->points[index];
	if (creating)
	{
		Uph_ActionAutomationPointCreate data = {
			.automation_index = editor->automation_index,
			.point_index = (uint32_t)index,
			.point = point,
			.applied_live = true
		};
		naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_CREATE, data);
		return;
	}

	_uph_automation_editor_commit_move(editor->automation_index, (uint32_t)index, editor->drag_initial, point);
}

static void _uph_automation_editor_commit_fields(Uph_AutomationEditorData *editor)
{
	if (!editor->field_editing)
		return;

	editor->field_editing = false;
	Uph_Automation *automation = _uph_automation_editor_current(editor);
	if (!automation || editor->field_index < 0 || (uint32_t)editor->field_index >= (uint32_t)naui_list_len(automation->points))
		return;

	_uph_automation_editor_commit_move(editor->automation_index, (uint32_t)editor->field_index, editor->field_initial, automation->points[editor->field_index]);
}

#pragma endregion

#pragma region Input

static void _uph_automation_editor_update_view(Uph_AutomationEditorData *editor, const Uph_Automation *automation)
{
	Uph_MidiView *view = &editor->view;
	const Leaf_BoundingBox box = editor->lane_box;
	if (box.width < 1.0f)
		return;

	const float zoom_min = NAUI_DPI(UPH_AUTOMATION_EDITOR_ZOOM_MIN);
	const float zoom_max = NAUI_DPI(UPH_AUTOMATION_EDITOR_ZOOM_MAX);

	if (editor->fit)
	{
		const double length = fmax(uph_calculate_automation_length(automation), 1.0);
		view->zoom.x = NAUI_CLAMP((float)((double)box.width * 0.95 / length), zoom_min, zoom_max);
		view->scroll.x = 0.0f;
		editor->fit = false;
	}

	const float wheel = (float)naui_mouse_scroll_delta();
	if (editor->lane_hovered && wheel != 0.0f)
	{
		if (uph_ui_ctrl_down())
		{
			const float mouse_x = (float)naui_mouse_x();
			const double anchor = uph_midi_view_x_to_beat(view, box, mouse_x);
			view->zoom.x = NAUI_CLAMP(view->zoom.x * (1.0f + wheel * UPH_AUTOMATION_EDITOR_ZOOM_SPEED), zoom_min, zoom_max);
			view->scroll.x = (float)(anchor * view->zoom.x) - (mouse_x - box.x);
		}
		else
		{
			view->scroll.x -= wheel * NAUI_DPI(UPH_AUTOMATION_EDITOR_SCROLL_SPEED);
		}
	}

	if (editor->lane_hovered && naui_mouse_pressed(NAUI_MOUSE_MIDDLE))
	{
		editor->panning = true;
		editor->pan_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
	}

	if (editor->panning)
	{
		const Naui_Vec2 current = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
		view->scroll.x -= current.x - editor->pan_last_mouse.x;
		editor->pan_last_mouse = current;

		if (naui_mouse_released(NAUI_MOUSE_MIDDLE) || !naui_mouse_down(NAUI_MOUSE_MIDDLE))
			editor->panning = false;
	}

	view->scroll.x = fmaxf(0.0f, view->scroll.x);
}

static void _uph_automation_editor_update_hover(Uph_AutomationEditorData *editor, const Uph_Automation *automation)
{
	editor->hovered_point = -1;
	editor->hovered_handle = -1;
	if (!editor->lane_hovered || editor->drag_mode != UPH_AUTOMATION_DRAG_NONE)
		return;

	const Leaf_BoundingBox box = editor->lane_box;
	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };
	const float radius = NAUI_DPI(UPH_AUTOMATION_EDITOR_HIT_RADIUS);
	const uint32_t count = (uint32_t)naui_list_len(automation->points);

	float best = radius * radius;
	for (uint32_t i = 0; i < count; i++)
	{
		const Naui_Vec2 position = _uph_automation_editor_point_position(editor, box, &automation->points[i]);
		const float dx = mouse.x - position.x;
		const float dy = mouse.y - position.y;
		const float distance = dx * dx + dy * dy;
		if (distance <= best)
		{
			best = distance;
			editor->hovered_point = (int32_t)i;
		}
	}

	if (editor->hovered_point >= 0)
		return;

	best = radius * radius;
	for (uint32_t i = 0; i + 1 < count; i++)
	{
		Naui_Vec2 position;
		if (!_uph_automation_editor_handle_position(editor, box, automation, i, &position))
			continue;

		const float dx = mouse.x - position.x;
		const float dy = mouse.y - position.y;
		const float distance = dx * dx + dy * dy;
		if (distance <= best)
		{
			best = distance;
			editor->hovered_handle = (int32_t)i;
		}
	}
}

static void _uph_automation_editor_update_drag(Uph_AutomationEditorData *editor, Uph_Automation *automation)
{
	const uint32_t count = (uint32_t)naui_list_len(automation->points);
	const int32_t index = editor->drag_index;
	const bool valid = index >= 0 && (uint32_t)index < count && (editor->drag_mode != UPH_AUTOMATION_DRAG_TENSION || (uint32_t)index + 1 < count);
	if (!valid)
	{
		editor->drag_mode = UPH_AUTOMATION_DRAG_NONE;
		editor->drag_index = -1;
		editor->drag_creating = false;
		return;
	}

	const Leaf_BoundingBox box = editor->lane_box;
	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };
	const float scale = _uph_automation_editor_fine() ? UPH_AUTOMATION_EDITOR_FINE_SCALE : 1.0f;
	editor->drag_offset.x += (mouse.x - editor->drag_last_mouse.x) * scale;
	editor->drag_offset.y += (mouse.y - editor->drag_last_mouse.y) * scale;
	editor->drag_last_mouse = mouse;

	const float value_height = _uph_automation_editor_value_height(box);
	Uph_AutomationPoint *point = &automation->points[index];

	if (editor->drag_mode == UPH_AUTOMATION_DRAG_POINT)
	{
		point->value = NAUI_CLAMP01(editor->drag_initial.value - editor->drag_offset.y / value_height);
		const double beat = _uph_automation_editor_snap(editor, editor->drag_initial.beat + (double)editor->drag_offset.x / editor->view.zoom.x);
		point->beat = _uph_automation_editor_clamp_beat(automation, (uint32_t)index, beat);
		naui_set_cursor(NAUI_CURSOR_HAND);
	}
	else
	{
		const Uph_AutomationPoint *next = &automation->points[index + 1];
		const float direction = (point->curve == UPH_AUTOMATION_CURVE_SINGLE && next->value < point->value) ? -1.0f : 1.0f;
		point->tension = NAUI_CLAMP(editor->drag_initial.tension - editor->drag_offset.y / (value_height * 0.5f) * direction, -1.0f, 1.0f);
		naui_set_cursor(NAUI_CURSOR_RESIZE_NS);
	}

	if (naui_mouse_down(NAUI_MOUSE_LEFT))
		return;

	if (editor->drag_mode == UPH_AUTOMATION_DRAG_TENSION && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
		point->tension = 0.0f;

	_uph_automation_editor_finish_drag(editor);
}

static void _uph_automation_editor_update_mouse(Uph_AutomationEditorData *editor, Uph_Automation *automation, Uph_UIMenuID point_menu, Uph_UIMenuID segment_menu)
{
	if (editor->drag_mode != UPH_AUTOMATION_DRAG_NONE)
	{
		_uph_automation_editor_update_drag(editor, automation);
		return;
	}

	if (editor->entry_active)
	{
		const bool pressed = naui_mouse_pressed(NAUI_MOUSE_LEFT) || naui_mouse_pressed(NAUI_MOUSE_RIGHT);
		if (pressed && !leaf_hovered(leaf_id("uph_automation_editor_entry")))
			_uph_automation_editor_close_entry(editor);

		return;
	}

	if (!editor->lane_hovered || editor->panning || uph_ui_any_menu_open())
		return;

	const Leaf_BoundingBox box = editor->lane_box;
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();

	if (editor->hovered_point >= 0)
		naui_set_cursor(NAUI_CURSOR_HAND);
	else if (editor->hovered_handle >= 0)
		naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

	if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
	{
		if (editor->hovered_point >= 0)
		{
			editor->selected_point = editor->hovered_point;
			_uph_automation_editor_begin_drag(editor, automation, (uint32_t)editor->hovered_point, UPH_AUTOMATION_DRAG_POINT, false);
			return;
		}

		if (editor->hovered_handle >= 0)
		{
			_uph_automation_editor_begin_drag(editor, automation, (uint32_t)editor->hovered_handle, UPH_AUTOMATION_DRAG_TENSION, false);
			return;
		}

		const uint32_t count = (uint32_t)naui_list_len(automation->points);
		const double beat = fmax(0.0, _uph_automation_editor_snap(editor, uph_midi_view_x_to_beat(&editor->view, box, mouse_x)));

		uint32_t insert_index = count;
		for (uint32_t i = 0; i < count; i++)
		{
			if (beat < automation->points[i].beat)
			{
				insert_index = i;
				break;
			}
		}

		Uph_AutomationPoint point = { .beat = beat, .value = _uph_automation_editor_y_to_value(box, mouse_y) };
		if (insert_index > 0)
			point.curve = automation->points[insert_index - 1].curve;

		naui_list_insert(automation->points, point, insert_index);
		editor->selected_point = (int32_t)insert_index;
		_uph_automation_editor_begin_drag(editor, automation, insert_index, UPH_AUTOMATION_DRAG_POINT, true);
		return;
	}

	if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
	{
		if (editor->hovered_point >= 0)
		{
			editor->selected_point = editor->hovered_point;
			editor->menu_point = editor->hovered_point;
			uph_ui_open_context_menu(point_menu);
			return;
		}

		const int32_t segment = editor->hovered_handle >= 0
			? editor->hovered_handle
			: _uph_automation_editor_segment_at(automation, uph_midi_view_x_to_beat(&editor->view, box, mouse_x));

		if (segment >= 0)
		{
			editor->menu_segment = segment;
			uph_ui_open_context_menu(segment_menu);
		}
	}
}

static void _uph_automation_editor_update_keys(Uph_AutomationEditorData *editor, Uph_Automation *automation)
{
	if (!editor->panel_hovered || editor->drag_mode != UPH_AUTOMATION_DRAG_NONE || editor->entry_active || _uph_automation_editor_fields_text_active())
		return;

	const int32_t index = editor->selected_point;
	if (index < 0 || (uint32_t)index >= (uint32_t)naui_list_len(automation->points))
		return;

	if (naui_key_pressed(NAUI_KEY_DELETE))
	{
		_uph_automation_editor_delete_point(editor, (uint32_t)index);
		return;
	}

	if (naui_key_pressed(NAUI_KEY_ESCAPE))
	{
		editor->selected_point = -1;
		return;
	}

	Uph_AutomationPoint *point = &automation->points[index];
	const Uph_AutomationPoint old_point = *point;
	const bool fine = _uph_automation_editor_fine();
	const float nudge = fine ? UPH_AUTOMATION_EDITOR_FINE_NUDGE : UPH_AUTOMATION_EDITOR_NUDGE;
	const double step = uph_snap_division(editor->snap_resolution) * (fine ? 0.25 : 1.0);

	if (naui_key_pressed_repeat(NAUI_KEY_UP))
		point->value = NAUI_CLAMP01(point->value + nudge);
	if (naui_key_pressed_repeat(NAUI_KEY_DOWN))
		point->value = NAUI_CLAMP01(point->value - nudge);
	if (naui_key_pressed_repeat(NAUI_KEY_RIGHT))
		point->beat = _uph_automation_editor_clamp_beat(automation, (uint32_t)index, point->beat + step);
	if (naui_key_pressed_repeat(NAUI_KEY_LEFT))
		point->beat = _uph_automation_editor_clamp_beat(automation, (uint32_t)index, point->beat - step);

	_uph_automation_editor_commit_move(editor->automation_index, (uint32_t)index, old_point, *point);
}

static void _uph_automation_editor_update_point_menu(Uph_AutomationEditorData *editor, Uph_Automation *automation, Uph_UIMenuID menu)
{
	const int32_t index = editor->menu_point;
	const bool valid = index >= 0 && (uint32_t)index < (uint32_t)naui_list_len(automation->points);

	if (uph_ui_menu_item(menu, "Remove", leaf_id("uph_automation_editor_point_remove")) && valid)
	{
		_uph_automation_editor_delete_point(editor, (uint32_t)index);
		return;
	}

	if (uph_ui_menu_item(menu, "Set value", leaf_id("uph_automation_editor_point_set")) && valid)
		_uph_automation_editor_open_entry(editor, (uint32_t)index);

	if (uph_ui_menu_item(menu, "Copy value", leaf_id("uph_automation_editor_point_copy")) && valid)
	{
		editor->clipboard_value = automation->points[index].value;
		editor->has_clipboard = true;
	}

	if (editor->has_clipboard)
	{
		char label[32];
		snprintf(label, sizeof(label), "Paste value (%.3f)", editor->clipboard_value);
		if (uph_ui_menu_item(menu, label, leaf_id("uph_automation_editor_point_paste")) && valid)
			_uph_automation_editor_set_value(editor, automation, (uint32_t)index, editor->clipboard_value);
	}
}

static void _uph_automation_editor_update_segment_menu(Uph_AutomationEditorData *editor, Uph_Automation *automation, Uph_UIMenuID menu)
{
	const int32_t segment = editor->menu_segment;
	const bool valid = segment >= 0 && (uint32_t)segment + 1 < (uint32_t)naui_list_len(automation->points);

	for (uint32_t c = 0; c < UPH_AUTOMATION_CURVE_COUNT; c++)
	{
		if (uph_ui_menu_item(menu, _uph_automation_editor_curve_names[c], leaf_id_indexed("uph_automation_editor_curve_item", c)) && valid)
			_uph_automation_editor_set_curve(editor, automation, (uint32_t)segment, (Uph_AutomationCurve)c, automation->points[segment].tension);
	}

	if (uph_ui_menu_item(menu, "Reset tension", leaf_id("uph_automation_editor_reset_tension")) && valid)
		_uph_automation_editor_set_curve(editor, automation, (uint32_t)segment, automation->points[segment].curve, 0.0f);
}

#pragma endregion

#pragma region Draw callbacks

static void _uph_automation_editor_ruler_draw(Leaf_BoundingBox box, void *user_data)
{
	uph_midi_grid_render_ruler(box, &uph_automation_editor_data.view, uph_automation_editor_data.snap_resolution);
}

static void _uph_automation_editor_gutter_draw(Leaf_BoundingBox box, void *user_data)
{
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_disabled_color");
	const Leaf_Color tick_color = naui_theme_color("uph_timeline_grid_beat_color");
	const float font_size = NAUI_DPI(11.0f);
	const float padding = NAUI_DPI(6.0f);

	for (uint32_t i = 0; i <= UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS; i++)
	{
		const float value = (float)i / (float)UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS;
		const float y = _uph_automation_editor_value_to_y(box, value);

		char label[16];
		snprintf(label, sizeof(label), "%.2f", value);
		const Naui_Vec2 text_size = naui_measure_text(label, (uint32_t)strlen(label), font_size, 0);

		naui_draw_line((Naui_Vec2) { box.x + box.width - padding * 0.5f, y }, (Naui_Vec2) { box.x + box.width, y }, tick_color, 1.0f);
		naui_draw_text((Naui_Vec2) { box.x + box.width - padding - text_size.x, y - text_size.y * 0.5f }, label, font_size, 0, text_color);
	}
}

static void _uph_automation_editor_draw_readout(Leaf_BoundingBox box, Naui_Vec2 anchor, const char *text)
{
	const float font_size = NAUI_DPI(12.0f);
	const float padding = NAUI_DPI(5.0f);
	const Naui_Vec2 text_size = naui_measure_text(text, (uint32_t)strlen(text), font_size, 0);
	const Naui_Vec2 size = { text_size.x + padding * 2.0f, text_size.y + padding * 2.0f };

	Naui_Vec2 position = { anchor.x + NAUI_DPI(10.0f), anchor.y - NAUI_DPI(10.0f) - size.y };
	if (position.x + size.x > box.x + box.width)
		position.x = anchor.x - NAUI_DPI(10.0f) - size.x;
	if (position.y < box.y)
		position.y = anchor.y + NAUI_DPI(10.0f);

	Leaf_Color background = naui_theme_color("uph_toolbox_bg_color");
	background.a = 230;
	naui_fill_rect(position, size, background, NAUI_DPI(3.0f), NAUI_CORNER_ALL);
	naui_draw_text((Naui_Vec2) { position.x + padding, position.y + padding }, text, font_size, 0, naui_theme_color("uph_ui_text_color"));
}

static void _uph_automation_editor_lane_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_AutomationEditorData *editor = &uph_automation_editor_data;
	const Uph_Automation *automation = _uph_automation_editor_current(editor);
	if (!automation)
		return;

	const Uph_MidiView *view = &editor->view;
	const uint32_t count = (uint32_t)naui_list_len(automation->points);
	const float value_top = _uph_automation_editor_value_top(box);
	const float value_height = _uph_automation_editor_value_height(box);

	Leaf_Color accent_color = naui_theme_color("uph_resource_list_item_fg_color");
	accent_color.a = 255;
	const Leaf_Color fill_color = { accent_color.r, accent_color.g, accent_color.b, 50 };
	const Leaf_Color dim_color = { accent_color.r, accent_color.g, accent_color.b, 110 };
	const Leaf_Color guide_color = naui_theme_color("uph_timeline_grid_subbeat_color");
	const Leaf_Color edge_color = naui_theme_color("uph_timeline_grid_beat_color");
	const Leaf_Color lane_color = naui_theme_color("uph_timeline_row_bg2_color");
	const Leaf_Color outline_color = naui_theme_color("uph_ui_text_color");

	naui_push_clip_rect(box.x, box.y, box.width, box.height);

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, naui_theme_color("uph_timeline_row_bg1_color"), 0.0f, NAUI_CORNER_NONE);
	naui_fill_rect((Naui_Vec2) { box.x, value_top }, (Naui_Vec2) { box.width, value_height }, lane_color, 0.0f, NAUI_CORNER_NONE);
	uph_midi_grid_render_beats(box, view, editor->snap_resolution);

	for (uint32_t i = 0; i <= UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS; i++)
	{
		const float y = _uph_automation_editor_value_to_y(box, (float)i / (float)UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS);
		const bool edge = i == 0 || i == UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS;
		naui_draw_line((Naui_Vec2) { box.x, y }, (Naui_Vec2) { box.x + box.width, y }, edge ? edge_color : guide_color, 1.0f);
	}

	const float end_x = uph_midi_view_beat_to_x(view, box, uph_calculate_automation_length(automation));
	if (end_x < box.x + box.width)
	{
		const float shade_x = fmaxf(end_x, box.x);
		naui_fill_rect((Naui_Vec2) { shade_x, box.y }, (Naui_Vec2) { box.x + box.width - shade_x, box.height }, leaf_rgba(0, 0, 0, 70), 0.0f, NAUI_CORNER_NONE);
	}

	const float line_width = NAUI_DPI(UPH_AUTOMATION_EDITOR_LINE_WIDTH);
	if (count > 0)
	{
		const Naui_Vec2 first = _uph_automation_editor_point_position(editor, box, &automation->points[0]);
		const Naui_Vec2 last = _uph_automation_editor_point_position(editor, box, &automation->points[count - 1]);
		if (first.x > box.x)
			naui_draw_line((Naui_Vec2) { box.x, first.y }, first, dim_color, 1.0f);
		if (last.x < box.x + box.width)
			naui_draw_line(last, (Naui_Vec2) { box.x + box.width, last.y }, dim_color, 1.0f);
	}

	uph_ui_automation_curve(automation, box.x - view->scroll.x, view->zoom.x, value_top, value_height, accent_color, fill_color, line_width);

	const float handle_size = NAUI_DPI(UPH_AUTOMATION_EDITOR_HANDLE_SIZE);
	for (uint32_t i = 0; i + 1 < count; i++)
	{
		Naui_Vec2 position;
		if (!_uph_automation_editor_handle_position(editor, box, automation, i, &position))
			continue;

		const bool active = editor->hovered_handle == (int32_t)i || (editor->drag_mode == UPH_AUTOMATION_DRAG_TENSION && editor->drag_index == (int32_t)i);
		const Naui_Vec2 handle_position = { position.x - handle_size * 0.5f, position.y - handle_size * 0.5f };
		naui_fill_rect(handle_position, (Naui_Vec2) { handle_size, handle_size }, active ? accent_color : lane_color, FLT_MAX, NAUI_CORNER_ALL);
		naui_draw_rect(handle_position, (Naui_Vec2) { handle_size, handle_size }, accent_color, NAUI_DPI(1.5f), FLT_MAX, NAUI_CORNER_ALL, NAUI_SIDE_ALL);
	}

	const float point_size = NAUI_DPI(UPH_AUTOMATION_EDITOR_POINT_SIZE);
	for (uint32_t i = 0; i < count; i++)
	{
		const Naui_Vec2 position = _uph_automation_editor_point_position(editor, box, &automation->points[i]);
		if (position.x < box.x - point_size || position.x > box.x + box.width + point_size)
			continue;

		const bool selected = editor->selected_point == (int32_t)i;
		const bool hovered = editor->hovered_point == (int32_t)i || (editor->drag_mode == UPH_AUTOMATION_DRAG_POINT && editor->drag_index == (int32_t)i);
		const float size = point_size + ((selected || hovered) ? NAUI_DPI(2.0f) : 0.0f);
		const Naui_Vec2 point_position = { position.x - size * 0.5f, position.y - size * 0.5f };

		naui_fill_rect(point_position, (Naui_Vec2) { size, size }, accent_color, FLT_MAX, NAUI_CORNER_ALL);
		if (selected)
			naui_draw_rect(point_position, (Naui_Vec2) { size, size }, outline_color, NAUI_DPI(1.5f), FLT_MAX, NAUI_CORNER_ALL, NAUI_SIDE_ALL);
	}

	char text[96];
	const int32_t point_index = editor->drag_mode == UPH_AUTOMATION_DRAG_POINT ? editor->drag_index : editor->hovered_point;
	const int32_t handle_index = editor->drag_mode == UPH_AUTOMATION_DRAG_TENSION ? editor->drag_index : editor->hovered_handle;
	if (point_index >= 0 && (uint32_t)point_index < count)
	{
		const Uph_AutomationPoint *point = &automation->points[point_index];
		snprintf(text, sizeof(text), "Beat %.3f  |  %.3f", point->beat, point->value);
		_uph_automation_editor_draw_readout(box, _uph_automation_editor_point_position(editor, box, point), text);
	}
	else if (handle_index >= 0 && (uint32_t)handle_index + 1 < count)
	{
		const Uph_AutomationPoint *point = &automation->points[handle_index];
		Naui_Vec2 position;
		if (_uph_automation_editor_handle_position(editor, box, automation, (uint32_t)handle_index, &position))
		{
			snprintf(text, sizeof(text), "%s  |  Tension %+.2f", _uph_automation_editor_curve_names[point->curve], point->tension);
			_uph_automation_editor_draw_readout(box, position, text);
		}
	}

	naui_pop_clip_rect();
}

#pragma endregion

#pragma region Layout

static void _uph_automation_editor_render_message(const char *message)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
	})
	{
		leaf_text(message, {
			.color = {naui_theme_color("uph_ui_text_disabled_color")},
			.font_size = {NAUI_DPI(naui_theme_float("uph_ui_font_size"))}
		});
	}
}

static void _uph_automation_editor_render_label(const char *label, bool disabled)
{
	leaf_text(label, {
		.color = {naui_theme_color(disabled ? "uph_ui_text_disabled_color" : "uph_ui_text_color")},
		.font_size = {NAUI_DPI(naui_theme_float("uph_ui_font_size"))}
	});
}

static void _uph_automation_editor_render_info(const Uph_Automation *automation)
{
	const float font_size = naui_theme_float("uph_ui_font_size");

	char info[320];
	snprintf(
		info,
		sizeof(info),
		"%u points  |  %.2f beats  |  Click to add, right-click a point for options, right-click a segment for curve type, drag a curve handle to bend (double-click resets). Hold Shift for fine control, Alt to bypass snap.",
		(uint32_t)naui_list_len(automation->points),
		uph_calculate_automation_length(automation)
	);

	leaf({
		.direction = LEAF_DIRECTION_VERTICAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), NAUI_DPI(6.0f)),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_gap = NAUI_DPI(2.0f)
	})
	{
		leaf_text(automation->name.data, {
			.color = {naui_theme_color("uph_ui_text_color")},
			.font_size = {NAUI_DPI(font_size + 2.0f)}
		});
		leaf_text(info, {
			.color = {naui_theme_color("uph_ui_text_disabled_color")},
			.font_size = {NAUI_DPI(font_size)}
		});
	}
}

static void _uph_automation_editor_render_toolbar(Uph_AutomationEditorData *editor, Uph_Automation *automation)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(UPH_AUTOMATION_EDITOR_TOOLBAR_FIELD_GAP),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_BOTTOM,
			.color = naui_theme_color("uph_timeline_top_ruler_border_color")
		}
	})
	{
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(6)
		})
		{
			static const char *snap_options[] = {
				"Beat",
				"1/2",
				"1/4",
				"1/8",
				"1/16"
			};

			_uph_automation_editor_render_label("Snap", false);
			leaf({
				.size = {LEAF_SIZE_FIXED(NAUI_DPI(60)), LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
			}) uph_ui_dropdown(snap_options, 5, (uint32_t*)&editor->snap_resolution, leaf_id("uph_automation_editor_snap"));

			if (uph_ui_text_button("Fit", leaf_id("uph_automation_editor_fit")))
				editor->fit = true;
		}

		const int32_t index = editor->selected_point;
		if (index >= 0 && (uint32_t)index < (uint32_t)naui_list_len(automation->points))
		{
			Uph_AutomationPoint *point = &automation->points[index];
			const Uph_AutomationPoint before = *point;
			const bool has_segment = (uint32_t)index + 1 < (uint32_t)naui_list_len(automation->points);
			bool changed = false;

			leaf({
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
				.child_gap = NAUI_DPI(6)
			})
			{
				float beat = (float)point->beat;
				_uph_automation_editor_render_label("Beat", index == 0);
				leaf({
					.size = {LEAF_SIZE_FIXED(NAUI_DPI(80)), LEAF_SIZE_FULL},
					.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
				})
				{
					if (uph_ui_drag_float(&beat, leaf_id("uph_automation_editor_beat"), 0.01f, 0.0f, 100000.0f, "%.3f", UPH_UI_DRAG_FLAG_CLAMPED))
					{
						point->beat = _uph_automation_editor_clamp_beat(automation, (uint32_t)index, (double)beat);
						changed = true;
					}
				}
			}

			leaf({
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
				.child_gap = NAUI_DPI(6)
			})
			{
				float value = point->value;
				_uph_automation_editor_render_label("Value", false);
				leaf({
					.size = {LEAF_SIZE_FIXED(NAUI_DPI(80)), LEAF_SIZE_FULL},
					.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
				})
				{
					if (uph_ui_drag_float(&value, leaf_id("uph_automation_editor_value"), 0.002f, 0.0f, 1.0f, "%.3f", UPH_UI_DRAG_FLAG_CLAMPED))
					{
						point->value = NAUI_CLAMP01(value);
						changed = true;
					}
				}
			}

			if (has_segment)
			{
				leaf({
					.direction = LEAF_DIRECTION_HORIZONTAL,
					.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
					.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
					.child_gap = NAUI_DPI(6)
				})
				{
					uint32_t curve = (uint32_t)point->curve;
					_uph_automation_editor_render_label("Curve", false);
					leaf({
						.size = {LEAF_SIZE_FIXED(NAUI_DPI(80)), LEAF_SIZE_FULL},
						.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
					})
					{
						if (uph_ui_dropdown(_uph_automation_editor_curve_names, UPH_AUTOMATION_CURVE_COUNT, &curve, leaf_id("uph_automation_editor_curve")))
						{
							point->curve = (Uph_AutomationCurve)curve;
							changed = true;
						}
					}
				}

				if (point->curve != UPH_AUTOMATION_CURVE_STEP)
				{
					leaf({
						.direction = LEAF_DIRECTION_HORIZONTAL,
						.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
						.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
						.child_gap = NAUI_DPI(6)
					})
					{
						float tension = point->tension;
						_uph_automation_editor_render_label("Tension", false);
						leaf({
							.size = {LEAF_SIZE_FIXED(NAUI_DPI(80)), LEAF_SIZE_FULL},
							.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
						})
						{
							if (uph_ui_drag_float(&tension, leaf_id("uph_automation_editor_tension"), 0.005f, -1.0f, 1.0f, "%.2f", UPH_UI_DRAG_FLAG_CLAMPED))
							{
								point->tension = NAUI_CLAMP(tension, -1.0f, 1.0f);
								changed = true;
							}
						}
					}
				}
			}

			if (changed && !editor->field_editing)
			{
				editor->field_editing = true;
				editor->field_index = index;
				editor->field_initial = before;
			}
		}
		else
		{
			_uph_automation_editor_render_label("Select a point to edit it precisely.", true);
		}
	}

	if (editor->field_editing && !naui_mouse_down(NAUI_MOUSE_LEFT) && !_uph_automation_editor_fields_text_active())
		_uph_automation_editor_commit_fields(editor);
}

static void _uph_automation_editor_render_entry(Uph_AutomationEditorData *editor, Uph_Automation *automation)
{
	if (!editor->entry_active)
		return;

	const int32_t index = editor->entry_index;
	if (index < 0 || (uint32_t)index >= (uint32_t)naui_list_len(automation->points) || naui_key_pressed(NAUI_KEY_ESCAPE))
	{
		_uph_automation_editor_close_entry(editor);
		return;
	}

	const Naui_Vec2 anchor = _uph_automation_editor_point_position(editor, editor->lane_box, &automation->points[index]);
	const float width = NAUI_DPI(90.0f);
	const float height = NAUI_DPI(26.0f);

	char placeholder[32];
	snprintf(placeholder, sizeof(placeholder), "%.3f", automation->points[index].value);

	bool submitted = false;
	leaf({
		.id = leaf_id("uph_automation_editor_entry"),
		.size = {LEAF_SIZE_FIXED(width), LEAF_SIZE_FIXED(height)},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(4.0f), NAUI_DPI(3.0f)),
		.color = {naui_theme_color("uph_ui_dropdown_bg_color")},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
		.floating = { .offset = { anchor.x + NAUI_DPI(10.0f), anchor.y - height * 0.5f } }
	})
	{
		submitted = uph_ui_textfield(&editor->entry_text, leaf_id("uph_automation_editor_entry_field"), UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY | UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE, placeholder);
	}

	if (!submitted || !naui_key_pressed(NAUI_KEY_ENTER))
		return;

	if (editor->entry_text.length > 0)
		_uph_automation_editor_set_value(editor, automation, (uint32_t)index, (float)atof(editor->entry_text.data));

	_uph_automation_editor_close_entry(editor);
}

#pragma endregion

void uph_automation_editor_init(void)
{
	Uph_AutomationEditorData *editor = &uph_automation_editor_data;
	memset(editor, 0, sizeof(*editor));
	editor->view.zoom = (Naui_Vec2) { NAUI_DPI(100.0f), NAUI_DPI(30.0f) };
	editor->snap_resolution = UPH_SNAP_QUARTER;
	editor->selected_point = -1;
	editor->hovered_point = -1;
	editor->hovered_handle = -1;
	editor->menu_segment = -1;
	editor->menu_point = -1;
	editor->entry_index = -1;
	editor->drag_index = -1;
	editor->field_index = -1;
	editor->fit = true;
}

void uph_automation_editor_reset_interaction(void)
{
	Uph_AutomationEditorData *editor = &uph_automation_editor_data;
	_uph_automation_editor_finish_drag(editor);
	_uph_automation_editor_commit_fields(editor);
	_uph_automation_editor_close_entry(editor);
	editor->panning = false;
	editor->lane_hovered = false;
	editor->hovered_point = -1;
	editor->hovered_handle = -1;
	editor->has_automation = false;
}

void uph_automation_editor_update(void)
{
	Uph_AutomationEditorData *editor = &uph_automation_editor_data;
	const Uph_ResourceIndex index = uph_state.shared.selected_resource.index;

	if (index >= (Uph_ResourceIndex)naui_list_len(uph_state.project.automations))
	{
		uph_automation_editor_reset_interaction();
		_uph_automation_editor_render_message("No automation is selected.");
		return;
	}

	if (!editor->has_automation || editor->automation_index != index)
	{
		uph_automation_editor_reset_interaction();
		editor->automation_index = index;
		editor->has_automation = true;
		editor->fit = true;
		editor->selected_point = -1;
		editor->menu_segment = -1;
		editor->menu_point = -1;
	}

	Uph_Automation *automation = &uph_state.project.automations[index];
	if (editor->selected_point >= (int32_t)naui_list_len(automation->points))
		editor->selected_point = -1;

	const Leaf_ID lane_id = leaf_id("uph_automation_editor_lane");
	editor->panel_hovered = naui_panel_is_hovered(naui_panel_current());
	editor->lane_box = leaf_get_bounding_box(lane_id);
	editor->lane_hovered = editor->panel_hovered && leaf_hovered(lane_id);

	const Uph_UIMenuID point_menu = uph_ui_context_menu();
	const Uph_UIMenuID segment_menu = uph_ui_context_menu();
	_uph_automation_editor_update_view(editor, automation);
	_uph_automation_editor_update_hover(editor, automation);
	_uph_automation_editor_update_mouse(editor, automation, point_menu, segment_menu);
	_uph_automation_editor_update_keys(editor, automation);
	_uph_automation_editor_update_point_menu(editor, automation, point_menu);
	_uph_automation_editor_update_segment_menu(editor, automation, segment_menu);

	_uph_automation_editor_render_info(automation);
	_uph_automation_editor_render_toolbar(editor, automation);

	const float gutter_width = NAUI_DPI(UPH_AUTOMATION_EDITOR_GUTTER_WIDTH);
	const int32_t ruler_height = (int32_t)NAUI_DPI(UPH_AUTOMATION_EDITOR_RULER_HEIGHT);
	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(ruler_height)},
		.color = naui_theme_color("uph_timeline_top_ruler_bg_color"),
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_BOTTOM,
			.color = naui_theme_color("uph_timeline_top_ruler_border_color")
		}
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(gutter_width), LEAF_SIZE_FULL},
			.color = naui_theme_color("uph_toolbox_bg_color")
		});
		leaf({
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.custom_draw = (Leaf_CustomDrawFn)_uph_automation_editor_ruler_draw
		});
	}

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
		.clip_children = true
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(gutter_width), LEAF_SIZE_FULL},
			.color = naui_theme_color("uph_toolbox_bg_color"),
			.custom_draw = (Leaf_CustomDrawFn)_uph_automation_editor_gutter_draw
		});
		leaf({
			.id = lane_id,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.custom_draw = (Leaf_CustomDrawFn)_uph_automation_editor_lane_draw
		});
	}

	_uph_automation_editor_render_entry(editor, automation);
}