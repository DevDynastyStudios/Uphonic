bool naui_tooltip_begin(Leaf_ID anchor_id)
{
    if (!naui_panel_hovered(naui_current_panel()))
        return false;

    bool hovered = leaf_hovered(anchor_id);

    if (hovered)
    {
        Leaf_ElementConfig config = {
            .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
            .floating = {
                .self_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_BOTTOM },
                .offset = { naui_mouse_x() + 4, naui_mouse_y() - 4 }
            },
            .padding = LEAF_PADDING_ALL(16.0f),
            .color = leaf_rgba(0, 0, 0, 150)
        };
        leaf_begin_element(config);
    }

    return hovered;
}

void naui_tooltip_end(bool state)
{
    if (state)
        leaf_end_element();
}
