typedef enum
{
    UPH_UI_EDIT_MODE_NONE,
    UPH_UI_EDIT_MODE_TEXT,
    UPH_UI_EDIT_MODE_DRAG_FLOAT,
    UPH_UI_EDIT_MODE_DRAG_INT
}
Uph_UIEditMode;

typedef struct
{
    Leaf_ID id;
    Uph_UIEditMode mode;
    Naui_String value;
    Naui_String *target_string;
    void *target_number;
    size_t cursor;
    size_t select_start;
    bool select_active;
    float scroll_x;
}
Uph_TextfieldData;

typedef struct
{
    Leaf_ID id;
    float start_value_f;
    int32_t start_value_i;
    float accum;
}
Uph_DragData;

typedef struct
{
    Leaf_ID id;
    int32_t last_mouse_y;
}
Uph_KnobData;

typedef struct Uph_UIMenuNode Uph_UIMenuNode;
struct Uph_UIMenuNode
{
    char *text;
    Leaf_ID element_id;
    Uph_UIMenuNode *next_sibling;
    Uph_UIMenuNode *first_child;
    Uph_UIMenuNode *last_child;
    void (*deferred_render)(Uph_UIMenuID menu_id);
};

typedef struct
{
    Naui_String *value;
    const char *placeholder;
    float scroll_x;
}
Uph_TextfieldDrawData;

typedef struct
{
    uint32_t id_counter;
    Uph_TextfieldData textfield_data;
    Uph_DragData drag_data;
    Uph_KnobData knob_data;
    Naui_Arena menu_arena;
    Uph_UIMenuNode *current_open_menu;
    uint64_t open_menu_key;
    Naui_Vec2 current_context_menu_position;
    bool is_current_menu_context;
    bool menu_opened_last_frame;
    bool textfield_submitted_this_frame;
    bool any_widget_hovered;
}
Uph_GlobalWidgetData;
Uph_GlobalWidgetData uph_global_widget_data = { 0 };

#pragma region Static Helpers

static const char *uph_ui__style_scope = NULL;

void uph_ui_set_style_scope(const char *scope)
{
    uph_ui__style_scope = scope;
}

static const char *uph_ui__style_key(const char *name)
{
    static char keys[8][128];
    static uint32_t slot = 0;
    char *key = keys[slot++ & 7];

    if (uph_ui__style_scope)
    {
        snprintf(key, sizeof(keys[0]), "%s_%s", uph_ui__style_scope, name);
        if (naui_theme_has(key))
            return key;
    }

    snprintf(key, sizeof(keys[0]), "uph_ui_%s", name);
    return key;
}

#define UPH_UI_SIZE_EXACT(v) (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_FIXED, .size.min_max = { .min = (v), .max = (v) } }

static float uph_ui__frame_height(void)
{
    return NAUI_DPI(naui_theme_float(uph_ui__style_key("frame_height")));
}

static bool uph_ui__is_word_char(char c)
{
    return (unsigned char)c >= 0x80 || isalnum((unsigned char)c) || c == '_';
}

static bool uph_ui__is_utf8_continuation(const Naui_String *value, size_t index)
{
    return index < value->length && ((unsigned char)value->data[index] & 0xC0) == 0x80;
}

static size_t uph_ui__prev_char(const Naui_String *value, size_t cursor)
{
    if (cursor == 0)
        return 0;

    size_t i = cursor - 1;
    while (i > 0 && uph_ui__is_utf8_continuation(value, i))
        --i;
    return i;
}

static size_t uph_ui__next_char(const Naui_String *value, size_t cursor)
{
    if (cursor >= value->length)
        return value->length;

    size_t i = cursor + 1;
    while (uph_ui__is_utf8_continuation(value, i))
        ++i;
    return i;
}

