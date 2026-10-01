NAUI_PANEL(uph_effect_list)

static void uph_effect_list_swap(Uph_Track *track, uint32_t a, uint32_t b)
{
    Uph_EffectPlugin temp = track->effects[a];
    track->effects[a] = track->effects[b];
    track->effects[b] = temp;

    uint64_t subtrack_count = naui_list_len(track->subtracks);
    for (uint64_t s = 0; s < subtrack_count; s++)
    {
        Uph_Track *sub = &track->subtracks[s];
        if (sub->type != UPH_RESOURCE_AUTOMATION)
            continue;

        if (sub->automation_target_effect_index == (int32_t)a)
            sub->automation_target_effect_index = (int32_t)b;
        else if (sub->automation_target_effect_index == (int32_t)b)
            sub->automation_target_effect_index = (int32_t)a;
    }
}

static void uph_effect_list_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "Effect List");
}

static void uph_effect_list_on_detach(void)
{
}

static void uph_effect_list_on_open(void)
{
}

static void uph_effect_list_on_close(void)
{
}

static void uph_effect_list_on_update(void)
{
    Uph_Track *track = uph_state.shared.selected_mixer_track;
    if (!track)
        return;

    Uph_UIMenuID context_menu = uph_ui_context_menu();

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Naui_Color text_color = naui_theme_color("uph_ui_text_color");
    const Naui_Color text_disabled_color = naui_theme_color("uph_ui_text_disabled_color");
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));

    if (uph_ui_text_button("Add Effect", leaf_id("uph_effect_list_add_effect")))
    {
        uph_state.shared.plugin_list_for_track_instrument = false;
        uph_state.shared.current_plugin_list_track = track;
        naui_open_panel(uph_state.panels.plugin_list);
    }

    const uint32_t effect_count = (uint32_t)naui_list_len(track->effects);
    int32_t move_from = -1;
    int32_t move_to = -1;

    static uint32_t current_effect_index;

    for (uint32_t i = 0; i < effect_count; i++)
    {
        Uph_EffectPlugin *effect = &track->effects[i];
        Uph_Plugin *plugin = &effect->plugin;
        
        Leaf_ID id = leaf_id_indexed("uph_effect_list_item", i);
        
        if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_ui_widget_hovered(id))
        {
            current_effect_index = i;
            uph_ui_open_context_menu(context_menu);
        }

        leaf({
            .id = id,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32))},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .direction = LEAF_DIRECTION_HORIZONTAL
        })
        {
            leaf({
                .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .child_gap = NAUI_DPI(6)
            })
            {
                uph_ui_checkbox(&effect->enabled, leaf_id_indexed("uph_effect_list_enable", i));
                leaf_text(plugin->name.data, { .font_size = font_size, .color = effect->enabled ? text_color : text_disabled_color });
            }
            leaf({
                .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .child_gap = NAUI_DPI(2)
            })
            {
                if (i > 0 && uph_ui_text_button("^", leaf_id_indexed("uph_effect_list_move_up", i)))
                {
                    move_from = (int32_t)i;
                    move_to = (int32_t)i - 1;
                }
                if (i + 1 < effect_count && uph_ui_text_button("v", leaf_id_indexed("uph_effect_list_move_down", i)))
                {
                    move_from = (int32_t)i;
                    move_to = (int32_t)i + 1;
                }
            }
        }
    }

    if (move_from >= 0 && move_to >= 0)
        uph_effect_list_swap(track, (uint32_t)move_from, (uint32_t)move_to);

    if (uph_ui_menu_item(context_menu, NAUI_TR("effect_list.remove"), leaf_id("uph_effect_list_remove")))
    {
        Uph_EffectPlugin *effect = &track->effects[current_effect_index];
        uph_audio_engine_lock();
        naui_list_remove(track->effects, current_effect_index);
        uph_audio_engine_unlock();
        uph_unload_plugin(&effect->plugin);
    }
}