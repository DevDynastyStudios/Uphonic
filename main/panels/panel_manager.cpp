#include "panel_manager.h"

std::vector<UphPanel>& panels() {
    static std::vector<UphPanel> instance; // constructed on first use
    return instance;
}

void uph_panel_register(const char* title, ImGuiItemFlags window_flags, ImGuiDockNodeFlags dock_flags, UphPanelRenderCallback render_callback)
{
    UphPanel panel { title, false, window_flags, dock_flags, render_callback };
    panels().push_back(panel);
}

void uph_panel_render_all()
{
    for(UphPanel& panel : panels())
    {
        if(!panel.is_visible)
            continue;

        if(panel.window_flags & ImGuiWindowFlags_MenuBar) 
        {
            if(ImGui::BeginMainMenuBar())
                if(panel.render_callback)
                    panel.render_callback(&panel);
                
            ImGui::EndMainMenuBar();
        } else
        {
            if(ImGui::Begin(panel.title, &panel.is_visible, panel.window_flags))
                if(panel.render_callback)
                    panel.render_callback(&panel);
    
            ImGui::End();
        }
    }
}
