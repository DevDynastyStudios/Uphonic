#include "panel_manager.h"

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar;
static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_AutoHideTabBar;

static void uph_menu_bar_render(UphPanel* panel)
{
    if(ImGui::MenuItem("File"))
    {

    }

    if(ImGui::MenuItem("Edit"))
    {
        
    }

    if(ImGui::MenuItem("Options"))
    {
        
    }

    if(ImGui::MenuItem("Help"))
    {
        
    }
}

UPH_REGISTER_PANEL("Menu Bar", window_flags, dock_flags, uph_menu_bar_render);