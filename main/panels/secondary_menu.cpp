#include "panel_manager.h"
#include "../types.h"

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;
static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_AutoHideTabBar;

static void uph_secondary_menu_init(UphPanel* panel)
{
	// panel->panel_flags |= UphPanelFlags_HiddenFromMenu;
	panel->window_flags = window_flags;
	panel->dock_flags = dock_flags;
}

static void uph_secondary_menu_render(UphPanel* panel)
{
    ImGuiKnobs::Knob("Volume", &app->project.volume, 0.0f, 1.0f);
	ImGui::SameLine();
    ImGuiKnobs::Knob("Bpm", &app->project.bpm, 10.0f, 1000, 1.0f);
}

UPH_REGISTER_PANEL("Secondary Panel", UphPanelFlags_Panel, uph_secondary_menu_render, uph_secondary_menu_init);