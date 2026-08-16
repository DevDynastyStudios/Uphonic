static uint64_t widget_counter;

Leaf_ID naui_widgets_alloc_id(void)
{
    return leaf_id_indexed("__naui_widget", widget_counter++);
}

void naui_widgets_new_frame(void)
{
    widget_counter = 0;
}

bool naui_button(const char *text)
{
    return false;
}

bool naui_toggle_button(const char *text, float font_size, Naui_Color text_color, bool *toggled)
{
    Leaf_ID id = naui_widgets_alloc_id();
    bool hovered = naui_panel_hovered(naui_current_panel()) && leaf_hovered(id);

    bool clicked = hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT);
    if (clicked)
        *toggled = !(*toggled);

    Naui_Vec2 padding = naui_theme_vec2("naui_widget_frame_padding");
    Naui_Color color;
    if (*toggled)
        color = naui_theme_color("naui_widget_frame_toggled_bg_color");
    else if (hovered)
        color = naui_theme_color("naui_widget_frame_hovered_bg_color");
    else color = naui_theme_color("naui_widget_frame_bg_color");

    leaf({
        .id = id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = color,
        .border = {
            .width = 1.0f,
            .sides = LEAF_SIDE_ALL,
            .color = naui_theme_color("naui_widget_frame_border")
        },
        .rounding = {
            .value = NAUI_DPI(naui_theme_float("naui_widget_frame_rounding")),
            .corners = LEAF_CORNER_ALL
        }
    })
    {
        leaf_text(text, {
            .font_size = NAUI_DPI(font_size),
            .color = text_color
        });
    }

    return clicked;
}

bool naui_toggle_image_button(Naui_Image *image, Naui_Vec2 size, Naui_Color tint, bool *toggled)
{
    Leaf_ID id = naui_widgets_alloc_id();
    bool hovered = naui_panel_hovered(naui_current_panel()) && leaf_hovered(id);

    bool clicked = hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT);
    if (clicked)
        *toggled = !(*toggled);

    Naui_Vec2 padding = naui_theme_vec2("naui_widget_frame_padding");
    Naui_Color color;
    if (*toggled)
        color = naui_theme_color("naui_widget_frame_toggled_bg_color");
    else if (hovered)
        color = naui_theme_color("naui_widget_frame_hovered_bg_color");
    else color = naui_theme_color("naui_widget_frame_bg_color");

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(size.x)), LEAF_SIZE_FIXED(NAUI_DPI(size.y))},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = color,
        .border = {
            .width = 1.0f,
            .sides = LEAF_SIDE_ALL,
            .color = naui_theme_color("naui_widget_frame_border")
        },
        .rounding = {
            .value = NAUI_DPI(naui_theme_float("naui_widget_frame_rounding")),
            .corners = LEAF_CORNER_ALL
        }
    })
    {
        leaf({
            .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_FULL},
            .image = image,
            .color = tint,
            .aspect_ratio = NAUI_IMAGE_ASPECT_RATIO(image)
        });
    }

    return clicked;
}

bool naui_image_button(Naui_Image *image, Naui_Vec2 size, Naui_Color tint)
{
    Leaf_ID id = naui_widgets_alloc_id();
    bool hovered = (!naui_current_panel() || naui_panel_hovered(naui_current_panel())) && leaf_hovered(id);

    Naui_Vec2 padding = naui_theme_vec2("naui_widget_frame_padding");
    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(size.x)), LEAF_SIZE_FIXED(NAUI_DPI(size.y))},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = hovered ?
            naui_theme_color("naui_widget_frame_hovered_bg_color") :
            naui_theme_color("naui_widget_frame_bg_color"),
        .border = {
            .width = 1.0f,
            .sides = LEAF_SIDE_ALL,
            .color = naui_theme_color("naui_widget_frame_border")
        },
        .rounding = {
            .value = NAUI_DPI(naui_theme_float("naui_widget_frame_rounding")),
            .corners = LEAF_CORNER_ALL
        }
    })
    {
        leaf({
            .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_FULL},
            .image = image,
            .color = tint,
            .aspect_ratio = NAUI_IMAGE_ASPECT_RATIO(image)
        });
    }

    return hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT);
}