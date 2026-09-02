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

typedef struct Uph_UIMenuNode Uph_UIMenuNode;
struct Uph_UIMenuNode
{
    const char *text;
    Leaf_ID element_id;
    Uph_UIMenuNode *next_sibling;
    Uph_UIMenuNode *first_child;
    Uph_UIMenuNode *last_child;
};

typedef struct
{
    uint32_t id_counter;

    Uph_TextfieldData textfield_data;
    Uph_DragData drag_data;

    Naui_Arena menu_arena;

    Uph_UIMenuNode *current_open_menu;
    Naui_Vec2 current_context_menu_position;
    bool is_current_menu_context;
    bool menu_opened_last_frame;

    bool any_widget_hovered;
}
Uph_GlobalWidgetData;
static Uph_GlobalWidgetData uph_global_widget_data = { 0 };

bool uph_ui_widget_hovered(const Leaf_ID id)
{
    const bool result = !uph_ui_any_widget_hovered() && ((!naui_current_panel() && !naui_any_panel_hovered()) || (naui_current_panel() && naui_panel_hovered(naui_current_panel()))) && leaf_hovered(id);
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
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));

    leaf({
        .id = node->element_id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_X_CENTER},
        .color = leaf_hovered(node->element_id) ?
            naui_theme_color("uph_ui_frame_hovered_bg_color") :
            LEAF_COLOR_TRANSPARENT,
        .child_gap = NAUI_DPI(8.0f),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {    
        leaf_text(node->text, {
            .color = text_color,
            .font_size = font_size
        });
        if (node->first_child)
        {
            leaf({
                .size = {LEAF_SIZE_FIXED(font_size), LEAF_SIZE_FIXED(font_size)},
                .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL),
                .image = naui_asset_image("uph_icon_dropdown"),
                .color = text_color
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
        .color = naui_theme_color("uph_ui_dropdown_bg_color"),
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

static void uph_ui_render_menu_dropdown(Uph_GlobalWidgetData *data)
{
    if (data->current_open_menu)
    {
        Uph_UIMenuNode *current_node = data->current_open_menu->first_child;
        if (!current_node)
        {
            data->current_open_menu = NULL;
            return;
        }

        naui_occlude_all_panels();

        if (data->menu_opened_last_frame &&
            (naui_mouse_pressed(NAUI_MOUSE_LEFT) || naui_mouse_pressed(NAUI_MOUSE_RIGHT) || naui_mouse_pressed(NAUI_MOUSE_MIDDLE)) &&
            !uph_ui_menu_dropdown_hovered(data->current_open_menu))
        {
            data->current_open_menu = NULL;
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

void uph_ui_widgets_flush(void)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    uph_ui_render_menu_dropdown(data);

    data->id_counter = 0;
    data->any_widget_hovered = false;
    naui_arena_reset(&data->menu_arena);
}

Uph_UIMenuID uph_ui_menu(const char *name, const Leaf_ID element_id)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;

    Uph_UIMenuNode *menu = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));
    menu->element_id = element_id;

    const bool hovered = uph_ui_widget_hovered(element_id);
    if (hovered)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            data->current_open_menu = data->current_open_menu ? NULL : menu;
            data->is_current_menu_context = false;
            data->menu_opened_last_frame = false;
        }
        else if (!data->is_current_menu_context && data->current_open_menu)
            data->current_open_menu = menu;
    }


    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    leaf({
        .id = element_id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = (data->current_open_menu == menu || hovered) ?
            naui_theme_color("uph_ui_frame_hovered_bg_color") : LEAF_COLOR_TRANSPARENT,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL)
    })
    {
        leaf_text(name, {
            .color = naui_theme_color("uph_ui_text_color"),
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"))
        });
    }

    return (Uph_UIMenuID)menu;
}

Uph_UIMenuID uph_ui_context_menu(void)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    Uph_UIMenuNode *menu = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));
    return (Uph_UIMenuID)menu;
}

void uph_ui_open_context_menu(Uph_UIMenuID menu)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;
    Uph_UIMenuNode *node = (Uph_UIMenuNode*)menu;
    data->current_open_menu = node;
    data->current_context_menu_position = (Naui_Vec2){naui_mouse_x(), naui_mouse_y()};
    data->is_current_menu_context = true;
    data->menu_opened_last_frame = false;
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
    menu->text = name;
    uph_ui_append_menu_child(parent, menu);

    return (Uph_UIMenuID)menu;
}

