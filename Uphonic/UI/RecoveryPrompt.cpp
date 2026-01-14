#include "RecoveryPrompt.h"

RecoveryPrompt::RecoveryPrompt() : Naui::Panel("Recovery"){}

void RecoveryPrompt::OnRender()
{
	ImGui::TextWrapped("An unsaved project was found. Would you like to restore or discard it?");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Restore", ImVec2(120, 0))) 
	{
        //uph_project_load_recovery();
        SetOpen(false);
    }

    ImGui::SameLine();

    if (ImGui::Button("Discard", ImVec2(120, 0))) 
	{
        //uph_project_clear_recovery();
		SetOpen(false);
    }
}