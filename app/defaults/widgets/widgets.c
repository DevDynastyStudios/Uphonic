typedef struct
{
	float fraction;
	Leaf_Color track;
	Leaf_Color fill;
} Naui_KnobDrawData;

typedef struct
{
	uint64_t id;
	int32_t cursor;
} Naui_FieldEntry;

#define NAUI_DRAG_ARROW_W 20.0f

#define NAUI_KNOB_START_DEG 225.0f
#define NAUI_KNOB_END_DEG 315.0f

#define NAUI_FIELD_TABLE_SIZE 64

static uint32_t g_font_index = 0;
static Naui_FieldEntry g_field_table[NAUI_FIELD_TABLE_SIZE];

static uint16_t g_btn_counter;
static uint16_t g_toggle_counter;
static uint16_t g_dropdown_counter;
static uint16_t g_slider_int_counter;
static uint16_t g_slider_float_counter;
static uint16_t g_drag_int_counter;
static uint16_t g_drag_float_counter;
static uint16_t g_text_field_counter;
static uint16_t g_knob_counter;

static uint64_t g_active_dropdown_id = 0;

static uint64_t g_active_slider_id = 0;
static float g_track_origin_px = 0.0f;
static float g_track_width_px = 1.0f;

static uint64_t g_active_drag_id = 0;
static int32_t g_drag_last_x = 0;
static float g_drag_accum = 0.0f;

static uint64_t g_active_knob_id = 0;
static int32_t g_knob_last_y = 0;

static uint64_t g_focused_field_id = 0;

static inline Leaf_Color tw(const char *name) { return naui_theme_color(name); }
static inline float tf(const char *name) { return naui_theme_float(name); }

static Naui_FieldEntry *naui_field_entry(uint64_t id)
{
	uint32_t slot = (uint32_t)(id % NAUI_FIELD_TABLE_SIZE);
	for (uint32_t i = 0; i < NAUI_FIELD_TABLE_SIZE; i++)
	{
		uint32_t s = (slot + i) % NAUI_FIELD_TABLE_SIZE;
		if (g_field_table[s].id == id || g_field_table[s].id == 0)
		{
			g_field_table[s].id = id;
			return &g_field_table[s];
		}
	}
	return &g_field_table[0];
}

static void naui_fmt_float(char *buf, size_t buf_size, float value, const char *fmt)
{
	snprintf(buf, buf_size, fmt ? fmt : "%.3f", (double)value);
}

static void naui_track_geometry(Leaf_ID id, float label_w, float value_w, float *out_origin, float *out_width)
{
	Leaf_BoundingBox bb = leaf_get_bounding_box(id);
	float pad_h = tf("naui_widget_padding_h");
	float gap = 8.0f;
	*out_origin = bb.x + pad_h + label_w + gap;

	float w = bb.width - (pad_h * 2.0f) - label_w - (gap * 2.0f) - value_w;
	*out_width = w > 1.0f ? w : 1.0f;
}