bool uph_ui_menu_item(Uph_UIMenuID menu_id, const char *name, const Leaf_ID element_id)
{
    Uph_GlobalWidgetData *data = &uph_global_widget_data;

    Uph_UIMenuNode *parent = (Uph_UIMenuNode*)menu_id;
    Uph_UIMenuNode *item = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));
    item->element_id = element_id;
    item->text = name;
    uph_ui_append_menu_child(parent, item);

    bool result;
    if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && leaf_hovered(element_id))
    {
        uph_global_widget_data.current_open_menu = NULL;
        result = true;
    }
    else result = false;

    return result;
}

bool uph_ui_text_button(const char *string, const Leaf_ID id)
{
    const Leaf_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
    const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");
    const Leaf_Color pressed_bg_color = naui_theme_color("uph_ui_frame_pressed_bg_color");

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    Leaf_Color color;
    if (hovered)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
            color = pressed_bg_color;
        else color = hovered_bg_color;
    }
    else color = bg_color;
    
    leaf({
        .id = id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        leaf_text(string, {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = text_color
        });
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

bool uph_ui_image_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners)
{
    const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");
    const Leaf_Color pressed_bg_color = naui_theme_color("uph_ui_frame_pressed_bg_color");

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    Leaf_Color color;
    if (hovered)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
            color = pressed_bg_color;
        else color = hovered_bg_color;
    }
    else color = bg_color;
    
    leaf({
        .id = id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), corners)
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(size.x), LEAF_SIZE_FIXED(size.y)},
            .color = tint,
            .image = (void*)image
        });
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

bool uph_ui_image_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint)
{
    return uph_ui_image_button_ex(image, id, size, tint, naui_theme_color("uph_ui_frame_bg_color"), NAUI_CORNER_ALL);
}

bool uph_ui_text_toggle_button(const char *string, const Leaf_ID id, bool enabled)
{
    const Leaf_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
    const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");
    const Leaf_Color pressed_bg_color = naui_theme_color("uph_ui_frame_pressed_bg_color");

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    Leaf_Color color = enabled ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    
    leaf({
        .id = id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        leaf_text(string, {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = text_color
        });
    }

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return true;

    return false;
}

bool uph_ui_image_toggle_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners, bool enabled)
{
    const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");
    const Leaf_Color pressed_bg_color = naui_theme_color("uph_ui_frame_pressed_bg_color");

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    Leaf_Color color = enabled ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    
    leaf({
        .id = id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), corners)
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(size.x), LEAF_SIZE_FIXED(size.y)},
            .color = tint,
            .image = (void*)image
        });
    }

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return true;

    return false;
}

bool uph_ui_image_toggle_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, bool enabled)
{
    return uph_ui_image_toggle_button_ex(image, id, size, tint, naui_theme_color("uph_ui_frame_bg_color"), NAUI_CORNER_ALL, enabled);
}

static bool uph_ui__is_word_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
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
    if (!(flags & UPH_UI_TEXTFIELD_NUMBER_ONLY))
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

typedef struct
{
    Naui_String *value;
    const char *placeholder;
    float scroll_x;
}
Uph_TextfieldDrawData;

static void uph_ui_textfield_text_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldDrawData *draw_data = (Uph_TextfieldDrawData*)user_data;

    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));

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
}

static void uph_ui_textfield_selection_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldData* data = (Uph_TextfieldData*)user_data;

    if (!data->select_active || data->select_start == data->cursor)
        return;

    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const float draw_x = box.x - data->scroll_x;

    size_t lo = NAUI_MIN(data->cursor, data->select_start);
    size_t hi = NAUI_MAX(data->cursor, data->select_start);

    const Naui_Vec2 lo_pos = naui_measure_text(data->target_string->data, lo, font_size, 0);
    const Naui_Vec2 hi_pos = naui_measure_text(data->target_string->data, hi, font_size, 0);

    naui_fill_rect(
        (Naui_Vec2) { draw_x + lo_pos.x, box.y },
        (Naui_Vec2) { hi_pos.x - lo_pos.x, box.height },
        naui_theme_color("uph_ui_selection_color"),
        0.0f,
        NAUI_CORNER_NONE
    );
}

