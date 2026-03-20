#include "PluginTab.h"
#include "Models/EditorModel/ApplicationSettings.h"
#include <imgui.h>
#include <cstring>

void PluginTab::Draw(ApplicationSettings& draft)
{
	PluginSettings& p = draft.pluginSettings;

	ImGui::SeparatorText("Behaviour");
	{
		ImGui::Checkbox("Sandbox Plugins (separate process)", &p.sandboxPlugins);
		ImGui::Checkbox("Open Window on Insert", &p.openPluginWindowOnInsert);
		ImGui::Checkbox("Dock Plugin Windows", &p.dockPluginWindows);
		ImGui::DragFloat("Plugin Window Scale", &p.pluginWindowScale, 0.05f, 0.5f, 4.0f, "%.2f");
	}

	ImGui::SeparatorText("Sorting");
	{
		ImGui::Checkbox("Sort Alphabetically", &p.sortAlphabetically);
		ImGui::Checkbox("Group by Vendor", &p.groupByVendor);
		ImGui::Checkbox("Group by Category", &p.groupByCategory);
	}

	ImGui::SeparatorText("Search Paths");
	{
		for (int i = 0; i < (int)p.pluginPaths.size(); ++i)
		{
			ImGui::PushID(i);

			if (m_editingPathIndex == i)
			{
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
				if (ImGui::InputText("##path_edit", m_pathBuffer, sizeof(m_pathBuffer),
					ImGuiInputTextFlags_EnterReturnsTrue))
				{
					p.pluginPaths[i] = m_pathBuffer;
					m_editingPathIndex = -1;
				}
				if (ImGui::IsItemDeactivatedAfterEdit() ||
					(ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered()))
					m_editingPathIndex = -1;

				ImGui::SameLine();
				if (ImGui::SmallButton("OK"))
				{
					p.pluginPaths[i] = m_pathBuffer;
					m_editingPathIndex = -1;
				}
			}
			else
			{
				ImGui::TextUnformatted(p.pluginPaths[i].c_str());
				ImGui::SameLine();
				if (ImGui::SmallButton("Edit"))
				{
					m_editingPathIndex = i;
					strncpy(m_pathBuffer, p.pluginPaths[i].c_str(), sizeof(m_pathBuffer) - 1);
					m_pathBuffer[sizeof(m_pathBuffer) - 1] = '\0';
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("X"))
				{
					p.pluginPaths.erase(p.pluginPaths.begin() + i);
					ImGui::PopID();
					break;
				}
			}

			ImGui::PopID();
		}

		if (ImGui::Button("+ Add Path"))
			p.pluginPaths.emplace_back("");
	}

	ImGui::SeparatorText("Blacklist");
	ImGui::TextDisabled("%zu entries", p.blackList.size());
}