#define UPH_DRAG_THRESHOLD 4.0f

typedef uint8_t Uph_DragState;
enum
{
	UPH_DRAG_NONE,
	UPH_DRAG_PRESSED,
	UPH_DRAG_DRAGGING,
	UPH_DRAG_DROPPED
};

static struct
{
	_Alignas(max_align_t) unsigned char payload[UPH_DRAG_PAYLOAD_MAX];
	const char *type;
	Uph_DragRenderFn render;
	Naui_PanelID origin;
	Naui_Vec2 press;
	Uph_DragState state;
}
s_uph_drag;

void uph_drag_begin(const char *type, const void *payload, size_t size, Uph_DragRenderFn render)
{
	if (!type || size > UPH_DRAG_PAYLOAD_MAX)
		return;

	memset(s_uph_drag.payload, 0, sizeof(s_uph_drag.payload));
	if (payload)
		memcpy(s_uph_drag.payload, payload, size);

	s_uph_drag.type = type;
	s_uph_drag.render = render;
	s_uph_drag.origin = naui_panel_current();
	s_uph_drag.press = (Naui_Vec2){ (float)naui_mouse_x(), (float)naui_mouse_y() };
	s_uph_drag.state = UPH_DRAG_PRESSED;
}

static const void *_uph_drag_payload_in(Uph_DragState state, const char *type)
{
	return s_uph_drag.state == state && type && strcmp(s_uph_drag.type, type) == 0 ? s_uph_drag.payload : NULL;
}

const void *uph_drag_hover(const char *type)
{
	return _uph_drag_payload_in(UPH_DRAG_DRAGGING, type);
}

const void *uph_drag_drop(const char *type)
{
	const void *payload = _uph_drag_payload_in(UPH_DRAG_DROPPED, type);
	if (payload)
		s_uph_drag.state = UPH_DRAG_NONE;
	return payload;
}

void uph_drag_label(const Naui_Image *icon, const char *text, Naui_Color accent)
{
	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const float icon_size = NAUI_DPI(14.0f);

	leaf({
		.size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(8.0f), NAUI_DPI(5.0f)),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(6.0f),
		.color = {naui_theme_color("uph_ui_dropdown_bg_color")},
		.border = {
			.width = NAUI_DPI(1),
			.sides = LEAF_SIDE_ALL,
			.color = {accent}
		},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.shadow = {
			.blur_radius = NAUI_DPI(12.0f),
			.color = naui_theme_color("uph_ui_dropdown_shadow_color")
		}
	})
	{
		if (icon)
		{
			leaf({
				.size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
				.color = {accent},
				.image = (void*)icon
			});
		}

		leaf_text(text, {
			.font_size = {font_size},
			.color = {naui_theme_color("uph_ui_text_color")}
		});
	}
}

void uph_drag_flush(void)
{
	if (s_uph_drag.state == UPH_DRAG_DROPPED || s_uph_drag.state == UPH_DRAG_NONE)
	{
		s_uph_drag.state = UPH_DRAG_NONE;
		return;
	}

	if (naui_key_pressed(NAUI_KEY_ESCAPE) || naui_mouse_pressed(NAUI_MOUSE_RIGHT) || uph_dialog_blocks_input())
	{
		s_uph_drag.state = UPH_DRAG_NONE;
		return;
	}

	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
	{
		const bool over_origin = s_uph_drag.origin && naui_panel_is_hovered(s_uph_drag.origin);
		s_uph_drag.state = s_uph_drag.state == UPH_DRAG_DRAGGING && !over_origin ? UPH_DRAG_DROPPED : UPH_DRAG_NONE;
		return;
	}

	if (s_uph_drag.state == UPH_DRAG_PRESSED)
	{
		const float dx = (float)naui_mouse_x() - s_uph_drag.press.x;
		const float dy = (float)naui_mouse_y() - s_uph_drag.press.y;
		const float threshold = NAUI_DPI(UPH_DRAG_THRESHOLD);
		if (dx * dx + dy * dy < threshold * threshold)
			return;

		s_uph_drag.state = UPH_DRAG_DRAGGING;
	}

	naui_set_cursor(NAUI_CURSOR_HAND);
	if (!s_uph_drag.render)
		return;

	leaf({
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
		.floating = {
			.offset = {(float)naui_mouse_x() + NAUI_DPI(14.0f), (float)naui_mouse_y() + NAUI_DPI(10.0f)}
		},
		.size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT}
	})
	{
		s_uph_drag.render(s_uph_drag.payload);
	}
}
