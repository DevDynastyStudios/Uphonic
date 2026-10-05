NAUI_PANEL(uph_automation_list)

typedef struct
{
    Naui_String filter;
}
Uph_AutomationListData;

static Uph_AutomationListData uph_automation_list_data = { 0 };

void uph_automation_list_on_attach(void)
{
    const Naui_PanelID panel_id = naui_current_panel();
    naui_panel_set_title(panel_id, NAUI_TR("automation.menu.title"));
    naui_panel_enable_flags(panel_id, NAUI_PANEL_FLAG_NO_DOCK | NAUI_PANEL_FLAG_NO_UNDOCK);
}

void uph_automation_list_on_detach(void)
{
    
}

void uph_automation_list_on_open(void)
{

}

void uph_automation_list_on_close(void)
{
    uph_automation_list_data.filter = (Naui_String){0};
}

static bool uph_automation_list_matches_filter(const Uph_PluginParam *param, const Uph_Plugin *plugin)
{
    if (uph_automation_list_data.filter.length == 0)
        return true;

    return naui_string_contains(param->name, uph_automation_list_data.filter, false)
        || naui_string_contains(plugin->name, uph_automation_list_data.filter, false);
}

void uph_automation_list_on_update(void)
{
    Uph_Track *track = uph_state.shared.current_automation_list_track;
    uph_ui_textfield(&uph_automation_list_data.filter, leaf_id("uph_automation_list_filter"), UPH_UI_TEXTFIELD_ALWAYS_ACTIVE, "Search");

    static float scroll = 0.0f;
    Uph_UIScrollContainer container = uph_ui_begin_scroll_container(
        UPH_UI_SCROLL_DIRECTION_VERTICAL,
        &scroll,
        leaf_id("uph_automation_list_scrollbar")
    );
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->instrument.params); i++)
    {
        Uph_PluginParam *param = &track->instrument.params[i];
        if (!uph_automation_list_matches_filter(param, &track->instrument))
            continue;
        if (!track->instrument.params[i].used && uph_ui_text_button(track->instrument.params[i].name.data, leaf_id_indexed("uph_song_timeline_options_automate_param", i)))
        {
            Uph_ActionTrackAutomationCreate lane = {
                .parent = uph_action_track_ref(track),
                .name = track->instrument.params[i].name,
                .effect_index = -1,
                .param_id = track->instrument.params[i].id
            };
            naui_action_execute_stack(UPH_ACTION_TRACK_AUTOMATION_CREATE, lane);
            param->used = true;
            naui_close_panel(naui_current_panel());
        }
    }
    uph_ui_end_scroll_container(&container);
}