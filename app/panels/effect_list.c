NAUI_PANEL(uph_effect_list)

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

    const Naui_Color text_color = naui_theme_color("uph_ui_text_color");
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));

    if (uph_ui_text_button("Add Effect", leaf_id("uph_effect_list_add_effect")))
    {
        uph_state.shared.plugin_list_for_track_instrument = false;
        uph_state.shared.current_plugin_list_track = track;
        naui_open_panel(uph_state.panels.plugin_list);
    }

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->effects); i++)
    {
        Uph_Plugin *effect = &track->effects[i];
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32))}
        })
        {
            leaf_text(effect->name.data, { .font_size = font_size, .color = text_color });
        }
    }
}