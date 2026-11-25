#define UPL_KNOB_START_ANGLE	2.35619449f
#define UPL_KNOB_SWEEP			4.71238898f
#define UPL_KNOB_DRAG_RANGE		200.0f
#define UPL_FINE_SCALE			0.1f
#define UPL_WHEEL_STEP			0.05f

typedef struct
{
	Upl_Plugin *current;
	Upl_Theme theme;
}
Upl_GuiState;

static Upl_GuiState upl__gui_state = {
	.theme = {
		.background = { 24, 25, 29, 255 },
		.panel = { 34, 36, 41, 255 },
		.track = { 52, 55, 62, 255 },
		.accent = { 255, 145, 77, 255 },
		.accent_hover = { 255, 172, 120, 255 },
		.thumb = { 230, 232, 236, 255 },
		.text = { 230, 232, 236, 255 },
		.text_dim = { 140, 145, 155, 255 },
		.font_size = 14,
		.rounding = 6
	}
};

#pragma region Context

Upl_Plugin *upl_current(void)
{
	return upl__gui_state.current;
}

float upl_scale(void)
{
	Upl_Plugin *plugin = upl__gui_state.current;
	return plugin ? plugin->gui.scale : 1.0f;
}

void upl_redraw(void)
{
	Upl_Plugin *plugin = upl__gui_state.current;
	if (plugin)
		upl__gui_mark_dirty(plugin);
}

Upl_Theme *upl_theme(void)
{
	return &upl__gui_state.theme;
}

#pragma endregion

#pragma region Input

static void upl__gui_mark_dirty(Upl_Plugin *plugin)
{
	plugin->gui.dirty_frames = 2;
}

static void upl__input_mouse_move(Upl_Plugin *plugin, int32_t x, int32_t y)
{
	Upl_Input *input = &plugin->gui.input;
	input->x = x;
	input->y = y;
	input->inside = true;
	upl__gui_mark_dirty(plugin);
}

static void upl__input_mouse_button(Upl_Plugin *plugin, Upl_MouseButton button, bool down)
{
	Upl_Input *input = &plugin->gui.input;
	if (button >= UPL_MOUSE_BUTTON_COUNT || input->down[button] == down)
		return;

	input->down[button] = down;
	upl__gui_mark_dirty(plugin);

	if (!down)
	{
		input->released[button] = true;
		return;
	}

	input->pressed[button] = true;

	const double now = upl__time();
	const int32_t dx = input->x - input->last_click_x;
	const int32_t dy = input->y - input->last_click_y;
	const bool close = dx * dx + dy * dy <= UPL_DOUBLE_CLICK_DISTANCE * UPL_DOUBLE_CLICK_DISTANCE;

	if (close && now - input->last_click_time[button] <= UPL_DOUBLE_CLICK_TIME)
	{
		input->double_clicked[button] = true;
		input->last_click_time[button] = 0.0;
	}
	else
	{
		input->last_click_time[button] = now;
	}

	input->last_click_x = input->x;
	input->last_click_y = input->y;
}

static void upl__input_mouse_wheel(Upl_Plugin *plugin, float delta)
{
	plugin->gui.input.wheel += delta;
	upl__gui_mark_dirty(plugin);
}

static void upl__input_mouse_leave(Upl_Plugin *plugin)
{
	plugin->gui.input.inside = false;
	upl__gui_mark_dirty(plugin);
}

static void upl__input_set_shift(Upl_Plugin *plugin, bool down)
{
	plugin->gui.input.shift = down;
}

static void upl__input_end_frame(Upl_Input *input)
{
	memset(input->pressed, 0, sizeof(input->pressed));
	memset(input->released, 0, sizeof(input->released));
	memset(input->double_clicked, 0, sizeof(input->double_clicked));
	input->wheel = 0.0f;
}

static Upl_Input *upl__input_current(void)
{
	Upl_Plugin *plugin = upl__gui_state.current;
	return plugin ? &plugin->gui.input : NULL;
}

int32_t upl_mouse_x(void)
{
	Upl_Input *input = upl__input_current();
	return input ? input->x : 0;
}

int32_t upl_mouse_y(void)
{
	Upl_Input *input = upl__input_current();
	return input ? input->y : 0;
}

bool upl_mouse_down(Upl_MouseButton button)
{
	Upl_Input *input = upl__input_current();
	return input && button < UPL_MOUSE_BUTTON_COUNT && input->down[button];
}

bool upl_mouse_pressed(Upl_MouseButton button)
{
	Upl_Input *input = upl__input_current();
	return input && button < UPL_MOUSE_BUTTON_COUNT && input->pressed[button];
}

bool upl_mouse_released(Upl_MouseButton button)
{
	Upl_Input *input = upl__input_current();
	return input && button < UPL_MOUSE_BUTTON_COUNT && input->released[button];
}

