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

static bool uph_automation_list_param_visible(const Uph_PluginParam *param, const Uph_Plugin *plugin)
{
    return !param->used && uph_automation_list_matches_filter(param, plugin);
}

static bool uph_automation_list_plugin_has_visible_params(const Uph_Plugin *plugin)
{
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(plugin->params); i++)
    {
        if (uph_automation_list_param_visible(&plugin->params[i], plugin))
            return true;
    }
    return false;
}

static void uph_automation_list_render_plugin_params(Uph_Plugin *plugin, const Uph_Track *parent_track)
{
    if (!uph_automation_list_plugin_has_visible_params(plugin))
        return;

    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Color text_color = naui_theme_color("uph_ui_text_color");

    leaf_text(plugin->name.data, {.font_size = font_size, .color = text_color});

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(plugin->params); i++)
    {
        Uph_PluginParam *param = &plugin->params[i];
        if (!uph_automation_list_param_visible(param, plugin))
            continue;

        if (uph_ui_text_button(param->name.data, leaf_id_indexed("uph_song_timeline_options_automate_param", i)))
        {
            Uph_ActionTrackAutomationCreate lane = {
                .parent = uph_action_track_ref(parent_track),
                .name = param->name,
                .effect_index = -1,
                .param_id = param->id
            };
            naui_action_execute_stack(UPH_ACTION_TRACK_AUTOMATION_CREATE, lane);
            naui_close_panel(naui_current_panel());
        }
    }
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

    uph_automation_list_render_plugin_params(&track->instrument, track);
    for (uint32_t i = 0; i < naui_list_len(track->effects); i++)
        uph_automation_list_render_plugin_params(&track->effects[i].plugin, track);
    uph_ui_end_scroll_container(&container);
}