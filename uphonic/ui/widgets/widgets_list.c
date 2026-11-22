typedef struct
{
    const char *key;
    const char *label;
}
Uph_UIKeyHint;

void uph_ui_highlighted_text(const char *string, Naui_String filter, Naui_Color color, Naui_Color highlight)
{
    const Leaf_SizeAxis font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE));
    const Naui_String text = naui_string_from_cstr(string);
    const size_t match = filter.length > 0 ? naui_string_find(text, filter, false) : (size_t)-1;

    if (match == (size_t)-1)
    {
        leaf_text(string, { .color = color, .font_size = font_size });
        return;
    }

    const size_t match_end = NAUI_MIN(match + filter.length, text.length);
    leaf({
        .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        if (match > 0)
            leaf_text(naui_string_format("%.*s", (int)match, text.data).data, { .color = color, .font_size = font_size });
        leaf_text(naui_string_format("%.*s", (int)(match_end - match), text.data + match).data, { .color = highlight, .font_size = font_size });
        if (match_end < text.length)
            leaf_text(text.data + match_end, { .color = color, .font_size = font_size });
    }
}

void uph_ui_count_badge(uint32_t count)
{
    leaf({
        .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(7), NAUI_DPI(1)),
        .color = uph_state.colors.ui_frame_bg,
        .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
    })
    {
        leaf_text(naui_string_format("%u", count).data, {
            .color = uph_state.colors.panel_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE - 2))
        });
    }
}

static void uph__ui_key_hint(const Uph_UIKeyHint *hint)
{
    const Leaf_SizeAxis font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE - 2));
    leaf({
        .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .child_gap = NAUI_DPI(5),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
            .padding = LEAF_PADDING_AXES(NAUI_DPI(5), NAUI_DPI(1)),
            .border = {
                .width = 1,
                .sides = LEAF_SIDE_ALL,
                .color = uph_state.colors.ui_frame_border
            },
            .color = uph_state.colors.ui_frame_bg,
            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL)
        }) leaf_text(hint->key, { .color = uph_state.colors.ui_text, .font_size = font_size });
        leaf_text(hint->label, { .color = uph_state.colors.panel_text, .font_size = font_size });
    }
}

void uph_ui_key_hint_bar(const Uph_UIKeyHint *hints, uint32_t count)
{
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2), NAUI_DPI(padding.y)),
        .border = {
            .width = 1,
            .sides = LEAF_SIDE_TOP,
            .color = uph_state.colors.ui_frame_border
        },
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .child_gap = NAUI_DPI(padding.x * 3),
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .clip_children = true
    })
    {
        for (uint32_t i = 0; i < count; i++)
            uph__ui_key_hint(&hints[i]);
    }
}

void uph_ui_empty_state(const Naui_Image *icon, const char *message)
{
    const int32_t icon_size = NAUI_DPI(32);
    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(UPH_UI_FRAME_PADDING.x * 4)),
        .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
        .child_gap = NAUI_DPI(10)
    })
    {
        leaf({
            .image = (void*)icon,
            .size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
            .color = uph_state.colors.ui_text_disabled
        });
        leaf_text(message, {
            .color = uph_state.colors.panel_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE)),
            .wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
        });
    }
}

void uph_ui_scroll_to_element(Leaf_ID scroll_id, Leaf_ID element_id, float *scroll, int32_t top_margin)
{
    const Leaf_BoundingBox view = leaf_get_bounding_box((Leaf_ID){ .value = scroll_id.value + 2 });
    const Leaf_BoundingBox element = leaf_get_bounding_box(element_id);
    if (view.height <= 0 || element.height <= 0)
        return;

    if (element.y - top_margin < view.y)
        *scroll -= (float)(view.y - (element.y - top_margin));
    else if (element.y + element.height > view.y + view.height)
        *scroll += (float)(element.y + element.height - (view.y + view.height));
    naui_app_stale_frame();
}