static void naui_widget_slider_layout(Leaf_ID id, const char *label, const char *val_str, float fraction, bool hovered, bool dragging)
{
	Leaf_Color bg = dragging ? tw("naui_widget_surface_active_color") : hovered ? tw("naui_widget_surface_hovered_color") : tw("naui_widget_surface_color");
	Leaf_Color fill = dragging ? tw("naui_widget_accent_color") : tw("naui_widget_accent_dim_color");

	float h = tf("naui_widget_height");
	float pad_h = tf("naui_widget_padding_h");
	float label_w = tf("naui_widget_label_width");
	float rounding = tf("naui_widget_rounding");
	float font_sz = tf("naui_widget_font_size");
	float border_w = tf("naui_panel_border_width");
	fraction = naui_clamp(fraction, 0.0f, 1.0f);

	leaf({
		.id = id,
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(h) },
		.padding = LEAF_PADDING_AXES(pad_h, 0),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.child_gap = 8,
		.color = { .color1 = bg },
		.border = {
			.width = border_w,
			.sides = LEAF_SIDE_ALL,
			.color = { .color1 = tw("naui_widget_border_color") }
		},
		.rounding = { rounding, LEAF_CORNER_ALL },
	})
	{
		leaf({
			.size = { LEAF_SIZE_FIXED(label_w), LEAF_SIZE_FULL },
			.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		})
		{
			leaf_text(label, {
				.font_id = g_font_index,
				.color = { .color1 = tw("naui_widget_text_dim_color") },
				.font_size = LEAF_SIZE_FIXED(font_sz),
			});
		}

		leaf({
			.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(5) },
			.color = { .color1 = tw("naui_widget_bg_color") },
			.rounding = { 3, LEAF_CORNER_ALL },
			.clip_children = true,
		})
		{
			leaf({
				.size = { LEAF_SIZE_PERCENT(fraction), LEAF_SIZE_FULL },
				.color = { .color1 = fill },
				.rounding = { 3, LEAF_CORNER_TL | LEAF_CORNER_BL },
			}) {}
		}

		leaf({
			.size = { LEAF_SIZE_FIXED(52), LEAF_SIZE_FULL },
			.child_alignment = { LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER },
		})
		{
			leaf_text(val_str, {
				.font_id = g_font_index,
				.color = { .color1 = tw("naui_widget_text_color") },
				.font_size = LEAF_SIZE_FIXED(font_sz),
				.alignment = LEAF_TEXT_ALIGN_RIGHT,
			});
		}
	}
}

static void naui_widget_drag_layout(Leaf_ID id, const char *label, const char *val_str, bool hovered, bool dragging)
{
	Leaf_Color bg = dragging ? tw("naui_widget_surface_active_color") : hovered ? tw("naui_widget_surface_hovered_color") : tw("naui_widget_surface_color");

	float h = tf("naui_widget_height");
	float pad_h = tf("naui_widget_padding_h");
	float label_w = tf("naui_widget_label_width");
	float rounding = tf("naui_widget_rounding");
	float font_sz = tf("naui_widget_font_size");
	float border_w = tf("naui_panel_border_width");

	leaf({
		.id = id,
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(h) },
		.padding = LEAF_PADDING_AXES(pad_h, 0),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.child_gap = 0,
		.color = { .color1 = bg },
		.border = {
			.width = border_w,
			.sides = LEAF_SIDE_ALL,
			.color = { .color1 = tw("naui_widget_border_color") }
		},
		.rounding = { rounding, LEAF_CORNER_ALL },
	})
	{
		leaf({
			.size = { LEAF_SIZE_FIXED(label_w + pad_h), LEAF_SIZE_FULL },
			.padding = LEAF_PADDING_AXES(0, 0),
			.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		})
		{
			leaf_text(label, {
				.font_id = g_font_index,
				.color = { .color1 = tw("naui_widget_text_dim_color") },
				.font_size = LEAF_SIZE_FIXED(font_sz),
			});
		}

		leaf({
			.id = { .value = 0 },
			.size = { LEAF_SIZE_GROW, LEAF_SIZE_FULL },
			.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER },
		})
		{
			leaf_text(val_str, {
				.font_id = g_font_index,
				.color = { .color1 = dragging ? tw("naui_widget_accent_color") : tw("naui_widget_text_color") },
				.font_size = LEAF_SIZE_FIXED(font_sz),
				.alignment = LEAF_TEXT_ALIGN_CENTER,
			});
		}
	}
}

static void naui_widget_text_render(const char *text)
{
	leaf_text(text, {
		.font_id = g_font_index,
		.color = { .color1 = tw("naui_widget_text_color") },
		.font_size = LEAF_SIZE_FIXED(tf("naui_widget_font_size")),
	});
}

