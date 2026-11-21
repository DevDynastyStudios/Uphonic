#define UPH_AUTOMATION_EDITOR_RULER_HEIGHT		24
#define UPH_AUTOMATION_EDITOR_GUTTER_WIDTH		44
#define UPH_AUTOMATION_EDITOR_LANE_PADDING		14
#define UPH_AUTOMATION_EDITOR_TOOLBAR_FIELD_GAP	16

#define UPH_AUTOMATION_EDITOR_POINT_SIZE		8
#define UPH_AUTOMATION_EDITOR_HANDLE_SIZE		7
#define UPH_AUTOMATION_EDITOR_HIT_RADIUS		8
#define UPH_AUTOMATION_EDITOR_LINE_WIDTH		2

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
Uph_AutomationEditorState;

static Uph_AutomationEditorState uph__automation_editor_state;

static const char *uph__automation_editor_curve_names[UPH_AUTOMATION_CURVE_COUNT] = {
	"Curve",
	"S-Curve",
	"Step"
};

typedef struct
{
	const char *unit;
	float min;
	float max;
	const char *format;
}
Uph_AutomationDisplayInfo;

static const char *uph__automation_editor_display_names[UPH_AUTOMATION_DISPLAY_COUNT] = {
	"Normalized",
	"Percent",
	"BPM",
	"Volume",
	"Pan"
};

static const Uph_AutomationDisplayInfo uph__automation_editor_displays[UPH_AUTOMATION_DISPLAY_COUNT] = {
	{ "", 0.0f, 1.0f, "%.3f" },
	{ "%", 0.0f, 100.0f, "%.1f" },
	{ "BPM", UPH_TEMPO_MIN_BPM, UPH_TEMPO_MAX_BPM, "%.1f" },
	{ "dB", UPH_MIXER_DB_MIN, UPH_MIXER_DB_MAX, "%.1f" },
	{ "", -100.0f, 100.0f, "%.0f" }
};

#pragma region Helpers

static inline bool uph__automation_editor_snap_disabled(void)
{
	return uph_ui_shift_down() || naui_key_down(NAUI_KEY_LALT) || naui_key_down(NAUI_KEY_RALT);
}

static inline float uph__automation_editor_value_top(Leaf_BoundingBox box)
{
	return box.y + NAUI_DPI(UPH_AUTOMATION_EDITOR_LANE_PADDING);
}

static inline float uph__automation_editor_value_height(Leaf_BoundingBox box)
{
	return fmaxf(1.0f, box.height - NAUI_DPI(UPH_AUTOMATION_EDITOR_LANE_PADDING) * 2.0f);
}

static inline float uph__automation_editor_value_to_y(Leaf_BoundingBox box, float value)
{
	return uph__automation_editor_value_top(box) + (1.0f - value) * uph__automation_editor_value_height(box);
}

static inline float uph__automation_editor_y_to_value(Leaf_BoundingBox box, float y)
{
	return NAUI_CLAMP01(1.0f - (y - uph__automation_editor_value_top(box)) / uph__automation_editor_value_height(box));
}

static inline Naui_Vec2 uph__automation_editor_point_position(const Uph_AutomationEditorState *editor, Leaf_BoundingBox box, const Uph_AutomationPoint *point)
{
	return (Naui_Vec2) {
		uph_midi_view_beat_to_x(&editor->view, box, point->beat),
		uph__automation_editor_value_to_y(box, point->value)
	};
}

static inline const Uph_AutomationDisplayInfo *uph__automation_editor_display_info(Uph_AutomationDisplay display)
{
	return &uph__automation_editor_displays[display < UPH_AUTOMATION_DISPLAY_COUNT ? display : UPH_AUTOMATION_DISPLAY_NORMALIZED];
}

static inline float uph__automation_editor_to_display(Uph_AutomationDisplay display, float value)
{
	const Uph_AutomationDisplayInfo *info = uph__automation_editor_display_info(display);
	return info->min + NAUI_CLAMP01(value) * (info->max - info->min);
}

static inline float uph__automation_editor_from_display(Uph_AutomationDisplay display, float shown)
{
	const Uph_AutomationDisplayInfo *info = uph__automation_editor_display_info(display);
	return NAUI_CLAMP01((shown - info->min) / (info->max - info->min));
}

static void uph__automation_editor_format_value(Uph_AutomationDisplay display, float value, bool compact, char *out, size_t size)
{
	const float shown = uph__automation_editor_to_display(display, value);
	switch (display)
	{
		case UPH_AUTOMATION_DISPLAY_PERCENT:
			snprintf(out, size, compact ? "%.0f%%" : "%.1f%%", shown);
			break;
		case UPH_AUTOMATION_DISPLAY_BPM:
			snprintf(out, size, compact ? "%.0f" : "%.1f BPM", shown);
			break;
		case UPH_AUTOMATION_DISPLAY_VOLUME:
			if (value <= 0.0f)
				snprintf(out, size, compact ? "-inf" : "-inf dB");
			else
				snprintf(out, size, compact ? "%.0f" : "%.1f dB", shown);
			break;
		case UPH_AUTOMATION_DISPLAY_PAN:
		{
			const float rounded = roundf(shown);
			if (rounded == 0.0f)
				snprintf(out, size, "C");
			else
				snprintf(out, size, "%c%.0f", rounded < 0.0f ? 'L' : 'R', fabsf(rounded));
			break;
		}
		default:
			snprintf(out, size, compact ? "%.2f" : "%.3f", shown);
			break;
	}
}

