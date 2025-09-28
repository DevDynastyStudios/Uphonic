#include "panel_manager.h"

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar;
static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_AutoHideTabBar;

static void uph_menu_bar_render(UphPanel* panel)
{
    // ImGuiDockNodeFlags dock_flags =  ImGuiDockNodeFlags_NoUndocking | ImGuiDockNodeFlags_AutoHideTabBar;
    // ImGui::SetWindowD
    // ImGuiDockNode* node = ImGui::GetWindowDockNode();
    // if (node && node->IsDocked) {
    //     // Force hide title bar if docked
    //     ImGui::SetWindowCollapsed(false, ImGuiCond_Always);
    //     ImGui::SetWindowDockingFlags(ImGuiDockNodeFlags_NoTabBar);
    // }
}

UPH_REGISTER_PANEL("Menu Bar", window_flags, dock_flags, uph_menu_bar_render);