#include "RecoveryPrompt.h"
#include "Core/ProjectManager.h"
#include "Core/Recovery.h"
#include "Naui/Localization/Localization.h"
#include <iostream>

RecoveryPrompt::RecoveryPrompt() : Naui::Panel(Naui::TR("recovery.title"))
{
	SetClosable(false);
	SetAutoResize(true);
	SetMinimizable(false);
	SetDockable(false);
	SetSerializable(false);
}

void RecoveryPrompt::OnRender()
{
	if(!recoverPath.has_value())
	{
		std::cout << "Unable to prompt recovery! Recovery path not set.\n";
		SetOpen(false);
		return;
	}

	ImGui::TextWrapped(Naui::TR("recovery.project_found"));
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button(Naui::TR("recovery.restore"), ImVec2(120, 0)))
	{
		Recovery::Recover(recoverPath.value());
		SetOpen(false);
	}

	ImGui::SameLine();

	if (ImGui::Button(Naui::TR("recovery.discard"), ImVec2(120, 0)))
	{
		Recovery::Discard(recoverPath.value());
		SetOpen(false);
	}
}