static Uph_Automation *uph__automation_editor_current(const Uph_AutomationEditorState *editor)
{
	if (!editor->has_automation || editor->automation_index >= (Uph_ResourceIndex)naui_list_len(uph_state.project.automations))
		return NULL;

	return &uph_state.project.automations[editor->automation_index];
}

static bool uph__automation_editor_handle_position(const Uph_AutomationEditorState *editor, Leaf_BoundingBox box, const Uph_Automation *automation, uint32_t segment, Naui_Vec2 *out)
{
	if (segment + 1 >= (uint32_t)naui_list_len(automation->points))
		return false;

	const Uph_AutomationPoint *from = &automation->points[segment];
	const Uph_AutomationPoint *to = &automation->points[segment + 1];
	if (from->curve == UPH_AUTOMATION_CURVE_STEP)
		return false;

	const float x0 = uph_midi_view_beat_to_x(&editor->view, box, from->beat);
	const float x1 = uph_midi_view_beat_to_x(&editor->view, box, to->beat);
	if (x1 - x0 < NAUI_DPI(UPH_AUTOMATION_EDITOR_HANDLE_SIZE * 3))
		return false;

	const double mid_beat = (from->beat + to->beat) * 0.5;
	*out = (Naui_Vec2) {
		uph_midi_view_beat_to_x(&editor->view, box, mid_beat),
		uph__automation_editor_value_to_y(box, uph_automation_segment_value(from, to, mid_beat))
	};
	return true;
}

static int32_t uph__automation_editor_segment_at(const Uph_Automation *automation, double beat)
{
	const uint32_t count = (uint32_t)naui_list_len(automation->points);
	for (uint32_t i = 0; i + 1 < count; i++)
	{
		if (beat >= automation->points[i].beat && beat < automation->points[i + 1].beat)
			return (int32_t)i;
	}
	return -1;
}

static double uph__automation_editor_clamp_beat(const Uph_Automation *automation, uint32_t index, double beat)
{
	if (index == 0)
		return automation->points[0].beat;

	beat = fmax(beat, automation->points[index - 1].beat);
	if (index + 1 < (uint32_t)naui_list_len(automation->points))
		beat = fmin(beat, automation->points[index + 1].beat);

	return fmax(beat, 0.0);
}

static double uph__automation_editor_snap(const Uph_AutomationEditorState *editor, double beat)
{
	if (uph__automation_editor_snap_disabled())
		return beat;

	return uph_snap_beat_round(beat, editor->snap_resolution, editor->view.zoom.x);
}

static bool uph__automation_editor_fields_text_active(void)
{
	return uph_ui_textfield_active(leaf_id("uph_automation_editor_beat"))
		|| uph_ui_textfield_active(leaf_id("uph_automation_editor_value"))
		|| uph_ui_textfield_active(leaf_id("uph_automation_editor_tension"));
}

#pragma endregion

#pragma region Edits

static void uph__automation_editor_commit_move(Uph_ResourceIndex automation_index, uint32_t point_index, Uph_AutomationPoint old_point, Uph_AutomationPoint new_point)
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

static void uph__automation_editor_delete_point(Uph_AutomationEditorState *editor, uint32_t index)
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

static void uph__automation_editor_set_curve(Uph_AutomationEditorState *editor, Uph_Automation *automation, uint32_t index, Uph_AutomationCurve curve, float tension)
{
	Uph_AutomationPoint *point = &automation->points[index];
	const Uph_AutomationPoint old_point = *point;
	point->curve = curve;
	point->tension = tension;
	uph__automation_editor_commit_move(editor->automation_index, index, old_point, *point);
}

static void uph__automation_editor_set_value(Uph_AutomationEditorState *editor, Uph_Automation *automation, uint32_t index, float value)
{
	Uph_AutomationPoint *point = &automation->points[index];
	const Uph_AutomationPoint old_point = *point;
	point->value = NAUI_CLAMP01(value);
	uph__automation_editor_commit_move(editor->automation_index, index, old_point, *point);
}

static void uph__automation_editor_open_entry(Uph_AutomationEditorState *editor, uint32_t index)
{
	editor->entry_active = true;
	editor->entry_index = (int32_t)index;
	editor->entry_text = naui_string_from_cstr("");
}

static void uph__automation_editor_close_entry(Uph_AutomationEditorState *editor)
{
	editor->entry_active = false;
	editor->entry_index = -1;
}

static void uph__automation_editor_begin_drag(Uph_AutomationEditorState *editor, const Uph_Automation *automation, uint32_t index, Uph_AutomationDragMode mode, bool creating)
{
	editor->drag_mode = mode;
	editor->drag_index = (int32_t)index;
	editor->drag_initial = automation->points[index];
	editor->drag_creating = creating;
	editor->drag_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
	editor->drag_offset = (Naui_Vec2) { 0.0f, 0.0f };
}

static void uph__automation_editor_finish_drag(Uph_AutomationEditorState *editor)
{
	const Uph_AutomationDragMode mode = editor->drag_mode;
	const int32_t index = editor->drag_index;
	const bool creating = editor->drag_creating;
	editor->drag_mode = UPH_AUTOMATION_DRAG_NONE;
	editor->drag_index = -1;
	editor->drag_creating = false;

	Uph_Automation *automation = uph__automation_editor_current(editor);
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

	uph__automation_editor_commit_move(editor->automation_index, (uint32_t)index, editor->drag_initial, point);
}