static void naui_widget_button_render(Leaf_ID id, const char *label)
{
	bool hovered = leaf_hovered(id);
	bool pressed = hovered && naui_mouse_down(NAUI_MOUSE_LEFT);

	Leaf_Color bg = pressed ? tw("naui_widget_surface_active_color") : hovered ? tw("naui_widget_surface_hovered_color") : tw("naui_widget_surface_color");

	leaf({
		.id = id,
		.size = { LEAF_SIZE_FIT, LEAF_SIZE_FIXED(tf("naui_widget_height")) },
		.padding = LEAF_PADDING_AXES(tf("naui_widget_padding_h"), 0),
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER },
		.color = { .color1 = bg },
		.rounding = { tf("naui_widget_rounding"), LEAF_CORNER_ALL },
	})
	{
		leaf_text(label, {
			.font_id = g_font_index,
			.color = { .color1 = tw("naui_widget_text_color") },
			.font_size = LEAF_SIZE_FIXED(tf("naui_widget_font_size")),
			.alignment = LEAF_TEXT_ALIGN_CENTER,
		});
	}
}

static void naui_widget_toggle_render(Leaf_ID id, const char *label, bool *value)
{
	bool hovered = leaf_hovered(id);
	Leaf_Color track = *value ? tw("naui_widget_toggle_on_color") : tw("naui_widget_toggle_off_color");
	if (hovered)
	{
		track.r = (uint8_t)naui_clamp((float)track.r + 20.0f, 0.0f, 255.0f);
		track.g = (uint8_t)naui_clamp((float)track.g + 20.0f, 0.0f, 255.0f);
		track.b = (uint8_t)naui_clamp((float)track.b + 20.0f, 0.0f, 255.0f);
	}

	leaf({
		.id = id,
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(tf("naui_widget_height")) },
		.padding = LEAF_PADDING_AXES(tf("naui_widget_padding_h"), 0),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.child_gap = 8,
		.color = { .color1 = tw("naui_widget_surface_color") },
		.rounding = { tf("naui_widget_rounding"), LEAF_CORNER_ALL },
	})
	{
		leaf({
			.size = { LEAF_SIZE_FIXED(34), LEAF_SIZE_FIXED(16) },
			.padding = LEAF_PADDING_ALL(2),
			.child_alignment = { (Leaf_LayoutAlignmentX)(*value ? LEAF_ALIGN_X_RIGHT : LEAF_ALIGN_X_LEFT), LEAF_ALIGN_Y_CENTER },
			.color = { .color1 = track },
			.rounding = { 8, LEAF_CORNER_ALL },
		})
		{
			leaf({
				.size = { LEAF_SIZE_FIXED(12), LEAF_SIZE_FIXED(12) },
				.color = { .color1 = leaf_rgb(255, 255, 255) },
				.rounding = { 6, LEAF_CORNER_ALL },
			}) {}
		}

		leaf({
			.size = { LEAF_SIZE_FIT, LEAF_SIZE_FIT },
			.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		})
		{
			leaf_text(label, {
				.font_id = g_font_index,
				.color = { .color1 = tw("naui_widget_text_color") },
				.font_size = LEAF_SIZE_FIXED(tf("naui_widget_font_size")),
				.alignment = LEAF_TEXT_ALIGN_LEFT,
			});
		}
	}
}

static void naui_widget_dropdown_render(Leaf_ID id, const char* label, uint32_t* select, const char** items, uint32_t item_count)
{
	bool hovered = leaf_hovered(id);
	bool panel_hovered = naui_panel_hovered(naui_current_panel());
	bool clicked = naui_mouse_clicked(NAUI_MOUSE_LEFT);

	if (panel_hovered && hovered && clicked)
	{
		if (g_active_dropdown_id == id.value)
			g_active_dropdown_id = 0;
		else
			g_active_dropdown_id = id.value;
	}

	bool open = (g_active_dropdown_id == id.value);
	leaf({
		.id = id,
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(tf("naui_widget_height")) },
		.padding = LEAF_PADDING_AXES(tf("naui_widget_padding_h"), 0),
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.color = { .color1 = tw("naui_widget_surface_color") },
		.rounding = { tf("naui_widget_rounding"), LEAF_CORNER_ALL },
	});
}

