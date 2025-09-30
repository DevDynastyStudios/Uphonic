#include "panel_manager.h"
#include "../settings/layout_manager.h"

struct UphTempoTapper
{
    char new_layout_name[64] = "";
    std::string current_layout = "Layout";
    bool open_save_as_popup = false;
    bool popup_focus_request = false;
};

static UphTempoTapper tapper_data {};

static void uph_tempo_tapper_render(UphPanel* panel)
{
    float dropdownWidth = 100.0f;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - dropdownWidth);
    ImGui::SetNextItemWidth(dropdownWidth);

    if (ImGui::BeginCombo("##LayoutCombo", tapper_data.current_layout.c_str()))
    {
        // --- Collect layouts explicitly from "layouts" folder ---
        std::vector<std::string> layouts = uph_list_layouts("layouts");

        // --- List layouts ---
        for (auto& layout : layouts)
        {
            bool selected = (layout == tapper_data.current_layout);
            if (ImGui::Selectable(layout.c_str(), selected))
            {
                tapper_data.current_layout = layout;
                uph_load_layout(("layouts/" + layout).c_str());
            }
        }

        ImGui::Separator();

        // --- Save Layout As ---
        if (ImGui::Selectable("Save Layout As..."))
        {
            tapper_data.new_layout_name[0] = '\0';
            tapper_data.open_save_as_popup = true;      // defer until after EndCombo
            tapper_data.popup_focus_request = true;     // request focus on first frame
        }

        // --- Delete Layout submenu ---
        if (ImGui::BeginMenu("Delete Layout"))
        {
            for (auto& layout : uph_list_layouts("layouts"))
            {
				std::string path = "layouts/" + layout;

				// Skip immutable layouts entirely
                if (uph_layout_is_immutable(path.c_str()))
					continue;

                if (ImGui::MenuItem(layout.c_str()))
                {
                    uph_remove_layout(path.c_str());
                    if (tapper_data.current_layout == layout)
                        tapper_data.current_layout = "Layout";
                }
            }
            ImGui::EndMenu();
        }

    if(ImGui::Button("Load Layout"))
    {
        uph_load_layout("layouts/Default");
    }
}

UPH_REGISTER_PANEL("Tempo Tapper", ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, uph_tempo_tapper_render);