static void uph__automation_editor_commit_fields(Uph_AutomationEditorState *editor)
{
	if (!editor->field_editing)
		return;

	editor->field_editing = false;
	Uph_Automation *automation = uph__automation_editor_current(editor);
	if (!automation || editor->field_index < 0 || (uint32_t)editor->field_index >= (uint32_t)naui_list_len(automation->points))
		return;

	uph__automation_editor_commit_move(editor->automation_index, (uint32_t)editor->field_index, editor->field_initial, automation->points[editor->field_index]);
}

#pragma endregion

#pragma region Input

static void uph__automation_editor_update_view(Uph_AutomationEditorState *editor, const Uph_Automation *automation)
{
	Uph_MidiView *view = &editor->view;
	const Leaf_BoundingBox box = editor->lane_box;
	if (box.width < 1.0f)
		return;

	const int32_t zoom_min = NAUI_DPI(UPH_AUTOMATION_EDITOR_ZOOM_MIN);
	const int32_t zoom_max = NAUI_DPI(UPH_AUTOMATION_EDITOR_ZOOM_MAX);

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

static void uph__automation_editor_update_hover(Uph_AutomationEditorState *editor, const Uph_Automation *automation)
{
	editor->hovered_point = -1;
	editor->hovered_handle = -1;
	if (!editor->lane_hovered || editor->drag_mode != UPH_AUTOMATION_DRAG_NONE)
		return;

	const Leaf_BoundingBox box = editor->lane_box;
	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };
	const int32_t radius = NAUI_DPI(UPH_AUTOMATION_EDITOR_HIT_RADIUS);
	const uint32_t count = (uint32_t)naui_list_len(automation->points);

	float best = radius * radius;
	for (uint32_t i = 0; i < count; i++)
	{
		const Naui_Vec2 position = uph__automation_editor_point_position(editor, box, &automation->points[i]);
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
		if (!uph__automation_editor_handle_position(editor, box, automation, i, &position))
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

static void uph__automation_editor_update_drag(Uph_AutomationEditorState *editor, Uph_Automation *automation)
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
	const float scale = uph_ui_shift_down() ? UPH_AUTOMATION_EDITOR_FINE_SCALE : 1.0f;
	editor->drag_offset.x += (mouse.x - editor->drag_last_mouse.x) * scale;
	editor->drag_offset.y += (mouse.y - editor->drag_last_mouse.y) * scale;
	editor->drag_last_mouse = mouse;

	const float value_height = uph__automation_editor_value_height(box);
	Uph_AutomationPoint *point = &automation->points[index];

	if (editor->drag_mode == UPH_AUTOMATION_DRAG_POINT)
	{
		point->value = NAUI_CLAMP01(editor->drag_initial.value - editor->drag_offset.y / value_height);
		const double beat = uph__automation_editor_snap(editor, editor->drag_initial.beat + (double)editor->drag_offset.x / editor->view.zoom.x);
		point->beat = uph__automation_editor_clamp_beat(automation, (uint32_t)index, beat);
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

	uph__automation_editor_finish_drag(editor);
}

static void uph__automation_editor_update_mouse(Uph_AutomationEditorState *editor, Uph_Automation *automation, Uph_UIMenuID point_menu, Uph_UIMenuID segment_menu)
{
	if (editor->drag_mode != UPH_AUTOMATION_DRAG_NONE)
	{
		uph__automation_editor_update_drag(editor, automation);
		return;
	}

	if (editor->entry_active)
	{
		const bool pressed = naui_mouse_pressed(NAUI_MOUSE_LEFT) || naui_mouse_pressed(NAUI_MOUSE_RIGHT);
		if (pressed && !leaf_hovered(leaf_id("uph_automation_editor_entry")))
			uph__automation_editor_close_entry(editor);

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
			uph__automation_editor_begin_drag(editor, automation, (uint32_t)editor->hovered_point, UPH_AUTOMATION_DRAG_POINT, false);
			return;
		}

		if (editor->hovered_handle >= 0)
		{
			uph__automation_editor_begin_drag(editor, automation, (uint32_t)editor->hovered_handle, UPH_AUTOMATION_DRAG_TENSION, false);
			return;
		}

		const uint32_t count = (uint32_t)naui_list_len(automation->points);
		const double beat = fmax(0.0, uph__automation_editor_snap(editor, uph_midi_view_x_to_beat(&editor->view, box, mouse_x)));

		uint32_t insert_index = count;
		for (uint32_t i = 0; i < count; i++)
		{
			if (beat < automation->points[i].beat)
			{
				insert_index = i;
				break;
			}
		}

		Uph_AutomationPoint point = { .beat = beat, .value = uph__automation_editor_y_to_value(box, mouse_y) };
		if (insert_index > 0)
			point.curve = automation->points[insert_index - 1].curve;

		naui_list_insert(automation->points, point, insert_index);
		editor->selected_point = (int32_t)insert_index;
		uph__automation_editor_begin_drag(editor, automation, insert_index, UPH_AUTOMATION_DRAG_POINT, true);
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
			: uph__automation_editor_segment_at(automation, uph_midi_view_x_to_beat(&editor->view, box, mouse_x));

		if (segment >= 0)
		{
			editor->menu_segment = segment;
			uph_ui_open_context_menu(segment_menu);
		}
	}
}

static void uph__automation_editor_update_keys(Uph_AutomationEditorState *editor, Uph_Automation *automation)
{
	if (naui_panel_focused() != uph_state.panels.resource_editor || editor->drag_mode != UPH_AUTOMATION_DRAG_NONE || editor->entry_active || uph__automation_editor_fields_text_active())
		return;

	const int32_t index = editor->selected_point;
	if (index < 0 || (uint32_t)index >= (uint32_t)naui_list_len(automation->points))
		return;

	if (naui_keybind_pressed(uph_state.settings.keybinds.delete_selected))
	{
		uph__automation_editor_delete_point(editor, (uint32_t)index);
		return;
	}

	if (naui_keybind_pressed(uph_state.settings.keybinds.deselect))
	{
		editor->selected_point = -1;
		return;
	}

	Uph_AutomationPoint *point = &automation->points[index];
	const Uph_AutomationPoint old_point = *point;
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;
	const double step = uph_snap_division(editor->snap_resolution, editor->view.zoom.x);

	if (naui_keybind_pressed_repeat(keybinds->automation_nudge_up))
		point->value = NAUI_CLAMP01(point->value + UPH_AUTOMATION_EDITOR_NUDGE);
	if (naui_keybind_pressed_repeat(keybinds->automation_nudge_down))
		point->value = NAUI_CLAMP01(point->value - UPH_AUTOMATION_EDITOR_NUDGE);
	if (naui_keybind_pressed_repeat(keybinds->automation_nudge_right))
		point->beat = uph__automation_editor_clamp_beat(automation, (uint32_t)index, point->beat + step);
	if (naui_keybind_pressed_repeat(keybinds->automation_nudge_left))
		point->beat = uph__automation_editor_clamp_beat(automation, (uint32_t)index, point->beat - step);

	if (naui_keybind_pressed_repeat(keybinds->automation_fine_nudge_up))
		point->value = NAUI_CLAMP01(point->value + UPH_AUTOMATION_EDITOR_FINE_NUDGE);
	if (naui_keybind_pressed_repeat(keybinds->automation_fine_nudge_down))
		point->value = NAUI_CLAMP01(point->value - UPH_AUTOMATION_EDITOR_FINE_NUDGE);
	if (naui_keybind_pressed_repeat(keybinds->automation_fine_nudge_right))
		point->beat = uph__automation_editor_clamp_beat(automation, (uint32_t)index, point->beat + step * 0.25);
	if (naui_keybind_pressed_repeat(keybinds->automation_fine_nudge_left))
		point->beat = uph__automation_editor_clamp_beat(automation, (uint32_t)index, point->beat - step * 0.25);

	uph__automation_editor_commit_move(editor->automation_index, (uint32_t)index, old_point, *point);
}

static void uph__automation_editor_update_point_menu(Uph_AutomationEditorState *editor, Uph_Automation *automation, Uph_UIMenuID menu)
{
	const int32_t index = editor->menu_point;
	const bool valid = index >= 0 && (uint32_t)index < (uint32_t)naui_list_len(automation->points);

	if (uph_ui_menu_item(menu, "Remove", leaf_id("uph_automation_editor_point_remove")) && valid)
	{
		uph__automation_editor_delete_point(editor, (uint32_t)index);
		return;
	}

	if (uph_ui_menu_item(menu, "Set value", leaf_id("uph_automation_editor_point_set")) && valid)
		uph__automation_editor_open_entry(editor, (uint32_t)index);

	if (uph_ui_menu_item(menu, "Copy value", leaf_id("uph_automation_editor_point_copy")) && valid)
	{
		editor->clipboard_value = automation->points[index].value;
		editor->has_clipboard = true;
	}

	if (editor->has_clipboard)
	{
		char value_text[32];
		char label[48];
		uph__automation_editor_format_value(automation->display, editor->clipboard_value, false, value_text, sizeof(value_text));
		snprintf(label, sizeof(label), "Paste value (%s)", value_text);
		if (uph_ui_menu_item(menu, label, leaf_id("uph_automation_editor_point_paste")) && valid)
			uph__automation_editor_set_value(editor, automation, (uint32_t)index, editor->clipboard_value);
	}
}

static void uph__automation_editor_update_segment_menu(Uph_AutomationEditorState *editor, Uph_Automation *automation, Uph_UIMenuID menu)
{
	const int32_t segment = editor->menu_segment;
	const bool valid = segment >= 0 && (uint32_t)segment + 1 < (uint32_t)naui_list_len(automation->points);

	for (uint32_t c = 0; c < UPH_AUTOMATION_CURVE_COUNT; c++)
	{
		if (uph_ui_menu_item(menu, uph__automation_editor_curve_names[c], leaf_id_indexed("uph_automation_editor_curve_item", c)) && valid)
			uph__automation_editor_set_curve(editor, automation, (uint32_t)segment, (Uph_AutomationCurve)c, automation->points[segment].tension);
	}

	if (uph_ui_menu_item(menu, "Reset tension", leaf_id("uph_automation_editor_reset_tension")) && valid)
		uph__automation_editor_set_curve(editor, automation, (uint32_t)segment, automation->points[segment].curve, 0.0f);
}

#pragma endregion

#pragma region Draw callbacks

static void uph__automation_editor_ruler_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_AutomationEditorState *state = &uph__automation_editor_state;
	uph_midi_grid_render_ruler(box, &state->view, state->snap_resolution);
}