static void naui_widget_slider_int_render(Leaf_ID id, const char *label, int32_t *value, int32_t min, int32_t max)
{
	bool hovered = leaf_hovered(id);
	bool panel_hovered = naui_panel_hovered(naui_current_panel());
	bool dragging = (g_active_slider_id == id.value);

	if (panel_hovered && hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT))
	{
		g_active_slider_id = id.value;
		naui_track_geometry(id, tf("naui_widget_label_width"), 52.0f, &g_track_origin_px, &g_track_width_px);
	}

	if (panel_hovered && dragging)
	{
		if (naui_mouse_down(NAUI_MOUSE_LEFT))
		{
			float t = ((float)naui_mouse_x() - g_track_origin_px) / g_track_width_px;
			t = naui_clamp(t, 0.0f, 1.0f);
			*value = min + (int32_t)(t * (float)(max - min) + 0.5f);
		}
		else
		{
			g_active_slider_id = 0;
		}
	}

	float range = (float)(max - min);
	float fraction = (range > 0.0f) ? ((float)(*value - min) / range) : 0.0f;

	char buf[32];
	snprintf(buf, sizeof(buf), "%d", *value);
	naui_widget_slider_layout(id, label, buf, fraction, hovered, dragging);
}

static void naui_widget_slider_float_render(Leaf_ID id, const char *label, float *value, float min, float max, const char *fmt)
{
	bool hovered = leaf_hovered(id);
	bool panel_hovered = naui_panel_hovered(naui_current_panel());
	bool dragging = (g_active_slider_id == id.value);

	if (panel_hovered && hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT))
	{
		g_active_slider_id = id.value;
		naui_track_geometry(id, tf("naui_widget_label_width"), 52.0f, &g_track_origin_px, &g_track_width_px);
	}

	if (panel_hovered && dragging)
	{
		if (naui_mouse_down(NAUI_MOUSE_LEFT))
		{
			float t = ((float)naui_mouse_x() - g_track_origin_px) / g_track_width_px;
			t = naui_clamp(t, 0.0f, 1.0f);
			*value = naui_lerp(min, max, t);
		}
		else
		{
			g_active_slider_id = 0;
		}
	}

	float range = max - min;
	float fraction = (range > 0.0f) ? ((*value - min) / range) : 0.0f;

	char buf[32];
	naui_fmt_float(buf, sizeof(buf), *value, fmt);
	naui_widget_slider_layout(id, label, buf, fraction, hovered, dragging);
}

static void naui_widget_drag_int_render(Leaf_ID id, const char *label, int *value, int speed, int min, int max)
{
	bool hovered = leaf_hovered(id);
	bool panel_hovered = naui_panel_hovered(naui_current_panel());
	bool dragging = (g_active_drag_id == id.value);
	float origin_px, width_px;
	naui_track_geometry(id, tf("naui_widget_label_width") + tf("naui_widget_padding_h"), NAUI_DRAG_ARROW_W * 2.0f, &origin_px, &width_px);
	float range = (min != max) ? (float)(max - min) : (float)(speed * 200);

	if (panel_hovered && hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT))
	{
		g_active_drag_id = id.value;
		g_drag_last_x = naui_mouse_x();
		g_drag_accum = 0.0f;
	}

	if (panel_hovered && dragging)
	{
		if (naui_mouse_down(NAUI_MOUSE_LEFT))
		{
			int32_t mouse_x = naui_mouse_x();
			float rate = naui_key_down(NAUI_KEY_CONTROL) ? 0.1f : 1.0f;
			float delta = (float)(mouse_x - g_drag_last_x) * rate * (range / width_px);
			g_drag_last_x = mouse_x;
			g_drag_accum += delta;

			int32_t whole = (int32_t)g_drag_accum;
			if (whole != 0)
			{
				g_drag_accum -= (float)whole;
				int32_t next = *value + whole;
				if (min != max)
					next = (int32_t)naui_clamp((float)next, (float)min, (float)max);

				*value = next;
			}
		}
		else
		{
			g_active_drag_id = 0;
			g_drag_accum = 0.0f;
		}
	}

	char buf[32];
	snprintf(buf, sizeof(buf), "%d", *value);
	naui_widget_drag_layout(id, label, buf, hovered, dragging);
}

