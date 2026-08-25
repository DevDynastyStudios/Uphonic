typedef struct
{
    Leaf_ID id;
    // flags
    Naui_String* value;
    size_t cursor;
}
Uph_TextfieldData;

typedef struct
{
    uint32_t id_counter;
    
    Uph_DropDownCallback current_dropdown_callback;
    Naui_Vec2 current_dropdown_position;

    bool dropdown_opened_this_frame;
    bool any_widget_hovered;
}
Uph_GlobalWidgetData;
static Uph_GlobalWidgetData uph_global_widget_data = { 0 };

static bool uph_ui_widget_hovered(const Leaf_ID id)
{
    const bool result = !uph_ui_any_widget_hovered() && (!naui_current_panel() || (naui_current_panel() && naui_panel_hovered(naui_current_panel()))) && leaf_hovered(id);
    if (result)
        uph_global_widget_data.any_widget_hovered = true;
    return result;
}

bool uph_ui_any_widget_hovered(void)
{
    return uph_global_widget_data.any_widget_hovered;
}

void uph_ui_widgets_flush(void)
{
    if (uph_global_widget_data.current_dropdown_callback)
    {
        Leaf_ID dropdown_id = leaf_id("uph_current_dropdown");

        const Leaf_Color shadow_color = naui_theme_color("uph_ui_dropdown_shadow_color");
        const Leaf_Color bg_color = naui_theme_color("uph_ui_dropdown_bg_color");

        leaf({
            .id = dropdown_id,
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .floating = {
                .offset = {
                    uph_global_widget_data.current_dropdown_position.x,
                    uph_global_widget_data.current_dropdown_position.y
                }
            },
            .padding = LEAF_PADDING_AXES(NAUI_DPI(6.0f), NAUI_DPI(6.0f)),
            .color = bg_color,
            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(6.0f), LEAF_CORNER_ALL),
            .shadow = {
                .blur_radius = NAUI_DPI(16.0f),
                .color = shadow_color
            }
        })
        {
            uph_global_widget_data.current_dropdown_callback();
        }

        if ((naui_mouse_pressed(NAUI_MOUSE_LEFT) || naui_mouse_pressed(NAUI_MOUSE_MIDDLE) || naui_mouse_pressed(NAUI_MOUSE_RIGHT)) && !leaf_hovered(dropdown_id) && !uph_global_widget_data.dropdown_opened_this_frame && !uph_ui_widget_hovered(dropdown_id))
            uph_ui_close_dropdown();
        
        naui_occlude_all_panels();
    }

    if (uph_global_widget_data.dropdown_opened_this_frame)
        uph_global_widget_data.dropdown_opened_this_frame = false;

    uph_global_widget_data.id_counter = 0;
    uph_global_widget_data.any_widget_hovered = false;
}

void uph_ui_dropdown(Uph_DropDownCallback callback, Naui_Vec2 position)
{
    uph_global_widget_data.current_dropdown_callback = callback;
    uph_global_widget_data.current_dropdown_position = position;
    uph_global_widget_data.dropdown_opened_this_frame = true;
}

void uph_ui_close_dropdown(void)
{
    uph_global_widget_data.current_dropdown_callback = NULL;
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
    return hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT);
}

bool uph_ui_text_toggle_button(const char *string, const Leaf_ID id, bool *enabled)
{
    const Leaf_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
    const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");
    const Leaf_Color pressed_bg_color = naui_theme_color("uph_ui_frame_pressed_bg_color");

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const bool hovered = uph_ui_widget_hovered(id);

    Leaf_Color color = (*enabled) ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    
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
    {
        *enabled = !(*enabled);
        return true;
    }

    return false;
}

void uph_ui_menu(const char *label, const Leaf_ID id, Uph_UIMenuFlags flags, Uph_DropDownCallback dropdown)
{
    const bool hovered = uph_ui_widget_hovered(id);
    const bool this_dropdown_open = (uph_global_widget_data.current_dropdown_callback == dropdown);

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        if (this_dropdown_open)
            uph_ui_close_dropdown();
        else
        {
            Leaf_BoundingBox parent_bbox = leaf_get_bounding_box(id);
            uph_ui_dropdown(dropdown, naui_vec2(parent_bbox.x, parent_bbox.y + parent_bbox.height));
        }
    }
    else if (hovered && uph_global_widget_data.current_dropdown_callback && !this_dropdown_open)
    {
        Leaf_BoundingBox parent_bbox = leaf_get_bounding_box(id);
        uph_ui_dropdown(dropdown, naui_vec2(parent_bbox.x, parent_bbox.y + parent_bbox.height));
    }

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    leaf({
        .id = id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = (hovered || this_dropdown_open) ? naui_theme_color("uph_ui_frame_hovered_bg_color") : LEAF_COLOR_TRANSPARENT,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL)
    })
    {
        leaf_text(label, {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = naui_theme_color("uph_ui_text_color")
        });
    }
}

static void uph_ui_textfield_cursor(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldData* data = (Uph_TextfieldData*)user_data;

    const Naui_Vec2 text_size = naui_measure_text(data->value->data, data->cursor, NAUI_DPI(naui_theme_float("uph_ui_font_size")), 0);

    naui_fill_rect(
        (Naui_Vec2) {
            box.x + text_size.x,
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

void uph_ui_textfield(Naui_String* value, const Leaf_ID id, const char *placeholder)
{
    static Uph_TextfieldData data;

    const bool hovered = uph_ui_widget_hovered(id);

    bool active = data.id.value == id.value;
    if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        if (hovered)
        {
            data.id.value = id.value;
            data.value = value;
            data.cursor = NAUI_MAX((int64_t)value->length, 0);

            active = true;
        }
        else if (active)
        {
            data.id.value = 0;
            data.value = NULL;

            active = false;
        }
    }

    if (active)
    {
        uint32_t codepoint = naui_app_char_pressed();
        switch (codepoint)
        {
        case 0: break;
        // Backspace
        case '\b':
            if (data.cursor > 0)
                naui_string_remove(data.value, --data.cursor, 1);
            break;
        case '\t':
            naui_string_append_char_at(data.value, ' ', data.cursor++);
            break;
        // Delete
        case 0x7F:
            naui_string_remove(data.value, data.cursor, 1);
            break;
        // ESC
        case '\x1b':
            data.id.value = 0;
            data.value = NULL;
            break;
        default:
            naui_string_append_char_at(data.value, (char)codepoint, data.cursor++);
            break;
        }
    }

    if (naui_key_pressed(NAUI_KEY_LEFT))
        data.cursor = NAUI_MAX(data.cursor--, 0);
    else if (naui_key_pressed(NAUI_KEY_RIGHT))
        data.cursor = NAUI_MIN(data.cursor++, data.value->length);

    const float font_size = naui_theme_float("uph_ui_font_size");
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_GROW,
            .height = LEAF_SIZE_FIXED(NAUI_DPI(font_size))
        },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = naui_theme_color("uph_ui_frame_bg_color")
    })
    {
        if (value->length)
        {
            leaf_text(value->data, {
                .color = naui_theme_color("uph_ui_text_color"),
                .font_size = NAUI_DPI(font_size)
            });
        }
        else
        {
            leaf_text(placeholder, {
                .color = naui_theme_color("uph_ui_text_disabled_color"),
                .font_size = NAUI_DPI(font_size)
            });
        }
        leaf({
            .custom_draw = active ? uph_ui_textfield_cursor : NULL,
            .custom_draw_data = LEAF_DATA_SLICE(data),
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
}