static size_t uph_ui__utf8_encode(uint32_t codepoint, char out[4])
{
    if (codepoint < 0x80)
    {
        out[0] = (char)codepoint;
        return 1;
    }
    if (codepoint < 0x800)
    {
        out[0] = (char)(0xC0 | (codepoint >> 6));
        out[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    }
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
        return 0;
    if (codepoint < 0x10000)
    {
        out[0] = (char)(0xE0 | (codepoint >> 12));
        out[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    }
    if (codepoint < 0x110000)
    {
        out[0] = (char)(0xF0 | (codepoint >> 18));
        out[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
    return 0;
}

static bool uph_ui__insert_codepoint(Naui_String *value, size_t *cursor, uint32_t codepoint)
{
    char bytes[4];
    const size_t count = uph_ui__utf8_encode(codepoint, bytes);
    if (count == 0 || *cursor > value->length || value->length + count > NAUI_STRING_MAX_SIZE - 1)
        return false;

    memmove(&value->data[*cursor + count], &value->data[*cursor], value->length - *cursor);
    memcpy(&value->data[*cursor], bytes, count);
    value->length += count;
    value->data[value->length] = '\0';
    *cursor += count;
    return true;
}

static void uph_ui__begin_text_edit(Uph_TextfieldData *data, const Leaf_ID id, Naui_String *value, size_t cursor)
{
    data->id = id;
    data->mode = UPH_UI_EDIT_MODE_TEXT;
    data->target_string = value;
    data->target_number = NULL;
    data->cursor = NAUI_MIN(cursor, value->length);
    data->select_start = data->cursor;
    data->select_active = false;
    data->scroll_x = 0.0f;
}

static void uph_ui__end_edit(Uph_TextfieldData *data)
{
    data->id.value = 0;
    data->mode = UPH_UI_EDIT_MODE_NONE;
    data->target_string = NULL;
    data->target_number = NULL;
    data->select_active = false;
}

static size_t uph_ui__prev_word(const Naui_String *value, size_t cursor)
{
    if (cursor == 0)
        return 0;

    size_t i = cursor;
    while (i > 0 && !uph_ui__is_word_char(value->data[i - 1]))
        --i;
    while (i > 0 && uph_ui__is_word_char(value->data[i - 1]))
        --i;
    return i;
}

static size_t uph_ui__next_word(const Naui_String *value, size_t cursor)
{
    if (cursor >= value->length)
        return value->length;

    size_t i = cursor;
    while (i < value->length && !uph_ui__is_word_char(value->data[i]))
        ++i;
    while (i < value->length && uph_ui__is_word_char(value->data[i]))
        ++i;
    return i;
}

static void uph_ui__textfield_delete_selection(Uph_TextfieldData *data)
{
    if (!data->select_active)
        return;

    size_t lo = NAUI_MIN(data->cursor, data->select_start);
    size_t hi = NAUI_MAX(data->cursor, data->select_start);

    naui_string_remove(data->target_string, lo, hi - lo);
    data->cursor = lo;
    data->select_active = false;
}

static bool uph_ui__char_allowed(uint32_t codepoint, Uph_UITextFieldFlags flags, const Naui_String *value, size_t cursor)
{
    if (!(flags & UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY))
        return true;

    if (codepoint >= '0' && codepoint <= '9')
        return true;

    if (codepoint == '-' && cursor == 0)
        return true;

    if (codepoint == '.')
    {
        for (size_t i = 0; i < value->length; ++i)
            if (value->data[i] == '.')
                return false;
        return true;
    }

    return false;
}

static size_t uph_ui__cursor_from_x(const Naui_String *value, float local_x, float font_size)
{
    if (local_x <= 0.0f)
        return 0;

    size_t best = value->length;
    float best_dist = fabsf(naui_measure_text(value->data, value->length, font_size, 0).x - local_x);

    for (size_t i = 0; i <= value->length; ++i)
    {
        if (uph_ui__is_utf8_continuation(value, i))
            continue;

        float x = naui_measure_text(value->data, i, font_size, 0).x;
        float dist = fabsf(x - local_x);
        if (dist < best_dist)
        {
            best_dist = dist;
            best = i;
        }
    }

    return best;
}

static bool uph_ui__slider_scalar(const Leaf_ID id, void *value, bool is_float, float min_f, float max_f, int32_t min_i, int32_t max_i, const char *format, Uph_UISliderFlags flags)
{
    Uph_TextfieldData *tdata = &uph_global_widget_data.textfield_data;
    static Naui_String edit_buffer;
    bool text_editing = (tdata->id.value == id.value) && (tdata->mode == UPH_UI_EDIT_MODE_TEXT) && (tdata->target_number == value);
    bool result = false;
    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    if (!text_editing && hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
    {
        if (is_float)
            edit_buffer = naui_string_format(format ? (char*)format : "%.3f", *(float*)value);
        else
            edit_buffer = naui_string_format(format ? (char*)format : "%d", *(int32_t*)value);

        uph_ui__begin_text_edit(tdata, id, &edit_buffer, edit_buffer.length);
        tdata->target_number = value;

        text_editing = true;
    }

    if (text_editing)
    {
        bool sub_result = uph_ui_textfield(&edit_buffer, id, UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY, NULL);
        if (sub_result)
        {
            if (is_float)
                *(float*)value = NAUI_CLAMP((float)atof(edit_buffer.data), min_f, max_f);
            else
                *(int32_t*)value = NAUI_CLAMP((int32_t)atoi(edit_buffer.data), min_i, max_i);
            result = true;
        }

        if (tdata->id.value != id.value || tdata->mode != UPH_UI_EDIT_MODE_TEXT)
            tdata->target_number = NULL;

        return result;
    }

    Uph_DragData *ddata = &uph_global_widget_data.drag_data;
    Leaf_BoundingBox box = leaf_get_bounding_box(id);
    const bool vertical = (flags & UPH_UI_SLIDER_FLAG_VERTICAL) != 0;

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        ddata->id = id;

    bool dragging_this = (ddata->id.value == id.value);
    if (dragging_this && !naui_mouse_down(NAUI_MOUSE_LEFT))
    {
        ddata->id.value = 0;
        dragging_this = false;
    }

    if (dragging_this)
        naui_set_cursor(NAUI_CURSOR_HAND);

    if (dragging_this)
    {
        float t;
        if (vertical)
        {
            float local_y = (float)naui_mouse_y() - box.y;
            t = 1.0f - NAUI_CLAMP(local_y / NAUI_MAX(box.height, 1.0f), 0.0f, 1.0f);
        }
        else
        {
            float local_x = (float)naui_mouse_x() - box.x;
            t = NAUI_CLAMP(local_x / NAUI_MAX(box.width, 1.0f), 0.0f, 1.0f);
        }

        if (is_float)
        {
            float new_value = min_f + t * (max_f - min_f);
            if (new_value != *(float*)value)
            {
                *(float*)value = new_value;
                result = true;
            }
        }
        else
        {
            int32_t new_value = min_i + (int32_t)(t * (float)(max_i - min_i) + 0.5f);
            if (new_value != *(int32_t*)value)
            {
                *(int32_t*)value = new_value;
                result = true;
            }
        }
    }

    float t;
    if (is_float)
        t = (max_f > min_f) ? NAUI_CLAMP((*(float*)value - min_f) / (max_f - min_f), 0.0f, 1.0f) : 0.0f;
    else
        t = (max_i > min_i) ? NAUI_CLAMP((float)(*(int32_t*)value - min_i) / (float)(max_i - min_i), 0.0f, 1.0f) : 0.0f;

    Naui_String display;
    if (is_float)
        display = naui_string_format(format ? (char*)format : "%.3f", *(float*)value);
    else
        display = naui_string_format(format ? (char*)format : "%d", *(int32_t*)value);

    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float rounding = NAUI_DPI(naui_theme_float("uph_ui_frame_rounding"));
    const float handle_size = NAUI_DPI(3.0f);

    const Leaf_Color bg_color = naui_theme_color(uph_ui__style_key("frame_bg_color"));
    const Leaf_Color hovered_bg_color = naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));
    const Leaf_Color pressed_bg_color = naui_theme_color(uph_ui__style_key("frame_pressed_bg_color"));
    const Leaf_Color frame_color = dragging_this ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);

    const Leaf_Color handle_color = naui_theme_color("uph_ui_slider_fill_color");
    const Leaf_Color fill_color = { handle_color.r, handle_color.g, handle_color.b, (uint8_t)(handle_color.a * 0.35f) };

    const float track_length = vertical ? box.height : box.width;
    const float fill_length = t * track_length;
    const float fill_rounding = NAUI_MIN(rounding, fill_length * 0.5f);
    const float handle_offset = t * NAUI_MAX(track_length - handle_size, 0.0f);

    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_GROW,
            .height = UPH_UI_SIZE_EXACT(uph_ui__frame_height())
        },
        .color = {frame_color},
        .rounding = LEAF_ROUNDING_FIXED(rounding, NAUI_CORNER_ALL)
    })
    {
        leaf({
            .size = vertical ? (Leaf_Size){ .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_PERCENT(t) }
                : (Leaf_Size){ .width = LEAF_SIZE_PERCENT(t), .height = LEAF_SIZE_FULL },
            .color = {fill_color},
            .rounding = LEAF_ROUNDING_FIXED(fill_rounding, vertical ? (NAUI_CORNER_BL | NAUI_CORNER_BR) : (NAUI_CORNER_TL | NAUI_CORNER_BL)),
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .floating = {
                .parent_alignment = vertical ? (Leaf_Alignment){LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_BOTTOM}
                    : (Leaf_Alignment){LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                .self_alignment = vertical ? (Leaf_Alignment){LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_BOTTOM}
                    : (Leaf_Alignment){LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
            }
        });

        leaf({
            .size = vertical ? (Leaf_Size){ .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(handle_size) }
                : (Leaf_Size){ .width = LEAF_SIZE_FIXED(handle_size), .height = LEAF_SIZE_FULL },
            .color = {handle_color},
            .rounding = LEAF_ROUNDING_FIXED(handle_size * 0.5f, NAUI_CORNER_ALL),
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .floating = {
                .offset = vertical ? (Leaf_Vec2){ 0.0f, -handle_offset } : (Leaf_Vec2){ handle_offset, 0.0f },
                .parent_alignment = vertical ? (Leaf_Alignment){LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_BOTTOM}
                    : (Leaf_Alignment){LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                .self_alignment = vertical ? (Leaf_Alignment){LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_BOTTOM}
                    : (Leaf_Alignment){LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
            }
        });

        leaf({
            .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .floating = {
                .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            },
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        })
        {
            leaf_text(display.data, {
                .color = {naui_theme_color("uph_ui_text_color")},
                .font_size = font_size
            });
        }
    }

    return result;
}

static bool uph_ui__drag_scalar(const Leaf_ID id, void *value, bool is_float, float speed, float min_f, float max_f, int32_t min_i, int32_t max_i, const char *format, Uph_UIDragFlags flags)
{
    Uph_TextfieldData *tdata = &uph_global_widget_data.textfield_data;
    Uph_DragData *ddata = &uph_global_widget_data.drag_data;

    bool result = false;

    static Naui_String edit_buffer;

    bool text_editing = (tdata->id.value == id.value) && (tdata->mode == UPH_UI_EDIT_MODE_TEXT) && (tdata->target_number == value);

    const bool hovered = uph_ui_widget_hovered(id);
    const bool dragging_this = (ddata->id.value == id.value);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    if (!text_editing && hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
    {
        if (is_float)
            edit_buffer = naui_string_format(format ? (char*)format : "%.3f", *(float*)value);
        else
            edit_buffer = naui_string_format(format ? (char*)format : "%d", *(int32_t*)value);

        uph_ui__begin_text_edit(tdata, id, &edit_buffer, edit_buffer.length);
        tdata->target_number = value;
        ddata->id.value = 0;

        text_editing = true;
    }

    if (text_editing)
    {
        bool sub_result = uph_ui_textfield(&edit_buffer, id, UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY, NULL);

        if (sub_result)
        {
            if (is_float)
            {
                float parsed = (float)atof(edit_buffer.data);
                if (flags & UPH_UI_DRAG_FLAG_CLAMPED)
                    parsed = NAUI_CLAMP(parsed, min_f, max_f);
                *(float*)value = parsed;
            }
            else
            {
                int32_t parsed = (int32_t)atoi(edit_buffer.data);
                if (flags & UPH_UI_DRAG_FLAG_CLAMPED)
                    parsed = NAUI_CLAMP(parsed, min_i, max_i);
                *(int32_t*)value = parsed;
            }
            result = true;
        }

        if (tdata->id.value != id.value || tdata->mode != UPH_UI_EDIT_MODE_TEXT)
            tdata->target_number = NULL;

        return result;
    }

    static int32_t drag_last_mouse_x = 0;
    static float drag_int_accum = 0.0f;

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT) && !dragging_this)
    {
        ddata->id = id;
        ddata->accum = 0.0f;
        drag_last_mouse_x = naui_mouse_x();
        drag_int_accum = 0.0f;
        if (is_float)
            ddata->start_value_f = *(float*)value;
        else
            ddata->start_value_i = *(int32_t*)value;
    }

    if (dragging_this)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
        {
            int32_t mx = naui_mouse_x();
            float delta = (float)(mx - drag_last_mouse_x);
            drag_last_mouse_x = mx;

            ddata->accum += delta * speed;

            if (is_float)
            {
                float new_value = *(float*)value + delta * speed;
                if (flags & UPH_UI_DRAG_FLAG_CLAMPED)
                    new_value = NAUI_CLAMP(new_value, min_f, max_f);
                if (new_value != *(float*)value)
                {
                    *(float*)value = new_value;
                    result = true;
                }
            }
            else
            {
                drag_int_accum += delta * speed;
                if (fabsf(drag_int_accum) >= 1.0f)
                {
                    int32_t step = (int32_t)drag_int_accum;
                    drag_int_accum -= (float)step;

                    int32_t new_value = *(int32_t*)value + step;
                    if (flags & UPH_UI_DRAG_FLAG_CLAMPED)
                        new_value = NAUI_CLAMP(new_value, min_i, max_i);
                    if (new_value != *(int32_t*)value)
                    {
                        *(int32_t*)value = new_value;
                        result = true;
                    }
                }
            }
        }
        else
        {
            ddata->id.value = 0;
        }
    }

    Naui_String display;
    if (is_float)
        display = naui_string_format(format ? (char*)format : "%.3f", *(float*)value);
    else
        display = naui_string_format(format ? (char*)format : "%d", *(int32_t*)value);

    const Leaf_Color bg_color = naui_theme_color(uph_ui__style_key("frame_bg_color"));
    const Leaf_Color hovered_bg_color = naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));
    const Leaf_Color pressed_bg_color = naui_theme_color(uph_ui__style_key("frame_pressed_bg_color"));

    const Leaf_Color color = dragging_this ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));

    const bool frameless = (flags & UPH_UI_DRAG_FLAG_FRAMELESS) != 0;

    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_GROW,
            .height = frameless ? LEAF_SIZE_FIT : UPH_UI_SIZE_EXACT(uph_ui__frame_height())
        },
        .padding = frameless ? (Leaf_Padding){0} : LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .color = {frameless ? LEAF_COLOR_TRANSPARENT : color},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.clip_children = true
    })
    {
        leaf_text(display.data, {
            .color = {naui_theme_color("uph_ui_text_color")},
            .font_size = font_size
        });
    }

    return result;
}

