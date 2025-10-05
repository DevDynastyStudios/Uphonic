#include "panel_manager.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <imgui.h>

std::vector<UphPanel>& panels() {
    static std::vector<UphPanel> instance; // constructed on first use
    return instance;
}

void uph_panel_register(const char* title, UphPanelFlags panel_flags, UphPanelCallback render_callback, UphPanelCallback init_callback)
{
    UphPanel panel{ title, nullptr, false, panel_flags, ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, render_callback, init_callback };
    panels().push_back(panel);
}

void uph_panel_init_all()
{
    for (UphPanel& panel : panels())
    {
        if (panel.init_callback)
            panel.init_callback(&panel);
    }

    // Keep panels sorted by title for uph_panel_get (binary search)
    std::sort(panels().begin(), panels().end(), [](const UphPanel& a, const UphPanel& b) 
	{ 
		return std::strcmp(a.title, b.title) < 0; 
	});
}

void uph_panel_render_all()
{
    std::vector<UphPanel*> popup_panels;
    std::vector<UphPanel*> modal_panels;

    for (UphPanel& panel : panels())
    {
        if (!panel.is_visible && !(panel.panel_flags & UphPanelFlags_Popup) && !(panel.panel_flags & UphPanelFlags_Modal))
            continue;

        if (panel.panel_flags & UphPanelFlags_MenuBar)
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (panel.render_callback)
                    panel.render_callback(&panel);
                ImGui::EndMainMenuBar();
            }
        }
        else if (panel.panel_flags & UphPanelFlags_Modal)
        {
            modal_panels.push_back(&panel);
        }
        else if (panel.panel_flags & UphPanelFlags_Popup)
        {
            popup_panels.push_back(&panel);
        }
        else
        {
            if (ImGui::Begin(panel.title, &panel.is_visible, panel.window_flags))
            {
                if (panel.render_callback)
                    panel.render_callback(&panel);
            }

            ImGui::End();
        }
    }

    for (UphPanel* panel : modal_panels)
    {
        if (panel->is_visible)
        {
            ImGui::OpenPopup(panel->title);
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(600, 300));
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImGui::GetMainViewport()->Pos,
            ImGui::GetMainViewport()->Pos + ImGui::GetMainViewport()->Size,
            IM_COL32(0, 0, 0, 128));

        if (ImGui::BeginPopupModal(panel->title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        {
            if (panel->render_callback)
                panel->render_callback(panel);


			if(!panel->is_visible)
				ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }

    bool any_popup_open_request = false;
    for (UphPanel* panel : popup_panels)
    {
        if (panel->is_visible)
            any_popup_open_request = true;
    }

    ImGui::SetNextWindowSize(ImVec2(1, 1));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos, ImGuiCond_None);
    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoInputs
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBackground
        | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##PopupHost", nullptr, host_flags);

    for (UphPanel* panel : popup_panels)
    {
        if (panel->is_visible)
            ImGui::OpenPopup(panel->title);

    }

    for (UphPanel* panel : popup_panels)
    {
        if (ImGui::BeginPopup(panel->title))
        {
            if (panel->render_callback)
                panel->render_callback(panel);

			 if(!panel->is_visible)
			 	ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }

    ImGui::End();
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