static void naui_widget_drag_float_render(Leaf_ID id, const char *label, float *value, float speed, float min, float max, const char *fmt)
{
	bool hovered = leaf_hovered(id);
	bool dragging = (g_active_drag_id == id.value);
	float origin_px, width_px;
	naui_track_geometry(id, tf("naui_widget_label_width") + tf("naui_widget_padding_h"), NAUI_DRAG_ARROW_W * 2.0f, &origin_px, &width_px);
	float range = (min != max) ? (max - min) : (speed * 200.0f);

	if (hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT))
	{
		g_active_drag_id = id.value;
		g_drag_last_x = naui_mouse_x();
	}

	if (dragging)
	{
		if (naui_mouse_down(NAUI_MOUSE_LEFT))
		{
			int32_t mouse_x = naui_mouse_x();
			float rate = naui_key_down(NAUI_KEY_CONTROL) ? 0.1f : 1.0f;
			float delta = (float)(mouse_x - g_drag_last_x) * rate * (range / width_px);
			g_drag_last_x = mouse_x;
			float next = *value + delta;
			if (min != max)
				next = naui_clamp(next, min, max);

			*value = next;
		}
		else
		{
			g_active_drag_id = 0;
		}
	}

	char buf[32];
	naui_fmt_float(buf, sizeof(buf), *value, fmt);
	naui_widget_drag_layout(id, label, buf, hovered, dragging);
}

static void naui_widget_text_field_render(Leaf_ID id, const char *label, const char* hint, char *buffer, size_t buffer_size)
{
	bool hovered = leaf_hovered(id);
	bool focused = (g_focused_field_id == id.value);
	Naui_FieldEntry *entry = naui_field_entry(id.value);

	if (naui_mouse_clicked(NAUI_MOUSE_LEFT))
	{
		if (hovered)
		{
			g_focused_field_id = id.value;
			entry->cursor = (int32_t)strlen(buffer);
		}
		else if (focused)
		{
			g_focused_field_id = 0;
		}
	}

	float h = tf("naui_widget_height");
	float pad_h = tf("naui_widget_padding_h");
	float label_w = tf("naui_widget_label_width");
	float rounding = tf("naui_widget_rounding");
	float font_sz = tf("naui_widget_font_size");
	float border_w = tf("naui_panel_border_width");
	Leaf_Color border_col = focused ? tw("naui_widget_accent_color") : tw("naui_widget_border_color");

	leaf({
		.id = id,
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(h) },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.child_gap = 8,
		.color = { .color1 = leaf_rgba(0, 0, 0, 0) },
	})
	{
		leaf({
			.size = { LEAF_SIZE_FIXED(label_w), LEAF_SIZE_FULL },
			.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		})
		{
			leaf_text(label, {
				.font_id = g_font_index,
				.color = { .color1 = tw("naui_widget_text_dim_color") },
				.font_size = LEAF_SIZE_FIXED(font_sz),
			});
		}

		leaf({
			.size = { LEAF_SIZE_GROW, LEAF_SIZE_FULL },
			.padding = LEAF_PADDING_AXES(pad_h, 0),
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
			.child_gap = 0,
			.color = { .color1 = focused ? tw("naui_widget_surface_active_color") : tw("naui_widget_surface_color") },
			.border = {
				.width = border_w,
				.sides = LEAF_SIDE_ALL,
				.color = { .color1 = border_col }
			},
			.rounding = { rounding, LEAF_CORNER_ALL },
			.clip_children = true,
		})
		{
			bool empty = (buffer[0] == '\0');
			Leaf_Color text_col = (empty && !focused) ? tw("naui_widget_text_dim_color") : tw("naui_widget_text_color");
			const char* hint_text = hint ? hint : "";

			leaf_text(empty && !focused ? hint_text : "", {
				.font_id = g_font_index,
				.color = { .color1 = text_col },
				.font_size = LEAF_SIZE_FIXED(font_sz),
			});

			if (focused)
			{
				leaf({
					.size = { LEAF_SIZE_FIXED(1), LEAF_SIZE_FIXED(h * 0.6f) },
					.color = { .color1 = tw("naui_widget_accent_color") },
				});
			}
		}
	}
}