#pragma region Knob
#define UPH_KNOB_ANGLE_MIN        (-0.75f * NAUI_PI)
#define UPH_KNOB_ANGLE_MAX        ( 0.75f * NAUI_PI)
#define UPH_KNOB_ARC_SEGMENTS     64
#define UPH_KNOB_DRAG_PIXELS      200.0f
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

static Naui_Vec2 uph_ui__knob_point(Naui_Vec2 center, float radius, float angle)
{
    return (Naui_Vec2){ center.x + sinf(angle) * radius, center.y - cosf(angle) * radius };
}

static void uph_ui__knob_ring(Naui_Vec2 center, float outer_r, float inner_r, float a0, float a1, Naui_Color color)
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
            uph_ui__knob_point(center, outer_r, sa),
            uph_ui__knob_point(center, outer_r, sb),
            uph_ui__knob_point(center, inner_r, sb),
            uph_ui__knob_point(center, inner_r, sa)
        };

        naui_fill_polygon(quad, 4, color);
    }
}

static void uph_ui__knob_draw(Leaf_BoundingBox box, void *user_data)
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

    uph_ui__knob_ring(center, radius, radius - arc_width, UPH_KNOB_ANGLE_MIN, UPH_KNOB_ANGLE_MAX, knob->track_color);
    uph_ui__knob_ring(center, radius, radius - arc_width, NAUI_MIN(origin_angle, value_angle), NAUI_MAX(origin_angle, value_angle), knob->color);

    naui_fill_rect((Naui_Vec2){ center.x - body_r, center.y - body_r }, (Naui_Vec2){ body_r * 2.0f, body_r * 2.0f }, knob->body_color, body_r, NAUI_CORNER_ALL);
    uph_ui__knob_ring(center, dial_r, body_r, 0.0f, 2.0f * NAUI_PI, knob->ring_color);

    naui_draw_line(
        uph_ui__knob_point(center, body_r * 0.1f, value_angle),
        uph_ui__knob_point(center, body_r * 0.68f, value_angle),
        knob->pointer_color,
        line_width
    );
}

#pragma endregion

#pragma endregion

#pragma region Public API
bool uph_ui_widget_active(const Leaf_ID id)
{
    return uph_global_widget_data.drag_data.id.value == id.value;
}

bool uph_ui_widget_hovered(const Leaf_ID id)
{
    if (uph_dialog_blocks_input())
        return false;

    const bool result = !uph_ui_any_widget_hovered() && ((!naui_panel_current() && !naui_panel_is_any_hovered()) || (naui_panel_current() && naui_panel_is_hovered(naui_panel_current()))) && leaf_hovered_clipped(id);
    if (result)
        uph_global_widget_data.any_widget_hovered = true;
    return result;
}

bool uph_ui_any_widget_hovered(void)
{
    return uph_global_widget_data.any_widget_hovered;
}

static void uph_ui_render_menu_dropdown_child(Uph_UIMenuNode *node)
{
    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));

    leaf({
        .id = node->element_id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_X_CENTER},
        .color = {leaf_hovered(node->element_id) ?
            naui_theme_color(uph_ui__style_key("frame_hovered_bg_color")) :
            LEAF_COLOR_TRANSPARENT},
        .child_gap = NAUI_DPI(8.0f),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {    
        leaf_text(node->text, {
            .color = {text_color},
            .font_size = font_size
        });
        if (node->first_child)
        {
            leaf({
                .size = {LEAF_SIZE_FIXED(font_size), LEAF_SIZE_FIXED(font_size)},
                .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL),
                .image = naui_asset_image("uph_icon_dropdown"),
                .color = {text_color}
            });
        }
    }
}

static bool uph_ui_menu_dropdown_hovered(Uph_UIMenuNode *node)
{
    if (leaf_hovered(node->element_id))
        return true;
    else
    {
        Uph_UIMenuNode *current_child = node->first_child;
        while (current_child)
        {
            if (uph_ui_menu_dropdown_hovered(current_child))
                return true;
            current_child = current_child->next_sibling;
        }
    }
    return false;
}

static void uph_ui_render_menu_dropdown_recursive(Uph_UIMenuNode *node, Naui_Vec2 position_offset)
{
    leaf({
        .color = {naui_theme_color("uph_ui_dropdown_bg_color")},
        .floating = {
            .offset = {position_offset.x, position_offset.y}
        },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
        .shadow = {
            .blur_radius = NAUI_DPI(16.0f),
            .color = naui_theme_color("uph_ui_dropdown_shadow_color")
        },
        .uniform_children = LEAF_UNIFORM_SIZING_WIDTH,
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
    })
    {
        while (node)
        {
            if (uph_ui_menu_dropdown_hovered(node))
            {
                Leaf_BoundingBox bbox = leaf_get_bounding_box(node->element_id);
                uph_ui_render_menu_dropdown_recursive(node->first_child, (Naui_Vec2){bbox.x + bbox.width, bbox.y});
            }
            uph_ui_render_menu_dropdown_child(node);
            node = node->next_sibling;
        }
    }
}

static Uph_UIMenuNode *uph_ui__menu_root(Uph_GlobalWidgetData *data, const Leaf_ID element_id)
{
    Uph_UIMenuNode *menu = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));
    menu->element_id = element_id;
    if (data->open_menu_key != 0 && element_id.value == data->open_menu_key)
        data->current_open_menu = menu;
    return menu;
}

static void uph_ui__open_menu(Uph_GlobalWidgetData *data, Uph_UIMenuNode *menu, bool context)
{
    data->current_open_menu = menu;
    data->open_menu_key = menu->element_id.value;
    data->is_current_menu_context = context;
    data->menu_opened_last_frame = false;
}

static void uph_ui__close_menu(Uph_GlobalWidgetData *data)
{
    data->current_open_menu = NULL;
    data->open_menu_key = 0;
}

static bool uph_ui__menu_is_open(const Uph_GlobalWidgetData *data, const Uph_UIMenuNode *menu)
{
    return data->open_menu_key != 0 && menu->element_id.value == data->open_menu_key;
}

