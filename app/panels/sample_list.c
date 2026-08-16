NAUI_PANEL(sample_list)

static void sample_list_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "Samples");
}

static void sample_list_on_detach(void)
{
    
}

static void sample_list_on_update(void)
{
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.samples); i++)
    {
        Uph_Sample *resource = &uph_state.project.samples[i];
        leaf({
            0
        })
        {
            leaf_text(resource->name.data, {
                .font_size = 16.0f,
            });
        }
    }
}