static void uph__automation_editor_gutter_draw(Leaf_BoundingBox box, void *user_data)
{
	const Uph_Automation *automation = uph__automation_editor_current(&uph__automation_editor_state);
	const Uph_AutomationDisplay display = automation ? automation->display : UPH_AUTOMATION_DISPLAY_NORMALIZED;
	const Leaf_Color text_color = uph_state.colors.ui_text_disabled;
	const Leaf_Color tick_color = uph_state.colors.automation_editor_grid_beat;
	const int32_t font_size = NAUI_DPI(11);
	const int32_t padding = NAUI_DPI(6);

	for (uint32_t i = 0; i <= UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS; i++)
	{
		const float value = (float)i / (float)UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS;
		const float y = uph__automation_editor_value_to_y(box, value);

		char label[16];
		uph__automation_editor_format_value(display, value, true, label, sizeof(label));
		const Naui_Vec2 text_size = naui_measure_text(label, (uint32_t)strlen(label), font_size);

		naui_draw_line((Naui_Vec2) { box.x + box.width - padding * 0.5f, y }, (Naui_Vec2) { box.x + box.width, y }, tick_color, 1.0f);
		naui_draw_text((Naui_Vec2) { box.x + box.width - padding - text_size.x, y - text_size.y * 0.5f }, label, font_size, text_color);
	}
}

