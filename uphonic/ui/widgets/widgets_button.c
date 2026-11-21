static bool uph__ui_text_button(const char *string, const Leaf_ID id, Naui_Color bg_color, Leaf_SizeAxis width, bool tinted)
{
    const Leaf_Color hovered_bg_color = tinted ? uph_state.colors.ui_tinted_frame_hovered_bg : uph_state.colors.ui_frame_hovered_bg;
    const Leaf_Color pressed_bg_color = tinted ? uph_state.colors.ui_tinted_frame_pressed_bg : uph_state.colors.ui_frame_pressed_bg;
    const Leaf_Color border_color = tinted ? uph_state.colors.ui_tinted_frame_border : uph_state.colors.ui_frame_border;

    const Leaf_Color text_color = uph_state.colors.ui_text;

    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const float rounding = UPH_UI_FRAME_ROUNDING;

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
    
    const float frame_height = uph__ui_frame_height();
    leaf({
        .id = id,
        .size = { .width = width, .height = UPH_UI_SIZE_EXACT(frame_height) },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .border = bg_color.a > 0 ? uph__ui_border(border_color) : (Leaf_Border){0},
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        if (string)
        {
            leaf_text(string, {
                .color = text_color,
                .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
            });
        }
        else
        {
            Naui_Color spinner_color = text_color;
            leaf({
                .custom_draw = uph_ui_spinner_custom_draw,
                .custom_draw_data = LEAF_DATA_SLICE(spinner_color),
                .size = { .width = UPH_UI_SIZE_EXACT(frame_height), .height = UPH_UI_SIZE_EXACT(frame_height) }
            });
        }
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

bool uph_ui_text_button_ex(const char *string, const Leaf_ID id, Naui_Color bg_color, Naui_CornerFlags corners)
{
    return uph__ui_text_button(string, id, bg_color, LEAF_SIZE_FIT_MIN(uph__ui_frame_height()), false);
}

bool uph_ui_tinted_text_button_full(const char *string, const Leaf_ID id)
{
    return uph__ui_text_button(string, id, uph_state.colors.ui_tinted_frame_bg, LEAF_SIZE_FULL, true);
}

void uph_ui_tinted_spinner_button_full(const Leaf_ID id)
{
    uph__ui_text_button(NULL, id, uph_state.colors.ui_tinted_frame_bg, LEAF_SIZE_FULL, true);
}

bool uph_ui_text_button(const char *string, const Leaf_ID id)
{
    return uph_ui_text_button_ex(string, id, uph_state.colors.ui_frame_bg, NAUI_CORNER_ALL);
}

static Naui_Vec2 uph__ui_icon_frame(Naui_Vec2 size)
{
    const float padding = NAUI_DPI(UPH_UI_FRAME_PADDING.y);
    return (Naui_Vec2){ NAUI_MAX(uph__ui_frame_height(), size.x + padding), NAUI_MAX(uph__ui_frame_height(), size.y + padding) };
}

static bool uph__ui_image_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Vec2 frame, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners)
{
    const bool overlay = bg_color.a == 0;
    const Leaf_Color hovered_bg_color = overlay ? (Leaf_Color) { 255, 255, 255, 36 } : uph_state.colors.ui_frame_hovered_bg;
    const Leaf_Color pressed_bg_color = overlay ? (Leaf_Color) { 255, 255, 255, 64 } : uph_state.colors.ui_frame_pressed_bg;

    const float rounding = UPH_UI_FRAME_ROUNDING;

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
    
    leaf({
        .id = id,
        .size = { .width = UPH_UI_SIZE_EXACT(frame.x), .height = UPH_UI_SIZE_EXACT(frame.y) },
        .border = overlay ? (Leaf_Border){0} : uph__ui_segment_border(uph_state.colors.ui_frame_border, corners),
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), corners)
    })
    {
        leaf({
            .image = (void*)image,
            .size = { .width = LEAF_SIZE_FIXED(size.x), .height = LEAF_SIZE_FIXED(size.y) },
            .color = tint
        });
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

bool uph_ui_image_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners)
{
    return uph__ui_image_button(image, id, size, uph__ui_icon_frame(size), tint, bg_color, corners);
}

static Naui_Vec2 uph__ui_toolbox_frame(void)
{
    const float size = NAUI_DPI(UPH_TOOLBOX_ICON_SIZE) + NAUI_DPI(UPH_TOOLBOX_BUTTON_PADDING) * 2.0f;
    return (Naui_Vec2){ size, size };
}

bool uph_ui_toolbox_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Color tint, Naui_CornerFlags corners)
{
    const float icon = NAUI_DPI(UPH_TOOLBOX_ICON_SIZE);
    return uph__ui_image_button(image, id, (Naui_Vec2){ icon, icon }, uph__ui_toolbox_frame(), tint, uph_state.colors.ui_frame_bg, corners);
}