static void uph_ui_render_menu_dropdown(Uph_GlobalWidgetData *data)
{
    if (data->current_open_menu)
    {
        if (data->current_open_menu->deferred_render)
        {
            naui_panel_occlude_all();
            Leaf_ID id = leaf_id("uph_widget_current_custom_context_menu");
            Leaf_BoundingBox bbox = leaf_get_bounding_box(data->current_open_menu->element_id);
            Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
            leaf({
                .id = id,
                .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
                .floating = {
                    .offset = {data->current_context_menu_position.x, data->current_context_menu_position.y}
                },
                .color = {naui_theme_color("uph_ui_dropdown_bg_color")},
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
                .shadow = {
                    .blur_radius = NAUI_DPI(16.0f),
                    .color = naui_theme_color("uph_ui_dropdown_shadow_color")
                },
                .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y))
            }) data->current_open_menu->deferred_render((Uph_UIMenuID)data->current_open_menu);

            if (data->menu_opened_last_frame && naui_any_mouse_pressed() && !leaf_hovered(id))
            {
                uph_ui__close_menu(data);
                return;
            }

            data->menu_opened_last_frame = true;
            return;
        }

        Uph_UIMenuNode *current_node = data->current_open_menu->first_child;
        if (!current_node)
        {
            uph_ui__close_menu(data);
            return;
        }

        naui_panel_occlude_all();

        if (data->menu_opened_last_frame && naui_any_mouse_pressed() && !uph_ui_menu_dropdown_hovered(data->current_open_menu))
        {
            uph_ui__close_menu(data);
            return;
        }

        if (!data->is_current_menu_context)
        {
            Leaf_BoundingBox bbox = leaf_get_bounding_box(data->current_open_menu->element_id);
            uph_ui_render_menu_dropdown_recursive(current_node, (Naui_Vec2){bbox.x, bbox.y + bbox.height});
        }
        else
        {
            uph_ui_render_menu_dropdown_recursive(current_node, data->current_context_menu_position);
        }

        data->menu_opened_last_frame = true;
    }
}

void uph_ui_widgets_init(void)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    naui_arena_init(&data->menu_arena, (1 << 14));
}

void uph_ui_widgets_flush(void)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    uph_ui_render_menu_dropdown(data);
    if (!data->current_open_menu)
        data->open_menu_key = 0;
    data->current_open_menu = NULL;

    if (data->textfield_data.mode != UPH_UI_EDIT_MODE_NONE && !data->textfield_submitted_this_frame)
        uph_ui__end_edit(&data->textfield_data);
    data->textfield_submitted_this_frame = false;

    data->id_counter = 0;
    data->any_widget_hovered = false;

    naui_arena_reset(&data->menu_arena);
}

Uph_UIMenuID uph_ui_menu(const char *name, const Leaf_ID element_id)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;

    Uph_UIMenuNode *menu = uph_ui__menu_root(data, element_id);

    const bool hovered = uph_ui_widget_hovered(element_id);
    if (hovered)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            if (data->open_menu_key != 0)
                uph_ui__close_menu(data);
            else
                uph_ui__open_menu(data, menu, false);
        }
        else if (!data->is_current_menu_context && data->open_menu_key != 0)
        {
            data->current_open_menu = menu;
            data->open_menu_key = element_id.value;
        }
    }


    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    leaf({
        .id = element_id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = {(uph_ui__menu_is_open(data, menu) || hovered) ?
            naui_theme_color(uph_ui__style_key("frame_hovered_bg_color")) : LEAF_COLOR_TRANSPARENT},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL)
    })
    {
        leaf_text(name, {
            .color = {naui_theme_color("uph_ui_text_color")},
            .font_size = {NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")))}
        });
    }

    return (Uph_UIMenuID)menu;
}

Uph_UIMenuID uph_ui_context_menu(const Leaf_ID id)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    Uph_UIMenuNode *menu = uph_ui__menu_root(data, id);
    return (Uph_UIMenuID)menu;
}

Uph_UIMenuID uph_ui_context_menu_custom(const Leaf_ID id, void (*custom_render)(Uph_UIMenuID menu_id))
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    Uph_UIMenuNode *menu = uph_ui__menu_root(data, id);
    menu->deferred_render = custom_render;
    return (Uph_UIMenuID)menu;
}

void uph_ui_open_context_menu(Uph_UIMenuID menu)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    uph_ui__open_menu(data, (Uph_UIMenuNode*)menu, true);
    data->current_context_menu_position = (Naui_Vec2){naui_mouse_x(), naui_mouse_y()};
}

void uph_ui_close_menu(void)
{
    uph_ui__close_menu(&uph_global_widget_data);
}

bool uph_ui_any_menu_open(void)
{
    return uph_global_widget_data.open_menu_key != 0;
}

static void uph_ui_append_menu_child(Uph_UIMenuNode *parent, Uph_UIMenuNode *child)
{
    if (!parent->first_child)
    {
        parent->first_child = child;
        parent->last_child = child;
    }
    else
    {
        parent->last_child->next_sibling = child;
        parent->last_child = child;
    }
}

Uph_UIMenuID uph_ui_submenu(Uph_UIMenuID parent_id, const char *name, const Leaf_ID element_id)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;

    Uph_UIMenuNode *parent = (Uph_UIMenuNode*)parent_id;
    Uph_UIMenuNode *menu = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));
    menu->element_id = element_id;

    const uint32_t name_length = strlen(name);
    menu->text = naui_arena_alloc(&data->menu_arena, name_length + 1);
    memcpy(menu->text, name, name_length);
    menu->text[name_length] = '\0';

    uph_ui_append_menu_child(parent, menu);

    return (Uph_UIMenuID)menu;
}

bool uph_ui_menu_item(Uph_UIMenuID menu_id, const char *name, const Leaf_ID element_id)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;

    Uph_UIMenuNode *parent = (Uph_UIMenuNode*)menu_id;
    Uph_UIMenuNode *item = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));

    item->element_id = element_id;
    const uint32_t name_length = strlen(name);
    item->text = naui_arena_alloc(&data->menu_arena, name_length + 1);
    memcpy(item->text, name, name_length);
    item->text[name_length] = '\0';
    uph_ui_append_menu_child(parent, item);

    bool hovered = leaf_hovered(element_id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    bool result;
    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        uph_ui__close_menu(&uph_global_widget_data);
        result = true;
    }
    else result = false;

    return result;
}

bool uph_ui_text_button_ex(const char *string, const Leaf_ID id, Naui_Color bg_color, Naui_CornerFlags corners)
{
    const Leaf_Color hovered_bg_color = naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));
    const Leaf_Color pressed_bg_color = naui_theme_color(uph_ui__style_key("frame_pressed_bg_color"));

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Leaf_Color color;
    if (hovered)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
            color = pressed_bg_color;
        else color = hovered_bg_color;
    }
    else color = bg_color;
    
    const float frame_height = uph_ui__frame_height();
    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIT_MIN(frame_height), UPH_UI_SIZE_EXACT(frame_height)},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = {color},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        leaf_text(string, {
            .font_size = {NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")))},
            .color = {text_color}
        });
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

bool uph_ui_text_button(const char *string, const Leaf_ID id)
{
    return uph_ui_text_button_ex(string, id, naui_theme_color(uph_ui__style_key("frame_bg_color")), NAUI_CORNER_ALL);
}

bool uph_ui_image_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners)
{
    const bool overlay = bg_color.a == 0;
    const Leaf_Color hovered_bg_color = overlay ? (Leaf_Color) { 255, 255, 255, 36 } : naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));
    const Leaf_Color pressed_bg_color = overlay ? (Leaf_Color) { 255, 255, 255, 64 } : naui_theme_color(uph_ui__style_key("frame_pressed_bg_color"));

    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Leaf_Color color;
    if (hovered)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
            color = pressed_bg_color;
        else color = hovered_bg_color;
    }
    else color = bg_color;
    
    const float frame_width = NAUI_MAX(uph_ui__frame_height(), size.x + NAUI_DPI(padding.y));
    const float frame_height = NAUI_MAX(uph_ui__frame_height(), size.y + NAUI_DPI(padding.y));
    leaf({
        .id = id,
        .size = {UPH_UI_SIZE_EXACT(frame_width), UPH_UI_SIZE_EXACT(frame_height)},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = {color},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), corners)
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(size.x), LEAF_SIZE_FIXED(size.y)},
            .color = {tint},
            .image = (void*)image
        });
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

bool uph_ui_image_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint)
{
    return uph_ui_image_button_ex(image, id, size, tint, naui_theme_color(uph_ui__style_key("frame_bg_color")), NAUI_CORNER_ALL);
}

bool uph_ui_text_toggle_button(const char *string, const Leaf_ID id, bool enabled)
{
    const Leaf_Color bg_color = naui_theme_color(uph_ui__style_key("frame_bg_color"));
    const Leaf_Color hovered_bg_color = naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));
    const Leaf_Color pressed_bg_color = naui_theme_color(uph_ui__style_key("frame_pressed_bg_color"));

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Leaf_Color color = enabled ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    
    const float frame_height = uph_ui__frame_height();
    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIT_MIN(frame_height), UPH_UI_SIZE_EXACT(frame_height)},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = {color},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        leaf_text(string, {
            .font_size = {NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")))},
            .color = {text_color}
        });
    }

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return true;

    return false;
}

