#include "panel_manager.h"
#include "../settings/layout_manager.h"
#include <map>
#include <string>
#include <algorithm>

struct UphMenuBar
{
	char new_layout_name[64] = "";
    std::string current_layout = "Layout";
    bool open_save_as_popup = false;
    bool popup_focus_request = false;
};

static UphMenuBar menu_bar_data = {};

static void uph_menu_bar_init(UphPanel* panel)
{
	panel->panel_flags |= UphPanelFlags_HiddenFromMenu;
	panel->window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar;
	panel->dock_flags = ImGuiDockNodeFlags_AutoHideTabBar;
}

static void uph_menu_bar_set_panel_visibility(bool visible)
{
    for (auto& panel : panels()) 
	{
		if(panel.panel_flags & UphPanelFlags_HiddenFromMenu)
			continue;

		panel.is_visible = visible;
	}
}

static void uph_menu_bar_window_menu()
{
	 // Bulk actions
	if (ImGui::MenuItem("Show All"))
		uph_menu_bar_set_panel_visibility(true);

	if (ImGui::MenuItem("Hide All"))
		uph_menu_bar_set_panel_visibility(false);

	ImGui::Separator();

	// --- Build category map and uncategorized list ---
	std::map<std::string, std::vector<UphPanel*>> grouped;
	std::vector<UphPanel*> uncategorized;

    for (auto& panel : panels()) 
	{
        if (panel.panel_flags & UphPanelFlags_HiddenFromMenu)
            continue;

	    if (panel.category && panel.category[0] != '\0') 
		{
	        grouped[std::string(panel.category)].push_back(&panel);
	    } else 
		{
	        uncategorized.push_back(&panel);
	    }
	}

    // --- Categorized panels first ---
    for (auto& [category, plist] : grouped) 
	{
        if (ImGui::BeginMenu(category.c_str())) 
		{
            for (auto* panel : plist) 
			{
                ImGui::MenuItem(panel->title, nullptr, &panel->is_visible);
            }
			
            ImGui::EndMenu();
        }
    }

	// --- Separator before uncategorized (only if they exist) ---
	if (!uncategorized.empty() && !grouped.empty()) 
	{
	    ImGui::Separator();
	}

	// --- Uncategorized panels at the bottom ---
	for (auto* panel : uncategorized) 
	{
	    ImGui::MenuItem(panel->title, nullptr, &panel->is_visible);
	}
}