static void uph__automation_editor_draw_readout(Leaf_BoundingBox box, Naui_Vec2 anchor, const char *text)
{
	const int32_t font_size = NAUI_DPI(12);
	const int32_t padding = NAUI_DPI(5);
	const Naui_Vec2 text_size = naui_measure_text(text, (uint32_t)strlen(text), font_size);
	const Naui_Vec2 size = { text_size.x + padding * 2.0f, text_size.y + padding * 2.0f };

	Naui_Vec2 position = { anchor.x + NAUI_DPI(10), anchor.y - NAUI_DPI(10) - size.y };
	if (position.x + size.x > box.x + box.width)
		position.x = anchor.x - NAUI_DPI(10) - size.x;
	if (position.y < box.y)
		position.y = anchor.y + NAUI_DPI(10);

	Leaf_Color background = uph_state.colors.automation_editor_toolbox_bg;
	background.a = 230;
	naui_fill_rect(position, size, background, NAUI_DPI(3), NAUI_CORNER_ALL);
	naui_draw_text((Naui_Vec2) { position.x + padding, position.y + padding }, text, font_size, uph_state.colors.ui_text);
}

static void uph__automation_editor_lane_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_AutomationEditorState *state = &uph__automation_editor_state;
	const Uph_Automation *automation = uph__automation_editor_current(state);
	if (!automation)
		return;

	const Uph_MidiView *view = &state->view;
	const uint32_t count = (uint32_t)naui_list_len(automation->points);
	const float value_top = uph__automation_editor_value_top(box);
	const float value_height = uph__automation_editor_value_height(box);

	Leaf_Color accent_color = uph_state.colors.automation_editor_accent;
	accent_color.a = 255;
	const Leaf_Color fill_color = { accent_color.r, accent_color.g, accent_color.b, 50 };
	const Leaf_Color dim_color = { accent_color.r, accent_color.g, accent_color.b, 110 };
	const Leaf_Color guide_color = uph_state.colors.automation_editor_grid_subbeat;
	const Leaf_Color edge_color = uph_state.colors.automation_editor_grid_beat;
	const Leaf_Color lane_color = uph_state.colors.automation_editor_row_bg2;
	const Leaf_Color outline_color = uph_state.colors.ui_text;

	naui_push_clip_rect(box.x, box.y, box.width, box.height);

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, uph_state.colors.automation_editor_row_bg1, 0.0f, NAUI_CORNER_NONE);
	naui_fill_rect((Naui_Vec2) { box.x, value_top }, (Naui_Vec2) { box.width, value_height }, lane_color, 0.0f, NAUI_CORNER_NONE);
	uph_midi_grid_render_beats(box, view, state->snap_resolution);

	for (uint32_t i = 0; i <= UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS; i++)
	{
		const float y = uph__automation_editor_value_to_y(box, (float)i / (float)UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS);
		const bool edge = i == 0 || i == UPH_AUTOMATION_EDITOR_VALUE_DIVISIONS;
		naui_draw_line((Naui_Vec2) { box.x, y }, (Naui_Vec2) { box.x + box.width, y }, edge ? edge_color : guide_color, 1.0f);
	}

	const float end_x = uph_midi_view_beat_to_x(view, box, uph_calculate_automation_length(automation));
	if (end_x < box.x + box.width)
	{
		const float shade_x = fmaxf(end_x, box.x);
		naui_fill_rect((Naui_Vec2) { shade_x, box.y }, (Naui_Vec2) { box.x + box.width - shade_x, box.height }, leaf_rgba(0, 0, 0, 70), 0.0f, NAUI_CORNER_NONE);
	}

	const int32_t line_width = NAUI_DPI(UPH_AUTOMATION_EDITOR_LINE_WIDTH);
	if (count > 0)
	{
		const Naui_Vec2 first = uph__automation_editor_point_position(state, box, &automation->points[0]);
		const Naui_Vec2 last = uph__automation_editor_point_position(state, box, &automation->points[count - 1]);
		if (first.x > box.x)
			naui_draw_line((Naui_Vec2) { box.x, first.y }, first, dim_color, 1.0f);
		if (last.x < box.x + box.width)
			naui_draw_line(last, (Naui_Vec2) { box.x + box.width, last.y }, dim_color, 1.0f);
	}

	uph_ui_automation_curve(automation, box.x - view->scroll.x, view->zoom.x, value_top, value_height, accent_color, fill_color, line_width);

	const int32_t handle_size = NAUI_DPI(UPH_AUTOMATION_EDITOR_HANDLE_SIZE);
	for (uint32_t i = 0; i + 1 < count; i++)
	{
		Naui_Vec2 position;
		if (!uph__automation_editor_handle_position(state, box, automation, i, &position))
			continue;

		const bool active = state->hovered_handle == (int32_t)i || (state->drag_mode == UPH_AUTOMATION_DRAG_TENSION && state->drag_index == (int32_t)i);
		const Naui_Vec2 handle_position = { position.x - handle_size * 0.5f, position.y - handle_size * 0.5f };
		naui_fill_rect(handle_position, (Naui_Vec2) { handle_size, handle_size }, active ? accent_color : lane_color, FLT_MAX, NAUI_CORNER_ALL);
		naui_draw_rect(handle_position, (Naui_Vec2) { handle_size, handle_size }, accent_color, NAUI_DPI(1.5f), FLT_MAX, NAUI_CORNER_ALL, NAUI_SIDE_ALL);
	}

	const int32_t point_size = NAUI_DPI(UPH_AUTOMATION_EDITOR_POINT_SIZE);
	for (uint32_t i = 0; i < count; i++)
	{
		const Naui_Vec2 position = uph__automation_editor_point_position(state, box, &automation->points[i]);
		if (position.x < box.x - point_size || position.x > box.x + box.width + point_size)
			continue;

		const bool selected = state->selected_point == (int32_t)i;
		const bool hovered = state->hovered_point == (int32_t)i || (state->drag_mode == UPH_AUTOMATION_DRAG_POINT && state->drag_index == (int32_t)i);
		const float size = point_size + ((selected || hovered) ? NAUI_DPI(2) : 0.0f);
		const Naui_Vec2 point_position = { position.x - size * 0.5f, position.y - size * 0.5f };

		naui_fill_rect(point_position, (Naui_Vec2) { size, size }, accent_color, FLT_MAX, NAUI_CORNER_ALL);
		if (selected)
			naui_draw_rect(point_position, (Naui_Vec2) { size, size }, outline_color, NAUI_DPI(1.5f), FLT_MAX, NAUI_CORNER_ALL, NAUI_SIDE_ALL);
	}

	char text[96];
	const int32_t point_index = state->drag_mode == UPH_AUTOMATION_DRAG_POINT ? state->drag_index : state->hovered_point;
	const int32_t handle_index = state->drag_mode == UPH_AUTOMATION_DRAG_TENSION ? state->drag_index : state->hovered_handle;
	if (point_index >= 0 && (uint32_t)point_index < count)
	{
		const Uph_AutomationPoint *point = &automation->points[point_index];
		char value_text[32];
		uph__automation_editor_format_value(automation->display, point->value, false, value_text, sizeof(value_text));
		snprintf(text, sizeof(text), "Beat %.3f  |  %s", point->beat, value_text);
		uph__automation_editor_draw_readout(box, uph__automation_editor_point_position(state, box, point), text);
	}
	else if (handle_index >= 0 && (uint32_t)handle_index + 1 < count)
	{
		const Uph_AutomationPoint *point = &automation->points[handle_index];
		Naui_Vec2 position;
		if (uph__automation_editor_handle_position(state, box, automation, (uint32_t)handle_index, &position))
		{
			snprintf(text, sizeof(text), "%s  |  Tension %+.2f", uph__automation_editor_curve_names[point->curve], point->tension);
			uph__automation_editor_draw_readout(box, position, text);
		}
	}

	naui_pop_clip_rect();
}

