#include "panel_manager.h"

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;
static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_AutoHideTabBar;

static void uph_secondary_menu_init(UphPanel* panel)
{
	panel->window_flags = window_flags;
	panel->dock_flags = dock_flags;
}

static void uph_secondary_menu_render(UphPanel* panel)
{

}

UPH_REGISTER_PANEL("Secondary Panel", UphPanelFlags_Panel, uph_secondary_menu_render, uph_secondary_menu_init);