static void uph_menu_bar_layout_dropdown()
{
    float dropdownWidth = 100.0f;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - dropdownWidth);
    ImGui::SetNextItemWidth(dropdownWidth);

    if (ImGui::BeginCombo("##LayoutCombo", menu_bar_data.current_layout.c_str()))
    {
        // --- Collect layouts explicitly from "layouts" folder ---
        std::vector<std::string> layouts = uph_list_layouts("layouts");

        // --- List layouts ---
        for (auto& layout : layouts)
        {
            bool selected = (layout == menu_bar_data.current_layout);
            if (ImGui::Selectable(layout.c_str(), selected))
            {
                menu_bar_data.current_layout = layout;
                uph_load_layout(("layouts/" + layout).c_str());
            }
        }

        ImGui::Separator();

        // --- Save Layout As ---
        if (ImGui::Selectable("Save Layout As..."))
        {
            menu_bar_data.new_layout_name[0] = '\0';
            menu_bar_data.open_save_as_popup = true;
            menu_bar_data.popup_focus_request = true;
        }

        // --- Delete Layout submenu ---
        if (ImGui::BeginMenu("Delete Layout"))
        {
            for (auto& layout : uph_list_layouts("layouts"))
            {
                std::string path = "layouts/" + layout;

                if (uph_layout_is_immutable(path.c_str()))
                    continue;

                if (ImGui::MenuItem(layout.c_str()))
                {
                    uph_remove_layout(path.c_str());
                    if (menu_bar_data.current_layout == layout)
                        menu_bar_data.current_layout = "Layout";
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndCombo();
    }

    if (menu_bar_data.open_save_as_popup)
    {
        ImGui::OpenPopup("SaveLayoutAsPopup");
        menu_bar_data.open_save_as_popup = false;
    }

    // --- Save Layout As popup ---
    if (ImGui::BeginPopup("SaveLayoutAsPopup"))
    {
        ImGui::Text("Enter new layout name:");

        if (menu_bar_data.popup_focus_request)
        {
            ImGui::SetKeyboardFocusHere();
            menu_bar_data.popup_focus_request = false;
        }

        if (ImGui::InputText("##newlayout",
                             menu_bar_data.new_layout_name,
                             sizeof(menu_bar_data.new_layout_name),
                             ImGuiInputTextFlags_EnterReturnsTrue))
        {
            std::string filename = std::string("layouts/") + menu_bar_data.new_layout_name;
            uph_save_layout(filename.c_str());
            menu_bar_data.current_layout = menu_bar_data.new_layout_name;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Save"))
        {
            std::string filename = std::string("layouts/") + menu_bar_data.new_layout_name;
            uph_save_layout(filename.c_str());
            menu_bar_data.current_layout = menu_bar_data.new_layout_name;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

static void uph_menu_bar_file_menu()
{
	if(ImGui::MenuItem("New Project"))
	{
	}
	
	if(ImGui::MenuItem("Open"))
	{
	}

	ImGui::Separator();

	if(ImGui::MenuItem("Save"))
	{
	}

	if(ImGui::MenuItem("Save As"))
	{
	}

	ImGui::Separator();

	if(ImGui::BeginMenu("Import"))
	{
		if(ImGui::MenuItem("MIDI"))
		{
		}
		ImGui::EndMenu();
	}

	if(ImGui::BeginMenu("Export"))
	{
		if(ImGui::MenuItem("Wave file..."))
		{
		}
		if(ImGui::MenuItem("Ogg file..."))
		{
			
		}
		if(ImGui::MenuItem("Mp3 file..."))
		{
			
		}
		if(ImGui::MenuItem("FLAC file..."))
		{
			
		}
		if(ImGui::MenuItem("M4A file..."))
		{
			
		}
		if(ImGui::MenuItem("MIDI file..."))
		{
			
		}
		ImGui::EndMenu();
	}

	ImGui::Separator();
	
	if(ImGui::MenuItem("Exit"))
	{
	}

}

static void uph_menu_bar_edit_menu()
{
	if(ImGui::MenuItem("Undo"))
	{
		
	}

	ImGui::Separator();

	if(ImGui::MenuItem("Cut"))
	{
		
	}

	if(ImGui::MenuItem("Copy"))
	{
		
	}

	if(ImGui::MenuItem("Paste"))
	{
		
	}

}

static void uph_menu_bar_options_menu()
{
	if(ImGui::MenuItem("MIDI Settings"))
	{

	}

	if(ImGui::MenuItem("Audio Settings"))
	{
		
	}

	if(ImGui::MenuItem("General Settings"))
	{
		
	}
}

static void uph_menu_bar_help_menu()
{
	if(ImGui::MenuItem("Tutorials"))
	{

	}

	if(ImGui::MenuItem("About"))
	{

	}
}

static void uph_menu_bar_render(UphPanel* panel)
{

	if(ImGui::BeginMenu("File"))
    {
		uph_menu_bar_file_menu();
		ImGui::EndMenu();
	}

    if(ImGui::BeginMenu("Edit"))
    {
        uph_menu_bar_edit_menu();
		ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Options"))
    {
		uph_menu_bar_options_menu();
        ImGui::EndMenu();
    }

	if (ImGui::BeginMenu("Window")) {
		uph_menu_bar_window_menu();
	    ImGui::EndMenu();
	}

    if(ImGui::BeginMenu("Help"))
    {
        uph_menu_bar_help_menu();
		ImGui::EndMenu();
    }

	uph_menu_bar_layout_dropdown();
}

UPH_REGISTER_PANEL("Menu Bar", UphPanelFlags_MenuBar, uph_menu_bar_render, uph_menu_bar_init);