#pragma endregion

#pragma region Layout

static void uph__automation_editor_render_message(const char *message)
{
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
	})
	{
		leaf_text(message, {
			.color = uph_state.colors.ui_text_disabled,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
		});
	}
}

static void uph__automation_editor_render_label(const char *label, bool disabled)
{
	leaf_text(label, {
		.color = disabled ? uph_state.colors.ui_text_disabled : uph_state.colors.ui_text,
		.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_TOOLBOX_FONT_SIZE))
	});
}

static void uph__automation_editor_render_info(const Uph_Automation *automation)
{
	const float font_size = UPH_UI_FONT_SIZE;

	char info[320];
	snprintf(
		info,
		sizeof(info),
		"%u points  |  %.2f beats  |  Click to add, right-click a point for options, right-click a segment for curve type, drag a curve handle to bend (double-click resets). Hold Shift for fine control, Alt to bypass snap.",
		(uint32_t)naui_list_len(automation->points),
		uph_calculate_automation_length(automation)
	);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_UI_FRAME_PADDING.x * 2), NAUI_DPI(6)),
		.color = uph_state.colors.automation_editor_toolbox_bg,
		.child_gap = NAUI_DPI(2),
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{
		leaf_text(automation->name.data, {
			.color = uph_state.colors.ui_text,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(font_size + 2))
		});
		leaf_text(info, {
			.color = uph_state.colors.ui_text_disabled,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(font_size))
		});
	}
}

