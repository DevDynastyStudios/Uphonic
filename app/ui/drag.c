#define UPH_DRAG_THRESHOLD 4
#define UPH_DRAG_PAYLOAD_MAX 4608

typedef void (*Uph_DragRenderFn)(const void *payload);

typedef uint8_t Uph_DragPhase;
enum
{
	UPH_DRAG_NONE,
	UPH_DRAG_PRESSED,
	UPH_DRAG_DRAGGING,
	UPH_DRAG_DROPPED
};

typedef struct
{
	_Alignas(max_align_t) unsigned char payload[UPH_DRAG_PAYLOAD_MAX];
	const char *type;
	Uph_DragRenderFn render;
	Naui_PanelID origin;
	Naui_Vec2 press;
	Uph_DragPhase phase;
}
Uph_DragState;

static Uph_DragState _uph_drag_state;

void uph_drag_begin(const char *type, const void *payload, size_t size, Uph_DragRenderFn render)
{
	Uph_DragState *state = &_uph_drag_state;
	if (!type || size > UPH_DRAG_PAYLOAD_MAX)
		return;

	memset(state->payload, 0, sizeof(state->payload));
	if (payload)
		memcpy(state->payload, payload, size);

	state->type = type;
	state->render = render;
	state->origin = naui_panel_current();
	state->press = (Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() };
	state->phase = UPH_DRAG_PRESSED;
}

static const void *_uph_drag_payload_in(Uph_DragPhase phase, const char *type)
{
	return _uph_drag_state.phase == phase && type && strcmp(_uph_drag_state.type, type) == 0 ? _uph_drag_state.payload : NULL;
}

const void *uph_drag_hover(const char *type)
{
	return _uph_drag_payload_in(UPH_DRAG_DRAGGING, type);
}

const void *uph_drag_drop(const char *type)
{
	Uph_DragState *state = &_uph_drag_state;
	const void *payload = _uph_drag_payload_in(UPH_DRAG_DROPPED, type);
	if (payload)
		state->phase = UPH_DRAG_NONE;
	return payload;
}

void uph_drag_label(const Naui_Image *icon, const char *text, Naui_Color accent)
{
	const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const int32_t icon_size = NAUI_DPI(14);

	leaf({
		.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(8), NAUI_DPI(5)),
		.border = {
			.width = NAUI_DPI(1),
			.sides = LEAF_SIDE_ALL,
			.color = accent
		},
		.color = naui_theme_color("uph_ui_dropdown_bg_color"),
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.shadow = {
			.blur_radius = NAUI_DPI(12),
			.color = naui_theme_color("uph_ui_dropdown_shadow_color")
		},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.child_gap = NAUI_DPI(6),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		if (icon)
		{
			leaf({
				.image = (void*)icon,
				.size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
				.color = accent
			});
		}

		leaf_text(text, {
			.color = naui_theme_color("uph_ui_text_color"),
			.font_size = LEAF_SIZE_FIXED(font_size)
		});
	}
}

void uph_drag_flush(void)
{
	Uph_DragState *state = &_uph_drag_state;
	if (state->phase == UPH_DRAG_DROPPED || state->phase == UPH_DRAG_NONE)
	{
		state->phase = UPH_DRAG_NONE;
		return;
	}

	if (naui_key_pressed(NAUI_KEY_ESCAPE) || naui_mouse_pressed(NAUI_MOUSE_RIGHT) || uph_dialog_blocks_input())
	{
		state->phase = UPH_DRAG_NONE;
		return;
	}

	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
	{
		const bool over_origin = state->origin && naui_panel_is_hovered(state->origin);
		state->phase = state->phase == UPH_DRAG_DRAGGING && !over_origin ? UPH_DRAG_DROPPED : UPH_DRAG_NONE;
		return;
	}

	if (state->phase == UPH_DRAG_PRESSED)
	{
		const float dx = (float)naui_mouse_x() - state->press.x;
		const float dy = (float)naui_mouse_y() - state->press.y;
		const int32_t threshold = NAUI_DPI(UPH_DRAG_THRESHOLD);
		if (dx * dx + dy * dy < threshold * threshold)
			return;

		state->phase = UPH_DRAG_DRAGGING;
	}

	naui_set_cursor(NAUI_CURSOR_HAND);
	if (!state->render)
		return;

	leaf({
		.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
		.floating = {
			.offset = { .x = (float)naui_mouse_x() + NAUI_DPI(14), .y = (float)naui_mouse_y() + NAUI_DPI(10) }
		},
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
	})
	{
		state->render(state->payload);
	}
}
