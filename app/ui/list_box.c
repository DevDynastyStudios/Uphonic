bool uph_ui_list_box(Naui_String *name, Leaf_CustomDrawFn content_draw, Leaf_DataSlice content_draw_data, Leaf_ID id, bool hovered, bool selected, bool renaming, const char *placeholder_name)
{
    leaf({
        .id = id,
        .custom_draw = content_draw,
        .custom_draw_data = content_draw_data,
        .size = {
            .width = LEAF_SIZE_FIXED(NAUI_DPI(150)),
            .height = LEAF_SIZE_DERIVED
        },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(2)),
        .aspect_ratio = 2.2f,
        .border = {
            .width = NAUI_DPI(selected ? 3.0f : 1.0f),
            .sides = LEAF_SIDE_ALL,
            .color = {leaf_rgb(145, 111, 205)}
        },
        .color = {leaf_rgb(108, 83, 154)},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), LEAF_CORNER_ALL),
        .clip_children = true
    }) {
        if (renaming)
        {
            if (uph_ui_textfield(name, id, UPH_UI_TEXTFIELD_ALWAYS_ACTIVE, placeholder_name))
            {
                if (name->length == 0)
                    *name = naui_string_from_cstr(placeholder_name);
                uph_state.shared.selected_resource.renaming = false;
            }
        }
        else
        {
            leaf_text(name->data, {
                .color = {LEAF_COLOR_WHITE},
                .font_size = {NAUI_DPI(13)}
            });
        }
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

bool uph_ui_list_plus_box(Leaf_ID id)
{
    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_FIXED(NAUI_DPI(150)),
            .height = LEAF_SIZE_DERIVED
        },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(2)),
        .aspect_ratio = 2.2f,
        .border = {
            .width = NAUI_DPI(1),
            .sides = LEAF_SIDE_ALL,
            .color = {naui_theme_color("uph_ui_frame_border")}
        },
        .color = {naui_theme_color("uph_ui_frame_bg_color")},
        .child_alignment = {
            LEAF_ALIGN_X_CENTER,
            LEAF_ALIGN_Y_CENTER
        },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), LEAF_CORNER_ALL)
    }) {
        leaf({
            .image = naui_asset_image("uph_icon_plus"),
            .size = {
                .width = LEAF_SIZE_PERCENT(0.1f),
                .height = LEAF_SIZE_DERIVED
            },
            .color = naui_theme_color("uph_ui_frame_border"),
            .aspect_ratio = 1.0f
        });
    }

	bool hovered = uph_ui_widget_hovered(id);
	if (hovered)
		naui_set_cursor(NAUI_CURSOR_HAND);

    return naui_mouse_pressed(NAUI_MOUSE_LEFT) && hovered;
}