static void uph__automation_editor_render_toolbar(Uph_AutomationEditorState *editor, Uph_Automation *automation)
{
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_TOOLBOX_HEIGHT)) },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_TOOLBOX_PADDING_X), 0.0f),
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_BOTTOM,
			.color = uph_state.colors.automation_editor_ruler_border
		},
		.color = uph_state.colors.automation_editor_toolbox_bg,
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(UPH_AUTOMATION_EDITOR_TOOLBAR_FIELD_GAP),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(6),
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			uph__automation_editor_render_label("Snap", false);
			leaf({
				.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(60)), .height = LEAF_SIZE_FULL },
				.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
			}) uph_ui_snap_dropdown(&editor->snap_resolution, leaf_id("uph_automation_editor_snap"));

			if (uph_ui_text_button("Fit", leaf_id("uph_automation_editor_fit")))
				editor->fit = true;
		}

		leaf({
			.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(6),
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			uint32_t display = (uint32_t)automation->display;
			uph__automation_editor_render_label("Display", false);
			leaf({
				.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(96)), .height = LEAF_SIZE_FULL },
				.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
			})
			{
				if (uph_ui_dropdown(uph__automation_editor_display_names, UPH_AUTOMATION_DISPLAY_COUNT, &display, leaf_id("uph_automation_editor_display")))
					automation->display = (Uph_AutomationDisplay)display;
			}
		}

		const int32_t index = editor->selected_point;
		if (index >= 0 && (uint32_t)index < (uint32_t)naui_list_len(automation->points))
		{
			Uph_AutomationPoint *point = &automation->points[index];
			const Uph_AutomationPoint before = *point;
			const bool has_segment = (uint32_t)index + 1 < (uint32_t)naui_list_len(automation->points);
			bool changed = false;

			leaf({
				.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
				.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
				.child_gap = NAUI_DPI(6),
				.direction = LEAF_DIRECTION_HORIZONTAL
			})
			{
				float beat = (float)point->beat;
				uph__automation_editor_render_label("Beat", index == 0);
				leaf({
					.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(80)), .height = LEAF_SIZE_FULL },
					.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
				})
				{
					if (uph_ui_drag_float(&beat, leaf_id("uph_automation_editor_beat"), 0.01f, 0.0f, 100000.0f, "%.3f", UPH_UI_DRAG_FLAG_CLAMPED))
					{
						point->beat = uph__automation_editor_clamp_beat(automation, (uint32_t)index, (double)beat);
						changed = true;
					}
				}
			}

			leaf({
				.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
				.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
				.child_gap = NAUI_DPI(6),
				.direction = LEAF_DIRECTION_HORIZONTAL
			})
			{
				const Uph_AutomationDisplayInfo *info = uph__automation_editor_display_info(automation->display);
				float value = uph__automation_editor_to_display(automation->display, point->value);

				char label[32];
				if (info->unit[0])
					snprintf(label, sizeof(label), "Value (%s)", info->unit);
				else
					snprintf(label, sizeof(label), "Value");

				uph__automation_editor_render_label(label, false);
				leaf({
					.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(80)), .height = LEAF_SIZE_FULL },
					.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
				})
				{
					if (uph_ui_drag_float(&value, leaf_id("uph_automation_editor_value"), 0.002f * (info->max - info->min), info->min, info->max, info->format, UPH_UI_DRAG_FLAG_CLAMPED))
					{
						point->value = uph__automation_editor_from_display(automation->display, value);
						changed = true;
					}
				}
			}

			if (has_segment)
			{
				leaf({
					.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
					.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
					.child_gap = NAUI_DPI(6),
					.direction = LEAF_DIRECTION_HORIZONTAL
				})
				{
					uint32_t curve = (uint32_t)point->curve;
					uph__automation_editor_render_label("Curve", false);
					leaf({
						.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(80)), .height = LEAF_SIZE_FULL },
						.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
					})
					{
						if (uph_ui_dropdown(uph__automation_editor_curve_names, UPH_AUTOMATION_CURVE_COUNT, &curve, leaf_id("uph_automation_editor_curve")))
						{
							point->curve = (Uph_AutomationCurve)curve;
							changed = true;
						}
					}
				}

				if (point->curve != UPH_AUTOMATION_CURVE_STEP)
				{
					leaf({
						.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
						.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
						.child_gap = NAUI_DPI(6),
						.direction = LEAF_DIRECTION_HORIZONTAL
					})
					{
						float tension = point->tension;
						uph__automation_editor_render_label("Tension", false);
						leaf({
							.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(80)), .height = LEAF_SIZE_FULL },
							.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
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
			uph__automation_editor_render_label("Select a point to edit it precisely.", true);
		}
	}

	if (editor->field_editing && !naui_mouse_down(NAUI_MOUSE_LEFT) && !uph__automation_editor_fields_text_active())
		uph__automation_editor_commit_fields(editor);
}

