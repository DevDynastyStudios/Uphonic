#define UPH_KNOB_ANGLE_MIN        (-0.75f * NAUI_PI)
#define UPH_KNOB_ANGLE_MAX        ( 0.75f * NAUI_PI)
#define UPH_KNOB_ARC_SEGMENTS     64
#define UPH_KNOB_DRAG_PIXELS      200
#define UPH_KNOB_FINE_FACTOR      0.1f

typedef struct
{
    Naui_Color color;
    Naui_Color track_color;
    Naui_Color body_color;
    Naui_Color ring_color;
    Naui_Color pointer_color;
    float t;
    Uph_UIKnobStartType start_type;
}
Uph_KnobDrawData;

static Naui_Vec2 _uph_ui_knob_point(Naui_Vec2 center, float radius, float angle)
{
    return (Naui_Vec2){ center.x + sinf(angle) * radius, center.y - cosf(angle) * radius };
}

static void _uph_ui_knob_ring(Naui_Vec2 center, float outer_r, float inner_r, float a0, float a1, Naui_Color color)
{
    if (a1 <= a0)
        return;

    const int segments = NAUI_MAX(1, (int)ceilf((a1 - a0) / (2.0f * NAUI_PI) * (float)UPH_KNOB_ARC_SEGMENTS));
    for (int i = 0; i < segments; i++)
    {
        const float sa = a0 + (a1 - a0) * ((float)i / (float)segments);
        const float sb = a0 + (a1 - a0) * ((float)(i + 1) / (float)segments);

        const Naui_Vec2 quad[4] =
        {
            _uph_ui_knob_point(center, outer_r, sa),
            _uph_ui_knob_point(center, outer_r, sb),
            _uph_ui_knob_point(center, inner_r, sb),
            _uph_ui_knob_point(center, inner_r, sa)
        };

        naui_fill_polygon(quad, 4, color);
    }
}

static void _uph_ui_knob_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_KnobDrawData *knob = (const Uph_KnobDrawData*)user_data;

    const float radius = NAUI_MIN(box.width, box.height) * 0.5f;
    const Naui_Vec2 center = { box.x + box.width * 0.5f, box.y + box.height * 0.5f };

    const float arc_width = NAUI_MAX(2.0f, radius * 0.13f);
    const float gap = NAUI_MAX(1.5f, radius * 0.12f);
    const float line_width = NAUI_MAX(1.5f, radius * 0.09f);
    const float dial_r = radius - arc_width - gap;
    const float body_r = dial_r - line_width;

    const float sweep = UPH_KNOB_ANGLE_MAX - UPH_KNOB_ANGLE_MIN;
    const float origin = knob->start_type == UPH_UI_KNOB_START_TYPE_CENTER ? 0.5f : 0.0f;
    const float value_angle = UPH_KNOB_ANGLE_MIN + NAUI_CLAMP01(knob->t) * sweep;
    const float origin_angle = UPH_KNOB_ANGLE_MIN + origin * sweep;

    _uph_ui_knob_ring(center, radius, radius - arc_width, UPH_KNOB_ANGLE_MIN, UPH_KNOB_ANGLE_MAX, knob->track_color);
    _uph_ui_knob_ring(center, radius, radius - arc_width, NAUI_MIN(origin_angle, value_angle), NAUI_MAX(origin_angle, value_angle), knob->color);

    naui_fill_rect((Naui_Vec2){ center.x - body_r, center.y - body_r }, (Naui_Vec2){ body_r * 2.0f, body_r * 2.0f }, knob->body_color, body_r, NAUI_CORNER_ALL);
    _uph_ui_knob_ring(center, dial_r, body_r, 0.0f, 2.0f * NAUI_PI, knob->ring_color);

    naui_draw_line(
        _uph_ui_knob_point(center, body_r * 0.1f, value_angle),
        _uph_ui_knob_point(center, body_r * 0.68f, value_angle),
        knob->pointer_color,
        line_width
    );
}

bool uph_ui_knob_float(float *value, const Leaf_ID id, float min, float max, float default_value, Naui_Color color, Uph_UIKnobStartType start_type)
{
    Uph_KnobData *kdata = &uph_global_widget_state.knob_data;

    const bool hovered = uph_ui_widget_hovered(id);
    const bool dragging = (kdata->id.value == id.value);
    const float range = max - min;
    bool result = false;

    if (hovered || dragging)
        naui_set_cursor(NAUI_CURSOR_RESIZE_NS);

    if (hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
    {
        const float reset = NAUI_CLAMP(default_value, min, max);
        kdata->id.value = 0;
        if (reset != *value)
        {
            *value = reset;
            result = true;
        }
    }
    else if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT) && !dragging)
    {
        kdata->id = id;
        kdata->last_mouse_y = naui_mouse_y();
    }
    else if (dragging)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
        {
            const int32_t my = naui_mouse_y();
            const float delta_pixels = (float)(kdata->last_mouse_y - my);
            kdata->last_mouse_y = my;

            const float fine = naui_key_down(NAUI_KEY_SHIFT) ? UPH_KNOB_FINE_FACTOR : 1.0f;
            const float delta = (delta_pixels / NAUI_DPI(UPH_KNOB_DRAG_PIXELS)) * range * fine;

            const float new_value = NAUI_CLAMP(*value + delta, min, max);
            if (new_value != *value)
            {
                *value = new_value;
                result = true;
            }
        }
        else
        {
            kdata->id.value = 0;
        }
    }

    const float t = (range != 0.0f) ? NAUI_CLAMP01((*value - min) / range) : 0.0f;
    const bool active = kdata->id.value == id.value;
    const Naui_Color body_color = active ? uph_state.colors.ui_tinted_frame_pressed_bg : (hovered ? uph_state.colors.ui_tinted_frame_hovered_bg : uph_state.colors.ui_tinted_frame_bg);
    Uph_KnobDrawData draw_data = {
        .color = color,
        .track_color = uph_state.colors.ui_tinted_knob_track,
        .body_color = body_color,
        .ring_color = active || hovered ?
            uph_color_mix(uph_state.colors.ui_knob_ring, uph_state.colors.ui_text, 0.4f) :
            uph_state.colors.ui_knob_ring,
        .pointer_color = uph_state.colors.ui_text,
        .t = t,
        .start_type = start_type
    };

    leaf({
        .id = id,
        .custom_draw = _uph_ui_knob_draw,
        .custom_draw_data = LEAF_DATA_SLICE(draw_data),
        .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_DERIVED },
        .aspect_ratio = 1.0f
    });

    return result;
}