static void uph_ui_textfield_cursor(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldData* data = (Uph_TextfieldData*)user_data;

    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const float draw_x = box.x - data->scroll_x;

    const Naui_Vec2 text_size = naui_measure_text(data->target_string->data, data->cursor, font_size, 0);

    naui_fill_rect(
        (Naui_Vec2) {
            draw_x + text_size.x,
            box.y
        },
        (Naui_Vec2) {
            NAUI_DPI(2.0f),
            box.height
        },
        naui_theme_color("uph_ui_text_color"),
        0.0f,
        NAUI_CORNER_NONE
    );
}

static size_t uph_ui__cursor_from_x(const Naui_String *value, float local_x, float font_size)
{
    if (local_x <= 0.0f)
        return 0;

    size_t best = value->length;
    float best_dist = fabsf(naui_measure_text(value->data, value->length, font_size, 0).x - local_x);

    for (size_t i = 0; i <= value->length; ++i)
    {
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

bool uph_ui_textfield(Naui_String* value, const Leaf_ID id, Uph_UITextFieldFlags flags, const char *placeholder)
{
    Uph_TextfieldData *data = &uph_global_widget_data.textfield_data;

    bool result = false;

    const bool hovered = uph_ui_widget_hovered(id);
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));

    bool active = (data->id.value == id.value) && (data->mode == UPH_UI_EDIT_MODE_TEXT);

    if (!active && (flags & UPH_UI_TEXTFIELD_ALWAYS_ACTIVE) && data->id.value == 0)
    {
        uph_ui__begin_text_edit(data, id, value, value->length);
        active = true;
    }

    if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        if (hovered)
        {
            Leaf_BoundingBox box = leaf_get_bounding_box(id);
            float local_x = (float)naui_mouse_x() - box.x + (active ? data->scroll_x : 0.0f);

            uph_ui__begin_text_edit(data, id, value, uph_ui__cursor_from_x(value, local_x, font_size));
            active = true;
        }
        else if (active && !(flags & UPH_UI_TEXTFIELD_ALWAYS_ACTIVE))
        {
            uph_ui__end_edit(data);
            active = false;
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
        float local_x = (float)naui_mouse_x() - box.x + data->scroll_x;

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

        uint32_t codepoint = naui_app_char_pressed();
        if (codepoint && codepoint != '\b' && codepoint != 0x7F && codepoint != '\t' && codepoint != '\x1b' && codepoint >= 0x20)
        {
            if (uph_ui__char_allowed(codepoint, flags, value, data->cursor))
            {
                uph_ui__textfield_delete_selection(data);
                naui_string_append_char_at(value, (char)codepoint, data->cursor++);
            }
        }

        if (naui_key_pressed(NAUI_KEY_BACKSPACE))
        {
            if (data->select_active)
                uph_ui__textfield_delete_selection(data);
            else if (data->cursor > 0)
            {
                size_t remove_from = ctrl ? uph_ui__prev_word(value, data->cursor) : data->cursor - 1;
                naui_string_remove(value, remove_from, data->cursor - remove_from);
                data->cursor = remove_from;
            }
        }
        else if (naui_key_pressed(NAUI_KEY_DELETE))
        {
            if (data->select_active)
                uph_ui__textfield_delete_selection(data);
            else if (data->cursor < value->length)
            {
                size_t remove_to = ctrl ? uph_ui__next_word(value, data->cursor) : data->cursor + 1;
                naui_string_remove(value, data->cursor, remove_to - data->cursor);
            }
        }
        else if (naui_key_pressed(NAUI_KEY_ENTER))
        {
            uph_ui__end_edit(data);
            active = false;
            result = true;
        }
        else if (naui_key_pressed(NAUI_KEY_ESCAPE) && !(flags & UPH_UI_TEXTFIELD_ALWAYS_ACTIVE))
        {
            uph_ui__end_edit(data);
            active = false;
        }
        else if (naui_key_pressed(NAUI_KEY_LEFT))
        {
            size_t new_cursor = ctrl ? uph_ui__prev_word(value, data->cursor) : (data->cursor > 0 ? data->cursor - 1 : 0);
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
            size_t new_cursor = ctrl ? uph_ui__next_word(value, data->cursor) : NAUI_MIN(data->cursor + 1, value->length);
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
    }

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const float padding_x = NAUI_DPI(padding.x);
    const float padding_y = NAUI_DPI(padding.y);

    Leaf_BoundingBox field_box = leaf_get_bounding_box(id);
    const float visible_width = NAUI_MAX(field_box.width - padding_x * 2.0f, 0.0f);

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
            .height = LEAF_SIZE_FIXED(font_size)
        },
        .padding = LEAF_PADDING_AXES(padding_x, padding_y),
        .color = naui_theme_color("uph_ui_frame_bg_color"),
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL)
    })
    {
        naui_push_clip_rect(field_box.x + padding_x, field_box.y, visible_width, field_box.height);

        Uph_TextfieldDrawData draw_data = { value, placeholder, data->scroll_x };

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
            .custom_draw_data = LEAF_DATA_SLICE(draw_data),
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

        naui_pop_clip_rect();
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
        bool sub_result = uph_ui_textfield(&edit_buffer, id, UPH_UI_TEXTFIELD_NUMBER_ONLY, NULL);

        if (sub_result)
        {
            if (is_float)
            {
                float parsed = (float)atof(edit_buffer.data);
                if (flags & UPH_UI_DRAG_CLAMPED)
                    parsed = NAUI_CLAMP(parsed, min_f, max_f);
                *(float*)value = parsed;
            }
            else
            {
                int32_t parsed = (int32_t)atoi(edit_buffer.data);
                if (flags & UPH_UI_DRAG_CLAMPED)
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
                if (flags & UPH_UI_DRAG_CLAMPED)
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
                    if (flags & UPH_UI_DRAG_CLAMPED)
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

    const Leaf_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
    const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");
    const Leaf_Color pressed_bg_color = naui_theme_color("uph_ui_frame_pressed_bg_color");

    const Leaf_Color color = dragging_this ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_GROW,
            .height = LEAF_SIZE_FIXED(font_size)
        },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
		.clip_children = true
    })
    {
        leaf_text(display.data, {
            .color = naui_theme_color("uph_ui_text_color"),
            .font_size = font_size
        });
    }

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

