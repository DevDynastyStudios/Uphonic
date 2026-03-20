#include "GeneralTab.h"
#include "Models/EditorModel/ApplicationSettings.h"
#include "Naui/Localization/Localization.h"
#include <imgui.h>
#include <cstring>

void GeneralTab::Draw(ApplicationSettings& draft)
{
	GeneralSettings& g = draft.generalSettings;
	ImGui::SeparatorText("Appearance");
	{
		char themeBuf[64];
		strncpy(themeBuf, g.theme.c_str(), sizeof(themeBuf) - 1);
		themeBuf[sizeof(themeBuf) - 1] = '\0';
		if (ImGui::InputText("Theme", themeBuf, sizeof(themeBuf)))
			g.theme = themeBuf;

		ImGui::DragFloat("UI Scale", &g.uiScale, 0.01f, 0.5f, 4.0f, "%.2f");
		ImGui::DragFloat("Font Size", &g.fontSize, 0.5f, 8.0f, 48.0f, "%.1f");
	}

	ImGui::SeparatorText("Language");
	{
		const auto& languages = Naui::Localization::GetLanguages();

		const std::string currentCode = g.languageCode + "-" + g.regionCode;
		int currentIdx = -1;
		for (int i = 0; i < (int)languages.size(); ++i)
		{
			if (languages[i].code == currentCode)
			{
				currentIdx = i;
				break;
			}
		}

		const char* preview = (currentIdx >= 0) ? languages[currentIdx].displayName.c_str() : currentCode.c_str();
		if (ImGui::BeginCombo("Language / Region", preview))
		{
			for (int i = 0; i < (int)languages.size(); ++i)
			{
				const bool selected = (i == currentIdx);
				if (ImGui::Selectable(languages[i].displayName.c_str(), selected))
				{
					const std::string& code = languages[i].code;
					const size_t dash = code.find('-');
					if (dash != std::string::npos)
					{
						g.languageCode = code.substr(0, dash);
						g.regionCode = code.substr(dash + 1);
					}
					else
					{
						g.languageCode = code;
						g.regionCode = {};
					}
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
	}

	ImGui::SeparatorText("Project");
	{
		ImGui::Checkbox("Open Last Project on Startup", &g.openLastProjectOnStartup);
		ImGui::Checkbox("Autosave", &g.autosaveEnabled);

		int undoLimit = (int)g.undoHistoryLimit;
		if (ImGui::DragInt("Undo History Limit", &undoLimit, 1, 16, 1024))
			g.undoHistoryLimit = (uint32_t)undoLimit;
	}

	ImGui::SeparatorText("Confirmations");
	{
		ImGui::Checkbox("Confirm on Exit", &g.confirmOnExit);
		ImGui::Checkbox("Confirm on Delete", &g.confirmOnDelete);
		ImGui::Checkbox("Show Tooltips", &g.showTooltips);
	}

	ImGui::SeparatorText("Scroll & Zoom");
	{
		ImGui::DragFloat("Horizontal Scroll Sensitivity", &g.horizontalScrollSensitivity, 0.5f, 1.0f, 200.0f, "%.1f");
		ImGui::DragFloat("Vertical Scroll Sensitivity", &g.verticalScrollSensitivity, 0.5f, 1.0f, 200.0f, "%.1f");
		ImGui::DragFloat("Horizontal Zoom Sensitivity", &g.horizontalZoomSensitivity, 0.005f, 0.01f, 1.0f, "%.3f");
		ImGui::DragFloat("Vertical Zoom Sensitivity", &g.verticalZoomSensitivity, 0.005f, 0.01f, 1.0f, "%.3f");
	}
}