static void uph__automation_editor_render_entry(Uph_AutomationEditorState *editor, Uph_Automation *automation)
{
	if (!editor->entry_active)
		return;

	const int32_t index = editor->entry_index;
	if (index < 0 || (uint32_t)index >= (uint32_t)naui_list_len(automation->points) || naui_key_pressed(NAUI_KEY_ESCAPE))
	{
		uph__automation_editor_close_entry(editor);
		return;
	}

	const Naui_Vec2 anchor = uph__automation_editor_point_position(editor, editor->lane_box, &automation->points[index]);
	const int32_t width = NAUI_DPI(90);
	const int32_t height = NAUI_DPI(26);

	bool submitted = false;
	leaf({
		.id = leaf_id("uph_automation_editor_entry"),
		.size = { .width = LEAF_SIZE_FIXED(width), .height = LEAF_SIZE_FIXED(height) },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(4), NAUI_DPI(3)),
		.color = uph_state.colors.ui_dropdown_bg,
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
		.floating = { .offset = { .x = anchor.x + NAUI_DPI(10), .y = anchor.y - height * 0.5f } },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
	})
	{
		submitted = uph_ui_textfield(&editor->entry_text, leaf_id("uph_automation_editor_entry_field"), UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY | UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE | UPH_UI_TEXTFIELD_FLAG_INLINE, NULL);
	}

	if (!submitted || !naui_key_pressed(NAUI_KEY_ENTER))
		return;

	if (editor->entry_text.length > 0)
		uph__automation_editor_set_value(editor, automation, (uint32_t)index, uph__automation_editor_from_display(automation->display, (float)atof(editor->entry_text.data)));

	uph__automation_editor_close_entry(editor);
}

#pragma endregion

void uph_automation_editor_init(void)
{
	Uph_AutomationEditorState *state = &uph__automation_editor_state;
	memset(state, 0, sizeof(*state));
	state->view.zoom = (Naui_Vec2) { NAUI_DPI(100), NAUI_DPI(30) };
	state->snap_resolution = UPH_SNAP_LINE;
	state->selected_point = -1;
	state->hovered_point = -1;
	state->hovered_handle = -1;
	state->menu_segment = -1;
	state->menu_point = -1;
	state->entry_index = -1;
	state->drag_index = -1;
	state->field_index = -1;
	state->fit = true;
}

void uph_automation_editor_reset_interaction(void)
{
	Uph_AutomationEditorState *state = &uph__automation_editor_state;
	uph__automation_editor_finish_drag(state);
	uph__automation_editor_commit_fields(state);
	uph__automation_editor_close_entry(state);
	state->panning = false;
	state->lane_hovered = false;
	state->hovered_point = -1;
	state->hovered_handle = -1;
	state->has_automation = false;
}

void uph_automation_editor_update(void)
{
	Uph_AutomationEditorState *state = &uph__automation_editor_state;
	const Uph_ResourceIndex index = uph_state.shared.selected_resource.index;

	if (index >= (Uph_ResourceIndex)naui_list_len(uph_state.project.automations))
	{
		uph_automation_editor_reset_interaction();
		uph__automation_editor_render_message("No automation is selected.");
		return;
	}

	if (!state->has_automation || state->automation_index != index)
	{
		uph_automation_editor_reset_interaction();
		state->automation_index = index;
		state->has_automation = true;
		state->fit = true;
		state->selected_point = -1;
		state->menu_segment = -1;
		state->menu_point = -1;
	}

	Uph_Automation *automation = &uph_state.project.automations[index];
	if (state->selected_point >= (int32_t)naui_list_len(automation->points))
		state->selected_point = -1;

	const Leaf_ID lane_id = leaf_id("uph_automation_editor_lane");
	state->panel_hovered = naui_panel_is_hovered(naui_panel_current());
	state->lane_box = leaf_get_bounding_box(lane_id);
	state->lane_hovered = state->panel_hovered && leaf_hovered(lane_id);

	const Uph_UIMenuID point_menu = uph_ui_context_menu(leaf_id("uph_automation_editor_point_menu"));
	const Uph_UIMenuID segment_menu = uph_ui_context_menu(leaf_id("uph_automation_editor_segment_menu"));
	uph__automation_editor_update_view(state, automation);
	uph__automation_editor_update_hover(state, automation);
	uph__automation_editor_update_mouse(state, automation, point_menu, segment_menu);
	uph__automation_editor_update_keys(state, automation);
	uph__automation_editor_update_point_menu(state, automation, point_menu);
	uph__automation_editor_update_segment_menu(state, automation, segment_menu);

	uph__automation_editor_render_info(automation);
	uph__automation_editor_render_toolbar(state, automation);

	const int32_t gutter_width = NAUI_DPI(UPH_AUTOMATION_EDITOR_GUTTER_WIDTH);
	const int32_t ruler_height = (int32_t)NAUI_DPI(UPH_AUTOMATION_EDITOR_RULER_HEIGHT);
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(ruler_height) },
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_BOTTOM,
			.color = uph_state.colors.automation_editor_ruler_border
		},
		.color = uph_state.colors.automation_editor_ruler_bg,
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FIXED(gutter_width), .height = LEAF_SIZE_FULL },
			.color = uph_state.colors.automation_editor_toolbox_bg
		});
		leaf({
			.custom_draw = (Leaf_CustomDrawFn)uph__automation_editor_ruler_draw,
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL }
		});
	}

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.clip_children = true
	})
	{
		leaf({
			.custom_draw = (Leaf_CustomDrawFn)uph__automation_editor_gutter_draw,
			.size = { .width = LEAF_SIZE_FIXED(gutter_width), .height = LEAF_SIZE_FULL },
			.color = uph_state.colors.automation_editor_toolbox_bg
		});
		leaf({
			.id = lane_id,
			.custom_draw = (Leaf_CustomDrawFn)uph__automation_editor_lane_draw,
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL }
		});
	}

	uph__automation_editor_render_entry(state, automation);
}