static void naui_knob_draw(Leaf_BoundingBox bb, void *user_data)
{
	// This is called by the renderer backend — implementation is
	// backend-specific (OpenGL, software, etc.). The contract:
	//
	// Draw a thick circular arc centred in `bb`.
	// Start angle : NAUI_KNOB_START_DEG degrees (clockwise from right)
	// End angle : NAUI_KNOB_START_DEG + 270° * fraction
	// Track arc : NAUI_KNOB_START_DEG..NAUI_KNOB_START_DEG+270°
	//				 in data->track color
	// Value arc : overlaid in data->fill color
	//
	// A small tick / dot at the value position indicates the
	// current angle.
	//
	// The renderer registers this via Leaf_CustomDrawFn. No leaf
	// primitives are used here — leaf just reserves the bounding box.
	(void)bb;
	(void)user_data;
}

static void naui_widget_knob_render(Leaf_ID id, const char *label, float *value, float min, float max, const char *fmt)
{
	bool hovered = leaf_hovered(id);
	bool dragging = (g_active_knob_id == id.value);

	if (hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT))
	{
		g_active_knob_id = id.value;
		g_knob_last_y = naui_mouse_y();
	}

	if (dragging)
	{
		if (naui_mouse_down(NAUI_MOUSE_LEFT))
		{
			int32_t mouse_y = naui_mouse_y();
			float rate = naui_key_down(NAUI_KEY_CONTROL) ? 0.001f : 0.01f;
			float delta = (float)(g_knob_last_y - mouse_y) * rate * (max - min);
			g_knob_last_y = mouse_y;
			*value = naui_clamp(*value + delta, min, max);
		}
		else
		{
			g_active_knob_id = 0;
		}
	}

	float range = max - min;
	float fraction = (range > 0.0f) ? ((*value - min) / range) : 0.0f;
	float knob_sz = tf("naui_widget_height") * 2.0f;
	float font_sz = tf("naui_widget_font_size");
	float rounding = tf("naui_widget_rounding");
	float border_w = tf("naui_panel_border_width");

	Leaf_Color bg = dragging ? tw("naui_widget_surface_active_color") : hovered ? tw("naui_widget_surface_hovered_color") : tw("naui_widget_surface_color");

	static Naui_KnobDrawData draw_data;
	draw_data.fraction = fraction;
	draw_data.track = tw("naui_widget_bg_color");
	draw_data.fill = dragging ? tw("naui_widget_accent_color") : tw("naui_widget_accent_dim_color");

	char buf[32];
	naui_fmt_float(buf, sizeof(buf), *value, fmt);

	leaf({
		.id = id,
		.size = { LEAF_SIZE_FIT, LEAF_SIZE_FIT },
		.direction = LEAF_DIRECTION_VERTICAL,
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP },
		.child_gap = 4,
		.padding = LEAF_PADDING_ALL(6),
		.color = { .color1 = bg },
		.rounding = { rounding, LEAF_CORNER_ALL },
		.border = {
			.width = border_w,
			.sides = LEAF_SIDE_ALL,
			.color = { .color1 = tw("naui_widget_border_color") }
		},
	})
	{
		leaf({
			.size = { LEAF_SIZE_FIXED(knob_sz), LEAF_SIZE_FIXED(knob_sz) },
			.custom_draw = naui_knob_draw,
			.custom_draw_data = LEAF_DATA_SLICE(draw_data),
		}) {}

		leaf_text(buf, {
			.font_id = g_font_index,
			.color = { .color1 = tw("naui_widget_text_color") },
			.font_size = LEAF_SIZE_FIXED(font_sz - 1.0f),
			.alignment = LEAF_TEXT_ALIGN_CENTER,
		});

		leaf_text(label, {
			.font_id = g_font_index,
			.color = { .color1 = tw("naui_widget_text_dim_color") },
			.font_size = LEAF_SIZE_FIXED(font_sz - 2.0f),
			.alignment = LEAF_TEXT_ALIGN_CENTER,
		});
	}
}