static bool uph_ui__slider_scalar(const Leaf_ID id, void *value, bool is_float, float min_f, float max_f, int32_t min_i, int32_t max_i, const char *format, Uph_UISliderFlags flags)
{
    Uph_TextfieldData *tdata = &uph_global_widget_data.textfield_data;

    bool result = false;

    static Naui_String edit_buffer;

    bool text_editing = (tdata->id.value == id.value) && (tdata->mode == UPH_UI_EDIT_MODE_TEXT) && (tdata->target_number == value);

    const bool hovered = uph_ui_widget_hovered(id);

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
        bool sub_result = uph_ui_textfield(&edit_buffer, id, UPH_UI_TEXTFIELD_NUMBER_ONLY, NULL);
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

    Leaf_BoundingBox box = leaf_get_bounding_box(id);
    const bool vertical = (flags & UPH_UI_SLIDER_VERTICAL) != 0;

    if (hovered && (naui_mouse_pressed(NAUI_MOUSE_LEFT) || naui_mouse_dragging(NAUI_MOUSE_LEFT)))
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

    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_GROW,
            .height = LEAF_SIZE_FIXED(font_size + NAUI_DPI(padding.y) * 2.0f)
        },
        .color = naui_theme_color("uph_ui_frame_bg_color"),
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        leaf({
            .size = vertical ? (Leaf_Size){ .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_PERCENT(t) }
                : (Leaf_Size){ .width = LEAF_SIZE_PERCENT(t), .height = LEAF_SIZE_FULL },
            .color = naui_theme_color("uph_ui_slider_fill_color"),
            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL),
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .floating = {
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
            }
        })
        {
            leaf_text(display.data, {
                .color = naui_theme_color("uph_ui_text_color"),
                .font_size = font_size
            });
        }
    }

    return result;
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

    const Leaf_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
    const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");
    const Leaf_Color check_color = naui_theme_color("uph_ui_slider_fill_color");

    const float size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_FIXED(size), .height = LEAF_SIZE_FIXED(size) },
        .color = hovered ? hovered_bg_color : bg_color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        if (*value)
        {
            leaf({
                .size = { .width = LEAF_SIZE_PERCENT(0.6f), .height = LEAF_SIZE_PERCENT(0.6f) },
                .color = check_color,
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