bool uph_ui_image_toggle_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners, bool enabled)
{
    const Leaf_Color hovered_bg_color = naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));
    const Leaf_Color pressed_bg_color = naui_theme_color(uph_ui__style_key("frame_pressed_bg_color"));

    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Leaf_Color color = enabled ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    
    const float frame_width = NAUI_MAX(uph_ui__frame_height(), size.x + NAUI_DPI(padding.y));
    const float frame_height = NAUI_MAX(uph_ui__frame_height(), size.y + NAUI_DPI(padding.y));
    leaf({
        .id = id,
        .size = {UPH_UI_SIZE_EXACT(frame_width), UPH_UI_SIZE_EXACT(frame_height)},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = {color},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), corners)
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(size.x), LEAF_SIZE_FIXED(size.y)},
            .color = {tint},
            .image = (void*)image
        });
    }

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return true;

    return false;
}

bool uph_ui_image_toggle_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, bool enabled)
{
    return uph_ui_image_toggle_button_ex(image, id, size, tint, naui_theme_color(uph_ui__style_key("frame_bg_color")), NAUI_CORNER_ALL, enabled);
}

static Leaf_BoundingBox uph_ui__textfield_line_box(Leaf_BoundingBox box, float font_size)
{
    const float line_height = NAUI_MIN(naui_measure_text("Ag", 2, font_size, 0).y, box.height);
    box.y += (box.height - line_height) * 0.5f;
    box.height = line_height;
    return box;
}

static void uph_ui_textfield_text_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldDrawData *draw_data = (Uph_TextfieldDrawData*)user_data;
    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    const Leaf_BoundingBox clip = box;
    box = uph_ui__textfield_line_box(box, font_size);

    naui_push_clip_rect(clip.x, clip.y, clip.width, clip.height);
    if (draw_data->value->length)
    {
        naui_draw_text(
            (Naui_Vec2) { box.x - draw_data->scroll_x, box.y },
            draw_data->value->data,
            font_size,
            0,
            naui_theme_color("uph_ui_text_color")
        );
    }
    else if (draw_data->placeholder)
    {
        naui_draw_text(
            (Naui_Vec2) { box.x, box.y },
            draw_data->placeholder,
            font_size,
            0,
            naui_theme_color("uph_ui_text_disabled_color")
        );
    }
    naui_pop_clip_rect();
}

static void uph_ui_textfield_selection_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldData* data = (Uph_TextfieldData*)user_data;

    if (!data->select_active || data->select_start == data->cursor)
        return;

    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    box = uph_ui__textfield_line_box(box, font_size);
    const float draw_x = box.x - data->scroll_x;

    size_t lo = NAUI_MIN(data->cursor, data->select_start);
    size_t hi = NAUI_MAX(data->cursor, data->select_start);

    const Naui_Vec2 lo_pos = naui_measure_text(data->target_string->data, lo, font_size, 0);
    const Naui_Vec2 hi_pos = naui_measure_text(data->target_string->data, hi, font_size, 0);

    naui_push_clip_rect(box.x, box.y, box.width, box.height);
    naui_fill_rect(
        (Naui_Vec2) { draw_x + lo_pos.x, box.y },
        (Naui_Vec2) { hi_pos.x - lo_pos.x, box.height },
        naui_theme_color("uph_ui_selection_color"),
        0.0f,
        NAUI_CORNER_NONE
    );
    naui_pop_clip_rect();
}

static void uph_ui_textfield_cursor(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldData* data = (Uph_TextfieldData*)user_data;

    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    box = uph_ui__textfield_line_box(box, font_size);
    const float draw_x = box.x - data->scroll_x;

    const Naui_Vec2 text_size = naui_measure_text(data->target_string->data, data->cursor, font_size, 0);
    const float cursor_width = NAUI_DPI(2.0f);

    naui_push_clip_rect(box.x, box.y, box.width + cursor_width, box.height);
    naui_fill_rect(
        (Naui_Vec2) {
            draw_x + text_size.x,
            box.y
        },
        (Naui_Vec2) {
            cursor_width,
            box.height
        },
        naui_theme_color("uph_ui_text_color"),
        0.0f,
        NAUI_CORNER_NONE
    );
    naui_pop_clip_rect();
}

bool uph_ui_textfield_active(const Leaf_ID id)
{
    const Uph_TextfieldData *data = &uph_global_widget_data.textfield_data;
    return data->id.value == id.value && data->mode == UPH_UI_EDIT_MODE_TEXT;
}

static void uph_ui__textfield_copy_selection(const Uph_TextfieldData *data)
{
    if (!data->select_active || data->select_start == data->cursor)
        return;

    const size_t lo = NAUI_MIN(data->cursor, data->select_start);
    const size_t hi = NAUI_MAX(data->cursor, data->select_start);
    Naui_String *s = data->target_string;

    const char saved = s->data[hi];
    s->data[hi] = '\0';
    naui_app_set_clipboard(s->data + lo);
    s->data[hi] = saved;
}

static void uph_ui__textfield_paste(Uph_TextfieldData *data, Naui_String *value, Uph_UITextFieldFlags flags)
{
    const char *clip = naui_app_get_clipboard();
    if (!clip || !*clip)
        return;

    uph_ui__textfield_delete_selection(data);

    const size_t clip_length = strlen(clip);
    size_t i = 0;
    while (i < clip_length)
    {
        uint32_t codepoint = 0;
        const size_t step = naui_utf8_next(clip + i, clip_length - i, &codepoint);
        if (step == 0)
        {
            ++i;
            continue;
        }
        i += step;

        if (codepoint < 0x20 || codepoint == 0x7F)
            continue;

        if (!uph_ui__char_allowed(codepoint, flags, value, data->cursor))
            continue;

        if (!uph_ui__insert_codepoint(value, &data->cursor, codepoint))
            break;
    }
}

static void uph_ui__textfield_layers(Uph_TextfieldData *data, Uph_TextfieldDrawData *draw_data, bool active)
{
    if (active)
    {
        leaf({
            .custom_draw = uph_ui_textfield_selection_draw,
            .custom_draw_data = LEAF_DATA_SLICE(*data),
            .size = {
                .width = LEAF_SIZE_FULL,
                .height = LEAF_SIZE_FULL
            },
            .floating = {
                .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            },
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
        });
    }

    leaf({
        .custom_draw = uph_ui_textfield_text_draw,
        .custom_draw_data = LEAF_DATA_SLICE(*draw_data),
        .size = {
            .width = LEAF_SIZE_FULL,
            .height = LEAF_SIZE_FULL
        },
        .floating = {
            .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        },
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
    });

    leaf({
        .custom_draw = active ? uph_ui_textfield_cursor : NULL,
        .custom_draw_data = LEAF_DATA_SLICE(*data),
        .size = {
            .width = LEAF_SIZE_FULL,
            .height = LEAF_SIZE_FULL
        },
        .floating = {
            .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        },
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
    });
}