bool uph_ui_image_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint)
{
    return uph_ui_image_button_ex(image, id, size, tint, uph_state.colors.ui_frame_bg, NAUI_CORNER_ALL);
}

bool uph_ui_text_toggle_button(const char *string, const Leaf_ID id, bool enabled)
{
    const Leaf_Color bg_color = uph_state.colors.ui_frame_bg;
    const Leaf_Color hovered_bg_color = uph_state.colors.ui_frame_hovered_bg;
    const Leaf_Color pressed_bg_color = uph_state.colors.ui_frame_pressed_bg;

    const Leaf_Color text_color = uph_state.colors.ui_text;

    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const float rounding = UPH_UI_FRAME_ROUNDING;

    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Leaf_Color color = enabled ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    
    const float frame_height = uph__ui_frame_height();
    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_FIT_MIN(frame_height), .height = UPH_UI_SIZE_EXACT(frame_height) },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .border = uph__ui_frame_border(),
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        leaf_text(string, {
            .color = text_color,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
        });
    }

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return true;

    return false;
}

static bool uph__ui_image_toggle_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Vec2 frame, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners, bool enabled, bool tinted, const Naui_Color *enabled_bg_color)
{
    const Leaf_Color hovered_bg_color = tinted ? uph_state.colors.ui_tinted_frame_hovered_bg : uph_state.colors.ui_frame_hovered_bg;
    const Leaf_Color theme_pressed_bg_color = tinted ? uph_state.colors.ui_tinted_frame_pressed_bg : uph_state.colors.ui_frame_pressed_bg;
    const Leaf_Color pressed_bg_color = enabled_bg_color ? uph_color_mix(theme_pressed_bg_color, *enabled_bg_color, 0.5f) : theme_pressed_bg_color;
    const Leaf_Color border_color = tinted ? uph_state.colors.ui_tinted_frame_border : uph_state.colors.ui_frame_border;

    const float rounding = UPH_UI_FRAME_ROUNDING;

    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Leaf_Color color = enabled ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    if (enabled_bg_color && hovered)
        color = enabled ? uph_color_mix(pressed_bg_color, hovered_bg_color, 0.25f) : uph_color_mix(hovered_bg_color, *enabled_bg_color, 0.15f);
    
    leaf({
        .id = id,
        .size = { .width = UPH_UI_SIZE_EXACT(frame.x), .height = UPH_UI_SIZE_EXACT(frame.y) },
        .border = (bg_color.a > 0 || enabled) ? uph__ui_segment_border(border_color, corners) : (Leaf_Border){0},
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), corners)
    })
    {
        leaf({
            .image = (void*)image,
            .size = { .width = LEAF_SIZE_FIXED(size.x), .height = LEAF_SIZE_FIXED(size.y) },
            .color = tint
        });
    }

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return true;

    return false;
}

bool uph_ui_image_toggle_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, bool enabled)
{
    return uph__ui_image_toggle_button(image, id, size, uph__ui_icon_frame(size), tint, uph_state.colors.ui_tinted_frame_bg, NAUI_CORNER_ALL, enabled, true, NULL);
}

bool uph_ui_toolbox_toggle_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Color tint, Naui_CornerFlags corners, bool enabled)
{
    const float icon = NAUI_DPI(UPH_TOOLBOX_ICON_SIZE);
    return uph__ui_image_toggle_button(image, id, (Naui_Vec2){ icon, icon }, uph__ui_toolbox_frame(), tint, uph_state.colors.ui_frame_bg, corners, enabled, false, NULL);
}

bool uph_ui_checkbox(bool *value, const Leaf_ID id)
{
    const bool hovered = uph_ui_widget_hovered(id);

    const Leaf_Color bg_color = uph_state.colors.ui_frame_bg;
    const Leaf_Color hovered_bg_color = uph_state.colors.ui_frame_hovered_bg;
    const Leaf_Color check_color = uph_state.colors.ui_slider_fill;

    const float size = uph__ui_frame_height() * 0.7f;
    const float rounding = UPH_UI_FRAME_ROUNDING;

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    leaf({
        .id = id,
        .size = { .width = UPH_UI_SIZE_EXACT(size), .height = UPH_UI_SIZE_EXACT(size) },
        .border = uph__ui_frame_border(),
        .color = hovered ? hovered_bg_color : bg_color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL)
    })
    {
        if (*value)
        {
            leaf({
                .size = { .width = LEAF_SIZE_PERCENT(0.6f), .height = LEAF_SIZE_PERCENT(0.6f) },
                .color = check_color,
                .floating = {
                    .parent_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
                    .self_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
                },
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding) * 0.5f, NAUI_CORNER_ALL),
                .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
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
