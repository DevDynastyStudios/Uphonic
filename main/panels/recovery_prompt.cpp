#include "panel_manager.h"
#include "../project_manager.h"

static ImGuiWindowFlags window_flags = 
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoDocking |
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_AlwaysAutoResize;

static void uph_recovery_prompt_init(UphPanel* panel)
{
	panel->window_flags = window_flags;
}

static void uph_recovery_prompt_render(UphPanel* panel)
{
    ImGui::TextWrapped("An unsaved project was found. Would you like to restore or discard it?");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Restore", ImVec2(120, 0))) 
	{
        uph_project_load_recovery();
        panel->is_visible = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Discard", ImVec2(120, 0))) 
	{
        uph_project_clear_recovery();
        panel->is_visible = false;
    }

}

UPH_REGISTER_PANEL("Recover Project", UphPanelFlags_Modal, uph_recovery_prompt_render, uph_recovery_prompt_init);