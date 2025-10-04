#include "panel_manager.h"
#include <map>
#include <string>
#include <algorithm>

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar;
static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_AutoHideTabBar;

static void uph_menu_bar_init(UphPanel* panel)
{
	panel->panel_flags |= UphPanelFlags_HiddenFromMenu;
	panel->window_flags = window_flags;
	panel->dock_flags = dock_flags;
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
	    // Bulk actions
	    if (ImGui::MenuItem("Show All")) {
	        for (auto& panel : panels()) panel.is_visible = true;
	    }
	    if (ImGui::MenuItem("Hide All")) {
	        for (auto& panel : panels()) panel.is_visible = false;
	    }

	    ImGui::Separator();

	    // --- Build category map and uncategorized list ---
	    std::map<std::string, std::vector<UphPanel*>> grouped;
	    std::vector<UphPanel*> uncategorized;

	    for (auto& panel : panels()) {
	        if (panel.panel_flags & UphPanelFlags_HiddenFromMenu)
	            continue;

	        if (panel.category && panel.category[0] != '\0') {
	            grouped[std::string(panel.category)].push_back(&panel);
	        } else {
	            uncategorized.push_back(&panel);
	        }
	    }

	    // --- Categorized panels first ---
	    for (auto& [category, plist] : grouped) {
	        if (ImGui::BeginMenu(category.c_str())) {
	            for (auto* panel : plist) {
	                ImGui::MenuItem(panel->title, nullptr, &panel->is_visible);
	            }
	            ImGui::EndMenu();
	        }
	    }

	    // --- Separator before uncategorized (only if they exist) ---
	    if (!uncategorized.empty() && !grouped.empty()) {
	        ImGui::Separator();
	    }

	    // --- Uncategorized panels at the bottom ---
	    for (auto* panel : uncategorized) {
	        ImGui::MenuItem(panel->title, nullptr, &panel->is_visible);
	    }

	    ImGui::EndMenu();
	}

    if(ImGui::BeginMenu("Help"))
    {
        uph_menu_bar_help_menu();
		ImGui::EndMenu();
    }
}

UPH_REGISTER_PANEL("Menu Bar", UphPanelFlags_MenuBar, uph_menu_bar_render, uph_menu_bar_init);