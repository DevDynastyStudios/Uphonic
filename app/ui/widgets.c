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

static Leaf_ID uph_ui_alloc_widget_id(void)
{
    return leaf_id_indexed("naui_widget", uph_global_widget_data.id_counter++);
}

static bool uph_ui_widget_hovered(Leaf_ID id)
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
        Leaf_ID dropdown_id = uph_ui_alloc_widget_id();

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

bool uph_ui_text_button(const char *string)
{
    const Leaf_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
    const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");
    const Leaf_Color pressed_bg_color = naui_theme_color("uph_ui_frame_pressed_bg_color");

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const float rounding = naui_theme_float("uph_ui_frame_rounding");

    const Leaf_ID id = uph_ui_alloc_widget_id();
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
            .font_size = NAUI_DPI(13.5f),
            .color = text_color
        });
    }
    return hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT);
}

void uph_ui_menu(const char *label, Uph_UIMenuFlags flags, Uph_DropDownCallback dropdown)
{
    const Leaf_ID id = uph_ui_alloc_widget_id();
    const bool hovered = uph_ui_widget_hovered(id);
    const bool this_dropdown_open = (uph_global_widget_data.current_dropdown_callback == dropdown);

    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");

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

    leaf({
        .id = id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(6.0f), NAUI_DPI(6.0f)),
        .color = (hovered || this_dropdown_open) ? leaf_rgb(255, 0, 0) : LEAF_COLOR_TRANSPARENT
    })
    {
        leaf_text(label, {
            .font_size = NAUI_DPI(13.5f),
            .color = text_color
        });
    }
}