bool upl_mouse_double_clicked(Upl_MouseButton button)
{
	Upl_Input *input = upl__input_current();
	return input && button < UPL_MOUSE_BUTTON_COUNT && input->double_clicked[button];
}

float upl_mouse_wheel(void)
{
	Upl_Input *input = upl__input_current();
	return input ? input->wheel : 0.0f;
}

bool upl_shift_down(void)
{
	Upl_Input *input = upl__input_current();
	return input && input->shift;
}

#pragma endregion

#pragma region Frame

static Leaf_Dimensions upl__leaf_measure_text(const char *text, uint32_t length, int32_t font_size, const Leaf_TextConfig *config)
{
	return upl_measure_text(text, length, font_size, config->font_id);
}

static void upl__gui_resize_buffer(Upl_Plugin *plugin)
{
	Upl_Gui *gui = &plugin->gui;
	gui->width = (int32_t)ceilf((float)upl_descriptor.gui_width * gui->scale);
	gui->height = (int32_t)ceilf((float)upl_descriptor.gui_height * gui->scale);
	free(gui->pixels);
	gui->pixels = (uint32_t *)calloc((size_t)gui->width * (size_t)gui->height, sizeof(uint32_t));
	upl__gui_mark_dirty(plugin);
}

static void upl__gui_create(Upl_Plugin *plugin)
{
	Upl_Gui *gui = &plugin->gui;
	Leaf_Context *previous = leaf_ctx;
	leaf_init();
	leaf_set_measure_text(upl__leaf_measure_text);
	gui->leaf = leaf_ctx;
	leaf_ctx = previous;

	if (gui->scale <= 0.0f)
		gui->scale = 1.0f;

	gui->drag.param = -1;
	gui->input = (Upl_Input) { 0 };
	gui->created = true;
	upl__gui_resize_buffer(plugin);
}

static void upl__gui_destroy(Upl_Plugin *plugin)
{
	Upl_Gui *gui = &plugin->gui;
	if (gui->window)
		upl__window_destroy(plugin);

	if (gui->leaf)
	{
		Leaf_Context *previous = leaf_ctx;
		leaf_ctx = gui->leaf;
		leaf_shutdown();
		leaf_ctx = previous == gui->leaf ? NULL : previous;
	}

	free(gui->pixels);
	*gui = (Upl_Gui) { .scale = gui->scale };
}

static void upl__gui_frame(Upl_Plugin *plugin)
{
	Upl_GuiState *state = &upl__gui_state;
	Upl_Gui *gui = &plugin->gui;
	const bool tracking = gui->input.inside || gui->input.down[UPL_MOUSE_LEFT] || gui->input.down[UPL_MOUSE_RIGHT] || gui->input.down[UPL_MOUSE_MIDDLE];

	Leaf_Context *previous = leaf_ctx;
	leaf_ctx = gui->leaf;
	state->current = plugin;

	leaf_begin_frame(gui->width, gui->height);
	leaf_set_pointer_pos(tracking ? gui->input.x : INT32_MIN / 2, tracking ? gui->input.y : INT32_MIN / 2);
	if (upl_descriptor.ui)
		upl_descriptor.ui(plugin);
	const Leaf_RenderCmdList commands = leaf_end_frame();

	upl__renderer_begin(gui->pixels, gui->width, gui->height, state->theme.background);
	upl__render_commands(&commands);

	state->current = NULL;
	leaf_ctx = previous;
	upl__input_end_frame(&gui->input);
}

static void upl__gui_tick(Upl_Plugin *plugin)
{
	Upl_Gui *gui = &plugin->gui;
	if (!gui->created || !gui->window)
		return;

	upl__window_poll(plugin);

	if (atomic_exchange(&plugin->ui_dirty, false))
		upl__gui_mark_dirty(plugin);

	if (gui->dirty_frames == 0 || !gui->pixels)
		return;

	gui->dirty_frames--;
	upl__gui_frame(plugin);
	upl__window_present(plugin);
}

#pragma endregion

#pragma region Widgets

typedef struct
{
	float value;
	bool highlighted;
}
Upl_WidgetDrawData;

