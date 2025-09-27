#include "panel_manager.h"

std::vector<UphPanel>& panels() {
    static std::vector<UphPanel> instance; // constructed on first use
    return instance;
}

void uph_panel_register(const char* title, ImGuiItemFlags window_flags, UphPanelRenderCallback render_callback, bool hidden_on_boot)
{
    UphPanel panel { title, !hidden_on_boot, window_flags, render_callback };
    panels().push_back(panel);
}

void uph_panel_render_all()
{
    for(UphPanel& panel : panels())
    {
        if(!panel.is_visible)
            continue;

        if(ImGui::Begin(panel.title, &panel.is_visible, panel.flags))
            if(panel.render_callback)
                panel.render_callback(&panel);

        ImGui::End();
    }
}

UphPanel* uph_panel_get(int id)
{
    std::vector<UphPanel>& vec = panels();
    if (id < 0 || id >= (int) vec.size()) 
        return nullptr;

    return &vec[id];
}