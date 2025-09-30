#include "panel_manager.h"

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;
static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_AutoHideTabBar;

static void uph_secondary_menu_render(UphPanel* panel)
{

}

UPH_REGISTER_PANEL("Secondary Panel", window_flags, dock_flags, uph_secondary_menu_render);