static bool upl__param_drag(Upl_Plugin *plugin, uint32_t param, Leaf_ID id, float range)
{
	Upl_Gui *gui = &plugin->gui;
	Upl_Input *input = &gui->input;
	bool changed = false;

	if (gui->drag.param < 0 && leaf_hovered(id))
	{
		if (input->double_clicked[UPL_MOUSE_LEFT])
		{
			upl_param_begin_gesture(plugin, param);
			upl_param_set(plugin, param, upl_descriptor.params[param].default_value);
			upl_param_end_gesture(plugin, param);
			return true;
		}

		if (input->pressed[UPL_MOUSE_LEFT])
		{
			gui->drag = (Upl_WidgetDrag) { (int32_t)param, (float)input->y, upl_param_normalized(plugin, param) };
			upl_param_begin_gesture(plugin, param);
		}
		else if (input->wheel != 0.0f)
		{
			const float step = input->shift ? UPL_WHEEL_STEP * UPL_FINE_SCALE : UPL_WHEEL_STEP;
			upl_param_begin_gesture(plugin, param);
			upl_param_set_normalized(plugin, param, upl_param_normalized(plugin, param) + input->wheel * step);
			upl_param_end_gesture(plugin, param);
			changed = true;
		}
	}

	if (gui->drag.param != (int32_t)param)
		return changed;

	const float delta = gui->drag.start_y - (float)input->y;
	gui->drag.start_y = (float)input->y;
	gui->drag.start_value += (double)(delta / (range * gui->scale)) * (input->shift ? UPL_FINE_SCALE : 1.0f);
	gui->drag.start_value = fmin(fmax(gui->drag.start_value, 0.0), 1.0);
	upl_param_set_normalized(plugin, param, gui->drag.start_value);

	if (!input->down[UPL_MOUSE_LEFT])
	{
		upl_param_end_gesture(plugin, param);
		gui->drag.param = -1;
	}

	return true;
}

static void upl__knob_draw(Leaf_BoundingBox box, void *user_data)
{
	const Upl_WidgetDrawData *data = (const Upl_WidgetDrawData *)user_data;
	const Upl_Theme *theme = &upl__gui_state.theme;
	const float scale = upl_scale();
	const float line_width = 4.0f * scale;
	const float cx = (float)box.x + (float)box.width * 0.5f;
	const float cy = (float)box.y + (float)box.height * 0.5f;
	const float radius = fminf((float)box.width, (float)box.height) * 0.5f - line_width;
	const float value_angle = UPL_KNOB_START_ANGLE + UPL_KNOB_SWEEP * data->value;
	const Leaf_Color accent = data->highlighted ? theme->accent_hover : theme->accent;

	upl_stroke_arc(cx, cy, radius, UPL_KNOB_START_ANGLE, UPL_KNOB_START_ANGLE + UPL_KNOB_SWEEP, theme->track, line_width);
	upl_stroke_arc(cx, cy, radius, UPL_KNOB_START_ANGLE, value_angle, accent, line_width);
	upl_fill_circle(cx, cy, radius - line_width * 1.5f, theme->panel);

	const float inner = radius - line_width * 2.5f;
	upl_draw_line(cx + cosf(value_angle) * inner * 0.35f, cy + sinf(value_angle) * inner * 0.35f, cx + cosf(value_angle) * inner, cy + sinf(value_angle) * inner, theme->thumb, 2.5f * scale);
}

static void upl__slider_draw(Leaf_BoundingBox box, void *user_data)
{
	const Upl_WidgetDrawData *data = (const Upl_WidgetDrawData *)user_data;
	const Upl_Theme *theme = &upl__gui_state.theme;
	const float track_height = fminf((float)box.height, 6.0f * upl_scale());
	const float thumb_radius = (float)box.height * 0.5f;
	const float usable = fmaxf((float)box.width - thumb_radius * 2.0f, 0.0f);
	const float thumb_x = (float)box.x + thumb_radius + usable * data->value;
	const float cy = (float)box.y + (float)box.height * 0.5f;
	const float track_x = (float)box.x + thumb_radius;

	upl_fill_rect(track_x, cy - track_height * 0.5f, usable, track_height, theme->track, track_height * 0.5f);
	upl_fill_rect(track_x, cy - track_height * 0.5f, thumb_x - track_x, track_height, data->highlighted ? theme->accent_hover : theme->accent, track_height * 0.5f);
	upl_fill_circle(thumb_x, cy, thumb_radius * 0.8f, theme->thumb);
}

static void upl__toggle_draw(Leaf_BoundingBox box, void *user_data)
{
	const Upl_WidgetDrawData *data = (const Upl_WidgetDrawData *)user_data;
	const Upl_Theme *theme = &upl__gui_state.theme;
	const float height = (float)box.height;
	const float radius = height * 0.5f;
	const float inset = 3.0f * upl_scale();
	const bool on = data->value >= 0.5f;
	const Leaf_Color background = on ? (data->highlighted ? theme->accent_hover : theme->accent) : theme->track;
	const float thumb_x = on ? (float)(box.x + box.width) - radius : (float)box.x + radius;

	upl_fill_rect((float)box.x, (float)box.y, (float)box.width, height, background, radius);
	upl_fill_circle(thumb_x, (float)box.y + radius, radius - inset, theme->thumb);
}

