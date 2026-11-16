static void _uph_ui_dropdown_arrow_draw(Leaf_BoundingBox box, void *user_data)
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
    Uph_GlobalWidgetState *data = &uph_global_widget_state;
    Uph_UIMenuNode *menu = _uph_ui_menu_root(data, id);
    bool result = false;

    const bool was_open = _uph_ui_menu_is_open(data, menu);
    const bool hovered = uph_ui_widget_hovered(id) || (was_open && leaf_hovered(id));

    if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            if (was_open)
                _uph_ui_close_menu(data);
            else
                _uph_ui_open_menu(data, menu, false);
        }
    }

    const bool open = _uph_ui_menu_is_open(data, menu);
    const uint32_t shown_index = (*current_index < item_count) ? *current_index : 0;
    const char *shown_text = item_count > 0 ? items[shown_index] : "";

    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const float rounding = UPH_UI_FRAME_ROUNDING;
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Leaf_Color text_color = uph_state.colors.ui_text;
    Leaf_Color arrow_color = text_color;

    Leaf_Color background = uph_state.colors.ui_frame_bg;
    if (open)
        background = uph_state.colors.ui_frame_pressed_bg;
    else if (hovered)
        background = uph_state.colors.ui_frame_hovered_bg;

    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_GROW, .height = UPH_UI_SIZE_EXACT(_uph_ui_frame_height()) },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .border = _uph_ui_frame_border(),
        .color = background,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(rounding), NAUI_CORNER_ALL),
        .child_gap = NAUI_DPI(6),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
            .clip_children = true
        })
        {
            leaf_text(shown_text, {
                .color = text_color,
                .font_size = LEAF_SIZE_FIXED(font_size)
            });
        }

        leaf({
            .custom_draw = _uph_ui_dropdown_arrow_draw,
            .custom_draw_data = LEAF_DATA_SLICE(arrow_color),
            .size = { .width = LEAF_SIZE_FIXED(font_size), .height = LEAF_SIZE_FIXED(font_size) }
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

bool uph_ui_snap_dropdown(Uph_SnapResolution *snap, const Leaf_ID id)
{
    static const char *const labels[] = { "Line", "Beat", "1/2", "1/4", "1/8", "1/16" };
    static const Uph_SnapResolution values[] = { UPH_SNAP_LINE, UPH_SNAP_BEAT, UPH_SNAP_HALF, UPH_SNAP_QUARTER, UPH_SNAP_EIGHTH, UPH_SNAP_SIXTEENTH };
    const uint32_t count = sizeof(values) / sizeof(values[0]);

    uint32_t index = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        if (values[i] == *snap)
            index = i;
    }

    if (!uph_ui_dropdown(labels, count, &index, id))
        return false;

    *snap = values[index];
    return true;
}
