NAUI_PANEL(uph_automation_list)

static void uph_automation_list_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, NAUI_TR("automations.title"));
}

static void uph_automation_list_on_detach(void)
{
    
}

static void uph_automation_list_on_open(void)
{
    
}

static void uph_automation_list_on_close(void)
{
    
}

static void uph_automation_list_custom_draw(Leaf_BoundingBox box, void **user_data)
{
    
}

static void uph_automation_list_on_update(void)
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
        uint32_t automation_count = (uint32_t)naui_list_len(uph_state.project.automations);
        Uph_UIMenuID context_menu = uph_ui_context_menu();

        for (uint32_t i = 0; i < automation_count; i++) {
            Uph_Automation *automation = &uph_state.project.automations[i];

            const Leaf_ID id = leaf_id_indexed("uph_automation_list_automation", i);
			bool hovered = uph_ui_widget_hovered(id);

			if (hovered)
				naui_set_cursor(NAUI_CURSOR_HAND);

            if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && hovered)
            {
                uph_state.shared.selected_resource.index = i;
                uph_state.shared.selected_resource.type = UPH_RESOURCE_AUTOMATION;
                uph_ui_open_context_menu(context_menu);
            }

            if (uph_ui_list_box(
                automation->name.data,
                (Leaf_CustomDrawFn)uph_automation_list_custom_draw,
                LEAF_DATA_SLICE(automation),
                id,
                hovered,
                uph_state.shared.selected_resource.index == i &&
                uph_state.shared.selected_resource.type == UPH_RESOURCE_AUTOMATION
            ))
            {
                uph_state.shared.selected_resource.index = i;
                uph_state.shared.selected_resource.type = UPH_RESOURCE_AUTOMATION;

                Uph_Automation *automation = &uph_state.project.automations[i];
                uph_state.shared.song_timeline_current_block_start_offset = 0;
                uph_state.shared.song_timeline_current_block_length = uph_calculate_automation_length(automation);
            }
        }

        if (uph_ui_menu_item(context_menu, "Remove", leaf_id("uph_automation_remove")))
        {
            uph_resources_remove_automation(uph_state.shared.selected_resource.index);
            if (uph_state.shared.selected_resource.index > 0 && uph_state.shared.selected_resource.index == naui_list_len(uph_state.project.automations))
                uph_state.shared.selected_resource.index--;
            else if (naui_list_len(uph_state.project.automations) == 0)
                uph_state.shared.selected_resource.type = UPH_RESOURCE_NONE;
        }
        if (uph_ui_menu_item(context_menu, "Duplicate", leaf_id("uph_automation_duplicate")))
            uph_resources_copy_automation(uph_state.shared.selected_resource.index);

        const Leaf_ID plus_id = leaf_id("uph_automation_list_plus");
        if (uph_ui_list_plus_box(plus_id))
        {
            uph_state.shared.selected_resource.index = naui_list_len(uph_state.project.automations);
            uph_state.shared.selected_resource.type = UPH_RESOURCE_AUTOMATION;
            uph_state.shared.song_timeline_current_block_start_offset = 0;
            uph_state.shared.song_timeline_current_block_length = 4.0;
            uph_resources_add_automation();
        }
    }
}