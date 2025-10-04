#include "panel_manager.h"

std::vector<UphPanel>& panels() {
    static std::vector<UphPanel> instance; // constructed on first use
    return instance;
}

void uph_panel_register(const char* title, UphPanelFlags panel_flags, UphPanelCallback render_callback, UphPanelCallback init_callback)
{
    UphPanel panel { title, "Uncategorized", false, panel_flags, ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, render_callback, init_callback };
    panels().push_back(panel);
}

void uph_panel_init_all()
{
	for(UphPanel& panel : panels())
	{
		if(panel.init_callback)
			panel.init_callback(&panel);
	}
}

void uph_panel_render_all()
{
    for(UphPanel& panel : panels())
    {
        if(!panel.is_visible)
            continue;

        if((int) panel.panel_flags & (int) UphPanelFlags_MenuBar)
        {
            if(ImGui::BeginMainMenuBar())
                if(panel.render_callback)
                    panel.render_callback(&panel);
                
            ImGui::EndMainMenuBar();

        } else if((int) panel.panel_flags & (int) UphPanelFlags_Modal)
		{
			ImGui::OpenPopup(panel.title);

			ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(400, 200));
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::GetBackgroundDrawList()->AddRectFilled(ImGui::GetMainViewport()->Pos, ImGui::GetMainViewport()->Pos + ImGui::GetMainViewport()->Size, IM_COL32(0, 0, 0, 128));

			if (ImGui::BeginPopupModal(panel.title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
			{
			    if (panel.render_callback)
			        panel.render_callback(&panel);
			
			    ImGui::EndPopup();
			}

		} else
        {
            if(ImGui::Begin(panel.title, &panel.is_visible, panel.window_flags))
                if(panel.render_callback)
                    panel.render_callback(&panel);
    
            ImGui::End();
        }
    }
}

void uph_panel_show(const char* title, bool visible)
{
	UphPanel* panel = uph_panel_get(title);
	assert(panel != nullptr && "Failed to load panel");
	panel->is_visible = visible;
}

UphPanel* uph_panel_get(const char* title)
{
    auto& vec = panels();
    auto it = std::lower_bound(vec.begin(), vec.end(), title, [](const UphPanel& a, const char* key) 
	{
        return std::strcmp(a.title, key) < 0;
    });

    if (it != vec.end() && std::strcmp(it->title, title) == 0)
        return &(*it);

	assert(false && "Panel not found");
    return nullptr;
}