bool uph_ui_textfield(Naui_String* value, const Leaf_ID id, Uph_UITextFieldFlags flags, const char *placeholder)
{
    Uph_TextfieldData *data = &uph_global_widget_data.textfield_data;

    bool result = false;

    const bool hovered = uph_ui_widget_hovered(id);
    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    const float text_offset = (flags & UPH_UI_TEXTFIELD_FLAG_SEARCH) ? font_size + NAUI_DPI(6.0f) : 0.0f;

    bool active = (data->id.value == id.value) && (data->mode == UPH_UI_EDIT_MODE_TEXT);

    if (!active && (flags & UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE) && data->id.value == 0)
    {
        uph_ui__begin_text_edit(data, id, value, value->length);
        active = true;
    }

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_IBEAM);

    if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        if (hovered)
        {
            Leaf_BoundingBox box = leaf_get_bounding_box(id);
            float local_x = (float)naui_mouse_x() - box.x - text_offset + (active ? data->scroll_x : 0.0f);

            uph_ui__begin_text_edit(data, id, value, uph_ui__cursor_from_x(value, local_x, font_size));
            active = true;
        }
        else if (active && !(flags & UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE))
        {
            uph_ui__end_edit(data);
            active = false;
            result = true;
        }
        else
        {
            result = true;
        }
    }

    if (active && hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
    {
        data->select_start = 0;
        data->cursor = value->length;
        data->select_active = (value->length > 0);
    }

    if (active && hovered && naui_mouse_dragging(NAUI_MOUSE_LEFT))
    {
        Leaf_BoundingBox box = leaf_get_bounding_box(id);
        float local_x = (float)naui_mouse_x() - box.x - text_offset + data->scroll_x;

        size_t new_cursor = uph_ui__cursor_from_x(value, local_x, font_size);
        if (!data->select_active)
        {
            data->select_start = data->cursor;
            data->select_active = true;
        }
        data->cursor = new_cursor;
        if (data->cursor == data->select_start)
            data->select_active = false;
    }

    if (active)
    {
        const bool ctrl = naui_key_down(NAUI_KEY_CONTROL) || naui_key_down(NAUI_KEY_LCONTROL) || naui_key_down(NAUI_KEY_RCONTROL);
        const bool shift = naui_key_down(NAUI_KEY_SHIFT) || naui_key_down(NAUI_KEY_LSHIFT) || naui_key_down(NAUI_KEY_RSHIFT);

        uint32_t codepoint = naui_app_codepoint();
        while (codepoint)
        {
            if (codepoint != '\b' && codepoint != 0x7F && codepoint != '\t' && codepoint != '\x1b' && codepoint >= 0x20)
            {
                if (uph_ui__char_allowed(codepoint, flags, value, data->cursor))
                {
                    uph_ui__textfield_delete_selection(data);
                    uph_ui__insert_codepoint(value, &data->cursor, codepoint);
                }
            }
            codepoint = naui_app_codepoint();
        }

        if (naui_key_pressed_repeat(NAUI_KEY_BACKSPACE))
        {
            if (data->select_active)
                uph_ui__textfield_delete_selection(data);
            else if (data->cursor > 0)
            {
                size_t remove_from = ctrl ? uph_ui__prev_word(value, data->cursor) : uph_ui__prev_char(value, data->cursor);
                naui_string_remove(value, remove_from, data->cursor - remove_from);
                data->cursor = remove_from;
            }
        }
        else if (naui_key_pressed_repeat(NAUI_KEY_DELETE))
        {
            if (data->select_active)
                uph_ui__textfield_delete_selection(data);
            else if (data->cursor < value->length)
            {
                size_t remove_to = ctrl ? uph_ui__next_word(value, data->cursor) : uph_ui__next_char(value, data->cursor);
                naui_string_remove(value, data->cursor, remove_to - data->cursor);
            }
        }
        else if (naui_key_pressed(NAUI_KEY_ENTER))
        {
            uph_ui__end_edit(data);
            active = false;
            result = true;
        }
        else if (naui_key_pressed(NAUI_KEY_ESCAPE) && !(flags & UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE))
        {
            uph_ui__end_edit(data);
            active = false;
        }
        else if (naui_key_pressed(NAUI_KEY_LEFT))
        {
            size_t new_cursor = ctrl ? uph_ui__prev_word(value, data->cursor) : uph_ui__prev_char(value, data->cursor);
            if (shift)
            {
                if (!data->select_active)
                {
                    data->select_start = data->cursor;
                    data->select_active = true;
                }
                data->cursor = new_cursor;
            }
            else
            {
                data->cursor = data->select_active ? NAUI_MIN(data->cursor, data->select_start) : new_cursor;
                data->select_active = false;
            }
        }
        else if (naui_key_pressed(NAUI_KEY_RIGHT))
        {
            size_t new_cursor = ctrl ? uph_ui__next_word(value, data->cursor) : uph_ui__next_char(value, data->cursor);
            if (shift)
            {
                if (!data->select_active)
                {
                    data->select_start = data->cursor;
                    data->select_active = true;
                }
                data->cursor = new_cursor;
            }
            else
            {
                data->cursor = data->select_active ? NAUI_MAX(data->cursor, data->select_start) : new_cursor;
                data->select_active = false;
            }
        }
        else if (naui_key_pressed(NAUI_KEY_HOME))
        {
            if (shift)
            {
                if (!data->select_active)
                {
                    data->select_start = data->cursor;
                    data->select_active = true;
                }
            }
            else data->select_active = false;
            data->cursor = 0;
        }
        else if (naui_key_pressed(NAUI_KEY_END))
        {
            if (shift)
            {
                if (!data->select_active)
                {
                    data->select_start = data->cursor;
                    data->select_active = true;
                }
            }
            else data->select_active = false;
            data->cursor = value->length;
        }
        else if (ctrl && naui_key_pressed(NAUI_KEY_A))
        {
            data->select_start = 0;
            data->cursor = value->length;
            data->select_active = (value->length > 0);
        }
        else if (ctrl && naui_key_pressed(NAUI_KEY_C))
        {
            uph_ui__textfield_copy_selection(data);
        }
        else if (ctrl && naui_key_pressed(NAUI_KEY_X))
        {
            uph_ui__textfield_copy_selection(data);
            uph_ui__textfield_delete_selection(data);
        }
        else if (ctrl && naui_key_pressed(NAUI_KEY_V))
        {
            uph_ui__textfield_paste(data, value, flags);
        }
    }

    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float padding_x = NAUI_DPI(padding.x);
    const bool inline_field = (flags & UPH_UI_TEXTFIELD_FLAG_INLINE) != 0;
    const float padding_y = inline_field ? NAUI_DPI(padding.y) : 0.0f;

    Leaf_BoundingBox field_box = leaf_get_bounding_box(id);
    const float visible_width = NAUI_MAX(field_box.width - padding_x * 2.0f - text_offset, 0.0f);

    if (active)
    {
        const Naui_Vec2 cursor_pos = naui_measure_text(value->data, data->cursor, font_size, 0);
        const Naui_Vec2 full_pos = naui_measure_text(value->data, value->length, font_size, 0);

        if (cursor_pos.x - data->scroll_x < 0.0f)
            data->scroll_x = cursor_pos.x;
        else if (cursor_pos.x - data->scroll_x > visible_width)
            data->scroll_x = cursor_pos.x - visible_width;

        const float max_scroll = NAUI_MAX(full_pos.x - visible_width, 0.0f);
        data->scroll_x = NAUI_CLAMP(data->scroll_x, 0.0f, max_scroll);
    }
    else
    {
        data->scroll_x = 0.0f;
    }

    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_GROW,
            .height = inline_field ? LEAF_SIZE_FIXED(font_size) : UPH_UI_SIZE_EXACT(uph_ui__frame_height())
        },
        .padding = LEAF_PADDING_AXES(padding_x, padding_y),
        .color = {naui_theme_color(uph_ui__style_key("frame_bg_color"))},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .child_gap = (flags & UPH_UI_TEXTFIELD_FLAG_SEARCH) ? text_offset - font_size : 0
    })
    {
        Uph_TextfieldDrawData draw_data = { value, placeholder, data->scroll_x };

        if (flags & UPH_UI_TEXTFIELD_FLAG_SEARCH)
        {
            leaf({
                .size = {LEAF_SIZE_FIXED(font_size), LEAF_SIZE_FIXED(font_size)},
                .image = naui_asset_image("uph_icon_zoom"),
                .color = {naui_theme_color("naui_panel_text_color")}
            });
            leaf({
                .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL}
            })
            {
                uph_ui__textfield_layers(data, &draw_data, active);
            }
        }
        else uph_ui__textfield_layers(data, &draw_data, active);
    }

    if (data->id.value == id.value && data->mode == UPH_UI_EDIT_MODE_TEXT)
        uph_global_widget_data.textfield_submitted_this_frame = true;

    return result;
}

bool uph_ui_drag_float(float *value, const Leaf_ID id, float speed, float min, float max, const char *format, Uph_UIDragFlags flags)
{
    return uph_ui__drag_scalar(id, value, true, speed, min, max, 0, 0, format, flags);
}

bool uph_ui_drag_int(int32_t *value, const Leaf_ID id, float speed, int32_t min, int32_t max, const char *format, Uph_UIDragFlags flags)
{
    return uph_ui__drag_scalar(id, value, false, speed, 0.0f, 0.0f, min, max, format, flags);
}

bool uph_ui_slider_float(float *value, const Leaf_ID id, float min, float max, const char *format, Uph_UISliderFlags flags)
{
    return uph_ui__slider_scalar(id, value, true, min, max, 0, 0, format, flags);
}

bool uph_ui_slider_int(int32_t *value, const Leaf_ID id, int32_t min, int32_t max, const char *format, Uph_UISliderFlags flags)
{
    return uph_ui__slider_scalar(id, value, false, 0.0f, 0.0f, min, max, format, flags);
}

