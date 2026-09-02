NAUI_PANEL(uph_pattern_list)

static void uph_pattern_list_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, NAUI_TR("patterns.title"));
}

static void uph_pattern_list_on_detach(void)
{
    
}

static void uph_pattern_list_on_open(void)
{
    
}

static void uph_pattern_list_on_close(void)
{
    
}

static void uph_pattern_list_custom_draw(Leaf_BoundingBox box, void **user_data)
{

}

static void uph_pattern_list_on_update(void)
{
    leaf({
        .size = {
            .width = LEAF_SIZE_FULL,
            .height = LEAF_SIZE_FULL
        },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(6)),
        .child_gap = NAUI_DPI(6),
        .child_cross_gap = NAUI_DPI(6),
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .wrap_children = true
    }) {
        uint32_t pattern_count = (uint32_t)naui_list_len(uph_state.project.midi_patterns);
        Uph_UIMenuID context_menu = uph_ui_context_menu();

        for (uint32_t i = 0; i < pattern_count; i++) {
            Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[i];

            const Leaf_ID id = leaf_id_indexed("uph_pattern_list", i);
            if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_ui_widget_hovered(id))
            {
                uph_state.shared.selected_resource.index = i;
                uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
                uph_ui_open_context_menu(context_menu);
            }

            if (uph_ui_list_box(
                pattern->name.data,
                (Leaf_CustomDrawFn)uph_pattern_list_custom_draw,
                LEAF_DATA_SLICE(pattern),
                id,
                uph_state.shared.selected_resource.index == i
            ))
            {
                uph_state.shared.selected_resource.index = i;
                uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
            }
        }

        if (uph_ui_menu_item(context_menu, "Remove", leaf_id("uph_pattern_remove")))
        {
            uph_resources_remove_pattern(uph_state.shared.selected_resource.index);
            if (uph_state.shared.selected_resource.index > 0 && uph_state.shared.selected_resource.index == naui_list_len(uph_state.project.midi_patterns))
                uph_state.shared.selected_resource.index--;
            else if (naui_list_len(uph_state.project.midi_patterns) == 0)
                uph_state.shared.selected_resource.type = UPH_RESOURCE_NONE;
        }
        if (uph_ui_menu_item(context_menu, "Duplicate", leaf_id("uph_pattern_duplicate")))
            uph_resources_copy_pattern(uph_state.shared.selected_resource.index);

        const Leaf_ID plus_id = leaf_id("uph_pattern_list_plus");
        if (uph_ui_list_plus_box(plus_id))
        {
            uph_state.shared.selected_resource.index = naui_list_len(uph_state.project.midi_patterns);
            uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
            uph_resources_add_pattern();
        }
    }
}