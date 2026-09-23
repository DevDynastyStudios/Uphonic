NAUI_PANEL(uph_plugin_list)

void uph_plugin_list_on_attach(void)
{
    const Naui_PanelID panel_id = naui_current_panel();
    naui_panel_set_title(panel_id, "Plugin List");
    naui_panel_enable_flags(panel_id, NAUI_PANEL_FLAG_NO_DOCK | NAUI_PANEL_FLAG_NO_UNDOCK);
}

void uph_plugin_list_on_detach(void)
{
    
}

void uph_plugin_list_on_open(void)
{
    
}

void uph_plugin_list_on_close(void)
{
    
}

void uph_plugin_list_on_update(void)
{
    uint32_t instrument_index = 0;
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.settings.plugin.plugin_paths); i++)
    {
        Naui_Path parent_path = uph_state.settings.plugin.plugin_paths[i];
        Naui_DirIterator it = naui_dir_iterator_open(parent_path, "", NAUI_EXTENSIONS(".clap", ".vst3"), true);
        while (naui_dir_iterator_valid(&it))
        {
            if (uph_ui_text_button(it.entry.path.data, leaf_id_indexed("uph_plugin_list_item", instrument_index++))) 
            {
                if (uph_state.shared.plugin_list_for_track_instrument)
                {
                    uph_state.shared.current_plugin_list_track->instrument = uph_load_plugin_effect(it.entry.path);
                    uph_state.shared.current_plugin_list_track->type = UPH_RESOURCE_PATTERN;
                }
                else
                {
                    // For effects
                }
                naui_close_panel(naui_current_panel());
            }
            naui_dir_iterator_next(&it);
        }
        naui_dir_iterator_close(&it);
    }
}