void naui_widgets_reset(void)
{
	g_btn_counter = 0;
	g_toggle_counter = 0;
	g_slider_int_counter = 0;
	g_slider_float_counter = 0;
	g_drag_int_counter = 0;
	g_drag_float_counter = 0;
	g_text_field_counter = 0;
	g_knob_counter = 0;
}

void naui_widget_font_set(uint32_t font_index)
{
	g_font_index = font_index;
}

void naui_text(const char *text)
{
	naui_widget_text_render(text);
}

bool naui_button(const char *label)
{
	Leaf_ID id = leaf_id_indexed("__naui_btn", (uint64_t)g_btn_counter++);
	naui_widget_button_render(id, label);
	return leaf_hovered(id) && naui_mouse_clicked(NAUI_MOUSE_LEFT);
}

bool naui_toggle(const char *label, bool *value)
{
	Leaf_ID id = leaf_id_indexed("__naui_toggle", (uint64_t)g_toggle_counter++);
	bool clicked = leaf_hovered(id) && naui_mouse_clicked(NAUI_MOUSE_LEFT);
	if (clicked)
		*value = !*value;

	naui_widget_toggle_render(id, label, value);
	return clicked;
}

bool naui_dropdown(const char* label, uint32_t* select, const char** items, size_t item_count)
{
	Leaf_ID id = leaf_id_indexed("__naui_dropdown", (uint64_t)g_dropdown_counter++);
	uint32_t original = *select;
	naui_widget_dropdown_render(id, label, select, items, item_count);
	return original != *select;
}

bool naui_slider_int(const char *label, int32_t *value, int32_t min, int32_t max)
{
	int32_t original = *value;
	Leaf_ID id = leaf_id_indexed("__naui_slider_int", (uint64_t)g_slider_int_counter++);
	naui_widget_slider_int_render(id, label, value, min, max);
	return original != *value;
}

bool naui_slider_float(const char *label, float *value, float min, float max, const char *fmt)
{
	float original = *value;
	Leaf_ID id = leaf_id_indexed("__naui_slider_float", (uint64_t)g_slider_float_counter++);
	naui_widget_slider_float_render(id, label, value, min, max, fmt);
	return original != *value;
}

bool naui_drag_int(const char *label, int *value, int speed, int min, int max)
{
	int original = *value;
	Leaf_ID id = leaf_id_indexed("__naui_drag_int", (uint64_t)g_drag_int_counter++);
	naui_widget_drag_int_render(id, label, value, speed, min, max);
	return original != *value;
}

bool naui_drag_float(const char *label, float *value, float speed, float min, float max, const char *fmt)
{
	float original = *value;
	Leaf_ID id = leaf_id_indexed("__naui_drag_float", (uint64_t)g_drag_float_counter++);
	naui_widget_drag_float_render(id, label, value, speed, min, max, fmt);
	return original != *value;
}

bool naui_text_field(const char *label, const char* hint, char *buffer, size_t buffer_size)
{
	size_t original_len = strlen(buffer);
	Leaf_ID id = leaf_id_indexed("__naui_text_field", (uint64_t)g_text_field_counter++);
	naui_widget_text_field_render(id, label, hint, buffer, buffer_size);
	return strlen(buffer) != original_len;
}

bool naui_knob(const char *label, float *value, float min, float max, const char *fmt)
{
	float original = *value;
	Leaf_ID id = leaf_id_indexed("__naui_knob", (uint64_t)g_knob_counter++);
	naui_widget_knob_render(id, label, value, min, max, fmt);
	return original != *value;
}