bool uph_ui_checkbox(bool *value, const Leaf_ID id)
{
    const bool hovered = uph_ui_widget_hovered(id);

    const Leaf_Color bg_color = naui_theme_color(uph_ui__style_key("frame_bg_color"));
    const Leaf_Color hovered_bg_color = naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));
    const Leaf_Color check_color = naui_theme_color("uph_ui_slider_fill_color");

    const float size = uph_ui__frame_height() * 0.7f;
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    leaf({
        .id = id,
        .size = { .width = UPH_UI_SIZE_EXACT(size), .height = UPH_UI_SIZE_EXACT(size) },
        .color = {hovered ? hovered_bg_color : bg_color},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        if (*value)
        {
            leaf({
                .size = { .width = LEAF_SIZE_PERCENT(0.6f), .height = LEAF_SIZE_PERCENT(0.6f) },
                .color = {check_color},
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding) * 0.5f, NAUI_CORNER_ALL),
                .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                .floating = {
                    .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                    .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
                }
            });
        }
    }

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        *value = !(*value);
        return true;
    }

    return false;
}

static void uph_ui_dropdown_arrow_draw(Leaf_BoundingBox box, void *user_data)
{
    const Leaf_Color color = *(const Leaf_Color*)user_data;

    const float width = box.width * 0.7f;
    const float height = width * 0.55f;
    const float center_x = box.x + box.width * 0.5f;
    const float center_y = box.y + box.height * 0.5f;

    const Naui_Vec2 points[3] =
    {
        { center_x - width * 0.5f, center_y - height * 0.5f },
        { center_x + width * 0.5f, center_y - height * 0.5f },
        { center_x,                center_y + height * 0.5f }
    };
    naui_fill_polygon(points, 3, color);
}

bool uph_ui_dropdown(const char *const *items, const uint32_t item_count, uint32_t *current_index, const Leaf_ID id)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    Uph_UIMenuNode *menu = uph_ui__menu_root(data, id);
    bool result = false;

    const bool was_open = uph_ui__menu_is_open(data, menu);
    const bool hovered = uph_ui_widget_hovered(id) || (was_open && leaf_hovered(id));

    if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            if (was_open)
                uph_ui__close_menu(data);
            else
                uph_ui__open_menu(data, menu, false);
        }
    }

    const bool open = uph_ui__menu_is_open(data, menu);
    const uint32_t shown_index = (*current_index < item_count) ? *current_index : 0;
    const char *shown_text = item_count > 0 ? items[shown_index] : "";

    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float rounding = naui_theme_float("uph_ui_frame_rounding");
    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
    Leaf_Color arrow_color = text_color;

    Leaf_Color background = naui_theme_color(uph_ui__style_key("frame_bg_color"));
    if (open)
        background = naui_theme_color(uph_ui__style_key("frame_pressed_bg_color"));
    else if (hovered)
        background = naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));

    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_GROW, .height = UPH_UI_SIZE_EXACT(uph_ui__frame_height()) },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
        .child_gap = NAUI_DPI(6.0f),
        .color = { background },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
            .clip_children = true
        })
        {
            leaf_text(shown_text, {
                .color = { text_color },
                .font_size = { font_size }
            });
        }

        leaf({
            .size = { .width = LEAF_SIZE_FIXED(font_size), .height = LEAF_SIZE_FIXED(font_size) },
            .custom_draw = uph_ui_dropdown_arrow_draw,
            .custom_draw_data = LEAF_DATA_SLICE(arrow_color)
        });
    }

    for (uint32_t i = 0; i < item_count; i++)
    {
        if (uph_ui_menu_item((Uph_UIMenuID)menu, items[i], (Leaf_ID){ .value = id.value + i + 1 }) && i != shown_index)
        {
            *current_index = i;
            result = true;
        }
    }

    return result;
}

bool uph_ui_knob_float(float *value, const Leaf_ID id, float min, float max, float default_value, Naui_Color color, Uph_UIKnobStartType start_type)
{
    Uph_KnobData *kdata = &uph_global_widget_data.knob_data;

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
            const float delta_pixels = (float)(kdata->last_mouse_y - my); // up = increase
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
    const Naui_Color body_color = naui_theme_color(uph_ui__style_key(active ? "frame_pressed_bg_color" : (hovered ? "frame_hovered_bg_color" : "frame_bg_color")));
    Uph_KnobDrawData draw_data = {
        .color = color,
        .track_color = naui_theme_color(uph_ui__style_key("knob_track_color")),
        .body_color = body_color,
        .ring_color = active || hovered ?
            uph_color_mix(naui_theme_color("uph_ui_knob_ring_color"), naui_theme_color("uph_ui_text_color"), 0.4f) :
            naui_theme_color("uph_ui_knob_ring_color"),
        .pointer_color = naui_theme_color("uph_ui_text_color"),
        .t = t,
        .start_type = start_type
    };

    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_DERIVED },
        .custom_draw = uph_ui__knob_draw,
        .custom_draw_data = LEAF_DATA_SLICE(draw_data),
        .aspect_ratio = 1.0f
    });

    return result;
}

typedef struct
{
    Leaf_Color color;
    bool up;
}
Uph_UITriangleDrawData;

static void uph_ui__triangle_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_UITriangleDrawData *data = (const Uph_UITriangleDrawData*)user_data;

    const float width = NAUI_MIN(box.width, box.height * 1.6f) * 0.6f;
    const float height = width * 0.55f;
    const float center_x = box.x + box.width * 0.5f;
    const float center_y = box.y + box.height * 0.5f;
    const float tip_y = data->up ? center_y - height * 0.5f : center_y + height * 0.5f;
    const float base_y = data->up ? center_y + height * 0.5f : center_y - height * 0.5f;

    const Naui_Vec2 points[3] =
    {
        { center_x - width * 0.5f, base_y },
        { center_x + width * 0.5f, base_y },
        { center_x,                tip_y }
    };
    naui_fill_polygon(points, 3, data->color);
}

static Leaf_Color uph_ui__dim_text_color(void)
{
    Leaf_Color color = naui_theme_color("uph_ui_text_color");
    color.a = (uint8_t)(color.a * 0.6f);
    return color;
}

static float uph_ui__tall_frame_height(void)
{
    return uph_ui__frame_height() + NAUI_DPI(8.0f);
}

static bool uph_ui__triangle_button(const Leaf_ID id, bool up, float width, float height, Naui_CornerFlags corners)
{
    const bool hovered = uph_ui_widget_hovered(id);
    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Leaf_Color background = LEAF_COLOR_TRANSPARENT;
    if (hovered)
        background = naui_theme_color(uph_ui__style_key(naui_mouse_down(NAUI_MOUSE_LEFT) ? "frame_pressed_bg_color" : "frame_hovered_bg_color"));

    Uph_UITriangleDrawData draw_data = {
        .color = hovered ? naui_theme_color("uph_ui_text_color") : uph_ui__dim_text_color(),
        .up = up
    };

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(width), LEAF_SIZE_FIXED(height)},
        .color = {background},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), corners),
        .custom_draw = uph_ui__triangle_draw,
        .custom_draw_data = LEAF_DATA_SLICE(draw_data)
    });

    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void uph_ui__frame_icon(const char *image, float size)
{
    leaf({
        .size = {LEAF_SIZE_FIXED(size), LEAF_SIZE_FIXED(size)},
        .image = naui_asset_image(image),
        .color = {uph_ui__dim_text_color()}
    });
}

static void uph_ui__frame_divider(float height)
{
    Leaf_Color color = naui_theme_color("uph_ui_text_color");
    color.a = (uint8_t)(color.a * 0.12f);
    leaf({
        .size = {LEAF_SIZE_FIXED(NAUI_MAX(1.0f, NAUI_DPI(1.0f))), LEAF_SIZE_FIXED(height)},
        .color = {color}
    });
}