bool upl_knob(uint32_t param, int32_t size)
{
	Upl_Plugin *plugin = upl__gui_state.current;
	if (!plugin || param >= upl_descriptor.param_count)
		return false;

	const Leaf_ID id = leaf_id_indexed("upl_knob", param);
	const bool changed = upl__param_drag(plugin, param, id, UPL_KNOB_DRAG_RANGE);
	Upl_WidgetDrawData data = {
		.value = (float)upl_param_normalized(plugin, param),
		.highlighted = plugin->gui.drag.param == (int32_t)param || (plugin->gui.drag.param < 0 && leaf_hovered(id))
	};

	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIXED(size), .height = LEAF_SIZE_FIXED(size) },
		.custom_draw = upl__knob_draw,
		.custom_draw_data = LEAF_DATA_SLICE(data)
	}) {}

	return changed;
}

bool upl_slider(uint32_t param, int32_t width, int32_t height)
{
	Upl_Plugin *plugin = upl__gui_state.current;
	if (!plugin || param >= upl_descriptor.param_count)
		return false;

	Upl_Gui *gui = &plugin->gui;
	Upl_Input *input = &gui->input;
	const Leaf_ID id = leaf_id_indexed("upl_slider", param);
	const Leaf_BoundingBox box = leaf_get_bounding_box(id);
	const float thumb_radius = (float)box.height * 0.5f;
	const float usable = fmaxf((float)box.width - thumb_radius * 2.0f, 1.0f);
	bool changed = false;

	if (gui->drag.param < 0 && leaf_hovered(id))
	{
		if (input->double_clicked[UPL_MOUSE_LEFT])
		{
			upl_param_begin_gesture(plugin, param);
			upl_param_set(plugin, param, upl_descriptor.params[param].default_value);
			upl_param_end_gesture(plugin, param);
			changed = true;
		}
		else if (input->pressed[UPL_MOUSE_LEFT])
		{
			gui->drag = (Upl_WidgetDrag) { (int32_t)param, 0.0f, 0.0 };
			upl_param_begin_gesture(plugin, param);
		}
		else if (input->wheel != 0.0f)
		{
			const float step = input->shift ? UPL_WHEEL_STEP * UPL_FINE_SCALE : UPL_WHEEL_STEP;
			upl_param_begin_gesture(plugin, param);
			upl_param_set_normalized(plugin, param, upl_param_normalized(plugin, param) + input->wheel * step);
			upl_param_end_gesture(plugin, param);
			changed = true;
		}
	}

	if (gui->drag.param == (int32_t)param)
	{
		upl_param_set_normalized(plugin, param, ((float)input->x - (float)box.x - thumb_radius) / usable);
		changed = true;

		if (!input->down[UPL_MOUSE_LEFT])
		{
			upl_param_end_gesture(plugin, param);
			gui->drag.param = -1;
		}
	}

	Upl_WidgetDrawData data = {
		.value = (float)upl_param_normalized(plugin, param),
		.highlighted = gui->drag.param == (int32_t)param || (gui->drag.param < 0 && leaf_hovered(id))
	};

	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIXED(width), .height = LEAF_SIZE_FIXED(height) },
		.custom_draw = upl__slider_draw,
		.custom_draw_data = LEAF_DATA_SLICE(data)
	}) {}

	return changed;
}

bool upl_toggle(uint32_t param, int32_t width, int32_t height)
{
	Upl_Plugin *plugin = upl__gui_state.current;
	if (!plugin || param >= upl_descriptor.param_count)
		return false;

	const Leaf_ID id = leaf_id_indexed("upl_toggle", param);
	const bool hovered = plugin->gui.drag.param < 0 && leaf_hovered(id);
	bool changed = false;

	if (hovered && plugin->gui.input.pressed[UPL_MOUSE_LEFT])
	{
		const bool on = upl_param_normalized(plugin, param) >= 0.5;
		upl_param_begin_gesture(plugin, param);
		upl_param_set_normalized(plugin, param, on ? 0.0 : 1.0);
		upl_param_end_gesture(plugin, param);
		changed = true;
	}

	Upl_WidgetDrawData data = {
		.value = (float)upl_param_normalized(plugin, param),
		.highlighted = hovered
	};

	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIXED(width), .height = LEAF_SIZE_FIXED(height) },
		.custom_draw = upl__toggle_draw,
		.custom_draw_data = LEAF_DATA_SLICE(data)
	}) {}

	return changed;
}

void upl_param_label(uint32_t param, Leaf_Color color, int32_t font_size)
{
	Upl_Plugin *plugin = upl__gui_state.current;
	if (!plugin || param >= upl_descriptor.param_count)
		return;

	char text[64];
	upl_param_text(plugin, param, upl_param(plugin, param), text, sizeof(text));
	leaf_text(text, { .color = color, .font_size = LEAF_SIZE_FIXED(font_size) });
}

#pragma endregion
