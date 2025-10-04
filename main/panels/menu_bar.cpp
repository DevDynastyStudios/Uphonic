#include "panel_manager.h"
#include <map>
#include <string>
#include <algorithm>

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar;
static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_AutoHideTabBar;

static void uph_menu_bar_init(UphPanel* panel)
{
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

	if (ImGui::BeginMenu("Window")) 
	{
	    if (ImGui::MenuItem("Show All"))
	        for (auto& panel : panels()) panel.is_visible = true;

	    if (ImGui::MenuItem("Hide All")) 
	        for (auto& panel : panels()) panel.is_visible = false;

	    ImGui::Separator();

	    std::map<std::string, std::vector<UphPanel*>> grouped;
	    for (auto& panel : panels()) 
		{
	        grouped[panel.category].push_back(&panel);
	    }

	    for (auto& [category, plist] : grouped) 
		{
	        if (ImGui::BeginMenu(category.c_str())) 
			{
	            std::sort(plist.begin(), plist.end(), [](UphPanel* a, UphPanel* b) 
				{ 
					return a->title < b->title; 
				});

	            for (auto* panel : plist) 
				{
	                if(ImGui::MenuItem(panel->title, nullptr, panel->is_visible))
					{
						uph_panel_show(panel->title, !panel->is_visible);
					}
	            }

	            ImGui::EndMenu();
	        }
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