bool uph_ui_bpm(float *value, const char *id, float min, float max)
{
    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float frame_height = uph_ui__tall_frame_height();
    const float arrow_width = NAUI_DPI(16.0f);
    const float arrow_height = (frame_height - NAUI_DPI(6.0f)) * 0.5f;
    const float icon_size = font_size * 1.15f;
    const Leaf_ID tap_id = leaf_id_indexed(id, 3);
    bool changed = false;

    const bool tap_hovered = uph_ui_widget_hovered(tap_id);
    if (tap_hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        float tapped_bpm;
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && _uph_metronome_tap(&tapped_bpm))
        {
            *value = NAUI_CLAMP(roundf(tapped_bpm * 100.0f) / 100.0f, min, max);
            changed = true;
        }
    }

    Leaf_Color tap_background = LEAF_COLOR_TRANSPARENT;
    if (tap_hovered)
        tap_background = naui_theme_color(uph_ui__style_key(naui_mouse_down(NAUI_MOUSE_LEFT) ? "frame_pressed_bg_color" : "frame_hovered_bg_color"));

    leaf({
        .id = leaf_id(id),
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FIT, UPH_UI_SIZE_EXACT(frame_height)},
        .padding = { .left = NAUI_DPI(3.0f), .right = NAUI_DPI(3.0f) },
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .child_gap = NAUI_DPI(6.0f),
        .color = {naui_theme_color(uph_ui__style_key("frame_bg_color"))},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL)
    })
    {
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
        })
        {
            leaf({
                .id = tap_id,
                .size = {LEAF_SIZE_FIXED(icon_size + NAUI_DPI(padding.x) * 2.0f), LEAF_SIZE_FIXED(frame_height - NAUI_DPI(6.0f))},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .color = {tap_background},
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL)
            })
            {
                leaf({
                    .size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
                    .image = naui_asset_image("uph_icon_metronome"),
                    .color = {tap_hovered ? naui_theme_color("uph_ui_text_color") : uph_ui__dim_text_color()}
                });
            }

            leaf({
                .direction = LEAF_DIRECTION_VERTICAL,
                .size = {LEAF_SIZE_FIXED(NAUI_DPI(48.0f)), LEAF_SIZE_FIT},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                if (uph_ui_drag_float(value, leaf_id_indexed(id, 0), 1.0f, min, max, "%.2f", UPH_UI_DRAG_FLAG_CLAMPED | UPH_UI_DRAG_FLAG_FRAMELESS))
                    changed = true;

                leaf_text("BPM", {
                    .font_size = font_size * 0.72f,
                    .color = uph_ui__dim_text_color()
                });
            }
        }

        uph_ui__frame_divider(frame_height - NAUI_DPI(12.0f));

        leaf({
            .direction = LEAF_DIRECTION_VERTICAL,
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT}
        })
        {
            if (uph_ui__triangle_button(leaf_id_indexed(id, 1), true, arrow_width, arrow_height, NAUI_CORNER_TL | NAUI_CORNER_TR))
            {
                *value = NAUI_CLAMP(floorf(*value) + 1.0f, min, max);
                changed = true;
            }
            if (uph_ui__triangle_button(leaf_id_indexed(id, 2), false, arrow_width, arrow_height, NAUI_CORNER_BL | NAUI_CORNER_BR))
            {
                *value = NAUI_CLAMP(ceilf(*value) - 1.0f, min, max);
                changed = true;
            }
        }
    }

    return changed;
}

bool uph_ui_time_signature(Uph_TimeSignature *signature, const char *id)
{
    static const uint32_t denominator_values[] = { 2, 4, 8, 16 };
    static const char *denominator_labels[] = { "2", "4", "8", "16" };
    static const Uph_TimeSignature presets[] = {
        {2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 8}, {7, 8}, {9, 8}, {12, 8}, {15, 16}
    };
    static const char *preset_labels[] = { "2/4", "3/4", "4/4", "5/4", "6/8", "7/8", "9/8", "12/8", "15/16" };
    const uint32_t preset_count = sizeof(presets) / sizeof(presets[0]);

    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    const Leaf_ID frame_id = leaf_id(id);
    const Leaf_ID denominator_id = leaf_id_indexed(id, 1);
    const Leaf_ID arrow_id = leaf_id_indexed(id, 2);
    Uph_UIMenuNode *menu = uph_ui__menu_root(data, frame_id);

    const float font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size")));
    const Naui_Vec2 padding = naui_theme_vec2(uph_ui__style_key("frame_padding"));
    const float frame_height = uph_ui__tall_frame_height();
    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
    bool changed = false;

    const bool was_open = uph_ui__menu_is_open(data, menu);
    const bool denominator_hovered = uph_ui_widget_hovered(denominator_id) || (was_open && leaf_hovered(denominator_id));
    const bool arrow_hovered = uph_ui_widget_hovered(arrow_id) || (was_open && leaf_hovered(arrow_id));
    if (denominator_hovered || arrow_hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            if (was_open)
                uph_ui__close_menu(data);
            else
                uph_ui__open_menu(data, menu, false);
        }
    }
    const bool open = uph_ui__menu_is_open(data, menu);

    char denominator_text[8];
    snprintf(denominator_text, sizeof(denominator_text), "%u", (unsigned)signature->denominator);

    Leaf_Color arrow_background = LEAF_COLOR_TRANSPARENT;
    if (open)
        arrow_background = naui_theme_color(uph_ui__style_key("frame_pressed_bg_color"));
    else if (arrow_hovered)
        arrow_background = naui_theme_color(uph_ui__style_key("frame_hovered_bg_color"));

    Uph_UITriangleDrawData arrow_data = {
        .color = (open || arrow_hovered) ? text_color : uph_ui__dim_text_color(),
        .up = false
    };

    leaf({
        .id = frame_id,
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FIT, UPH_UI_SIZE_EXACT(frame_height)},
        .padding = { .left = NAUI_DPI(padding.x), .right = NAUI_DPI(3.0f) },
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .child_gap = NAUI_DPI(6.0f),
        .color = {naui_theme_color(uph_ui__style_key("frame_bg_color"))},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL)
    })
    {
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .child_gap = NAUI_DPI(1.0f)
        })
        {
            uph_ui__frame_icon("uph_icon_treble-clef", font_size * 1.9f);

            leaf({
                .direction = LEAF_DIRECTION_VERTICAL,
                .size = {LEAF_SIZE_FIXED(NAUI_DPI(16.0f)), LEAF_SIZE_FIT},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                if (uph_ui_drag_int((int32_t*)&signature->numerator, leaf_id_indexed(id, 0), 0.1f, 1, 32, "%d", UPH_UI_DRAG_FLAG_CLAMPED | UPH_UI_DRAG_FLAG_FRAMELESS))
                    changed = true;

                leaf({
                    .id = denominator_id,
                    .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
                    .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
                })
                {
                    leaf_text(denominator_text, {
                        .font_size = font_size,
                        .color = text_color
                    });
                }
            }
        }

        uph_ui__frame_divider(frame_height - NAUI_DPI(12.0f));

        leaf({
            .id = arrow_id,
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(16.0f)), LEAF_SIZE_FIXED(frame_height - NAUI_DPI(6.0f))},
            .color = {arrow_background},
            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
            .custom_draw = uph_ui__triangle_draw,
            .custom_draw_data = LEAF_DATA_SLICE(arrow_data)
        });
    }

    for (uint32_t i = 0; i < preset_count; i++)
    {
        if (uph_ui_menu_item((Uph_UIMenuID)menu, preset_labels[i], leaf_id_indexed(id, 16 + i)))
        {
            signature->numerator = presets[i].numerator;
            signature->denominator = presets[i].denominator;
            changed = true;
        }
    }

    Uph_UIMenuID note_menu = uph_ui_submenu((Uph_UIMenuID)menu, "Note Value", leaf_id_indexed(id, 8));
    for (uint32_t i = 0; i < 4; i++)
    {
        if (uph_ui_menu_item(note_menu, denominator_labels[i], leaf_id_indexed(id, 32 + i)))
        {
            signature->denominator = denominator_values[i];
            changed = true;
        }
    }

    return changed;
}

int32_t uph_ui_track_color_palette(const Uph_Track *track)
{
    const Naui_List(Naui_Color) palette = naui_theme_color_list("uph_track_palette");
    const uint32_t color_count = (uint32_t)naui_list_len(palette);
    if (color_count == 0)
        return -1;

    const uint32_t rows = 2;
    const uint32_t columns = (uint32_t)ceilf((color_count + rows - 1) / rows);
    const int32_t current = track->color_index % (int32_t)color_count;

    const uint32_t font_size = NAUI_DPI(naui_theme_float(uph_ui__style_key("font_size"))) * 1.5f;
    int32_t picked = -1;

    leaf({
        .child_gap = NAUI_DPI(4)
    })
    {
        for (uint32_t i = 0; i < rows; i++)
        {
            leaf({
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .child_gap = NAUI_DPI(4)
            })
            {
                for (uint32_t j = 0; j < columns; j++)
                {
                    const int32_t index = j + i * columns;
                    const Leaf_ID id = leaf_id_indexed("uph_track_color_picker", index);
                    if (leaf_hovered(id) && naui_mouse_pressed(NAUI_MOUSE_LEFT))
                    {
                        picked = index;
                        uph_ui_close_menu();
                    }
                    leaf({
                        .id = id,
                        .size = {LEAF_SIZE_FIXED(font_size), LEAF_SIZE_FIXED(font_size)},
                        .color = palette[index],
                        .border = {
                            .width = NAUI_DPI(2),
                            .color = track->color_index == index ?
                                LEAF_COLOR_WHITE : LEAF_COLOR_TRANSPARENT,
                            .sides = LEAF_SIDE_ALL
                        }
                    });
                }
            }
        }
    }

    return picked;
}

#pragma endregion