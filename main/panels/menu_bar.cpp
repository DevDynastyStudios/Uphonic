#include "panel_manager.h"

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar;
static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_AutoHideTabBar;

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
    	for (auto& panel : panels()) {
        	ImGui::MenuItem(panel.title, nullptr, panel.is_visible);
    	}

    	ImGui::EndMenu();
	}

    if(ImGui::BeginMenu("Help"))
    {
        uph_menu_bar_help_menu();
		ImGui::EndMenu();
    }
}

UPH_REGISTER_PANEL("Menu Bar", window_flags, dock_flags, uph_menu_bar_render);