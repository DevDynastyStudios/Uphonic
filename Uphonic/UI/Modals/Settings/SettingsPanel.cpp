#include "SettingsPanel.h"
#include "Core/ProjectState.h"
#include "Naui/FileSystem/File.h"
#include "Naui/Localization/Localization.h"
#include "Naui/Core/Theme.h"
#include "Serializer/EditorSerializer.h"

#include "GeneralTab.h"
#include "AudioTab.h"
#include "MidiTab.h"
#include "TimelineTab.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <algorithm>
#include <string>

void SettingsPanel::PushSettingStyle()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.22f, 0.25f, 1.0f));
}

void SettingsPanel::PopSettingStyle()
{
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(3);
}

void SettingsPanel::SectionHeader(const char* label)
{
	ImGui::Spacing();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.58f, 1.0f));
	ImGui::TextUnformatted(label);
	ImGui::PopStyleColor();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	dl->AddLine(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y), IM_COL32(50, 50, 55, 255));
	ImGui::Dummy(ImVec2(0, 4.0f));
}

float SettingsPanel::BeginRow(const char* label)
{
	constexpr float LABEL_RATIO = 0.44f;
	const float	avail = ImGui::GetContentRegionAvail().x;
	const float	labelW = avail * LABEL_RATIO;
	const float	controlW = avail - labelW - 8.0f;
	ImGui::TextUnformatted(label);
	ImGui::SameLine(labelW);
	return controlW;
}

SettingsPanel::SettingsPanel() : Naui::Modal(Naui::TR("settings.title"))
{
	SetMinSize(820.0f, 640.0f);
	SetFocusPolicy(Naui::ModalFocusPolicy::Free);
	SetCloseOnOverlayClick(false);
	SetAllowMultipleInstances(false);
}

void SettingsPanel::OnClose()
{
	m_hasChanges = false;
}

void SettingsPanel::OnRender()
{
	if (!m_initialized)
	{
		m_draft	= ProjectState::GetInstance().settings;
		m_initialized = true;
	}

	const ImVec2 avail = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - FOOTER_HEIGHT);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	if (ImGui::BeginChild("##settings_body", avail, false, ImGuiWindowFlags_NoScrollbar))
	{
		DrawSidebar();
		ImGui::SameLine();
		DrawContent();
	}

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
	DrawFooter();
}



void SettingsPanel::DrawSidebar()
{
	static const char* labels[] = { "General", "Audio", "MIDI", "Timeline", "Plugins" };
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.11f, 1.0f));
	ImGui::BeginChild("##sidebar", ImVec2(SIDEBAR_WIDTH, 0), false, ImGuiWindowFlags_NoScrollbar);
	ImGui::Dummy(ImVec2(0, 10.0f));

	for (int i = 0; i < IM_ARRAYSIZE(labels); ++i)
	{
		const bool active = (m_activeTab == i);
		if (active)
		{
			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 3.0f, p.y + 28.0f), IM_COL32(82, 140, 220, 255));
		}

		ImGui::PushStyleColor(ImGuiCol_Header, active ? ImVec4(0.16f, 0.17f, 0.19f, 1.0f) : ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.16f, 0.17f, 0.19f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.20f, 0.21f, 0.24f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4(0.90f, 0.91f, 0.94f, 1.0f) : ImVec4(0.54f, 0.55f, 0.58f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.12f, 0.5f));

		if (ImGui::Selectable(labels[i], active, 0, ImVec2(SIDEBAR_WIDTH, 28.0f)))
			m_activeTab = i;

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void SettingsPanel::DrawContent()
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.13f, 0.145f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 16.0f));
	ImGui::BeginChild("##content", ImVec2(0, 0), false);

	PushSettingStyle();

	const ApplicationSettings before = m_draft;

	switch (m_activeTab)
	{
		case 0: GeneralTab::Draw(m_draft);	break;
		case 1: AudioTab::Draw(m_draft);	break;
		case 2: MidiTab::Draw(m_draft);		break;
		case 3: TimelineTab::Draw(m_draft);	break;
		case 4: m_pluginTab.Draw(m_draft);	break;
	}

	if (!m_hasChanges)
	{
		m_hasChanges =
			memcmp(&before.generalSettings, &m_draft.generalSettings, sizeof(GeneralSettings)) != 0 ||
			memcmp(&before.audioSettings, &m_draft.audioSettings, sizeof(AudioSettings)) != 0 ||
			memcmp(&before.midiSettings, &m_draft.midiSettings,	sizeof(MIDISettings)) != 0 ||
			memcmp(&before.timelineSettings, &m_draft.timelineSettings, sizeof(TimelineSettings)) != 0 ||
			before.pluginSettings.pluginPaths != m_draft.pluginSettings.pluginPaths ||
			before.pluginSettings.blackList != m_draft.pluginSettings.blackList;
	}

	PopSettingStyle();
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

void SettingsPanel::DrawFooter()
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.11f, 1.0f));
	ImGui::BeginChild("##footer", ImVec2(0, FOOTER_HEIGHT), false, ImGuiWindowFlags_NoScrollbar);

	ImVec2 wp = ImGui::GetWindowPos();
	ImGui::GetWindowDrawList()->AddLine(wp, ImVec2(wp.x + ImGui::GetWindowWidth(), wp.y), IM_COL32(38, 38, 42, 255));

	ImGui::Dummy(ImVec2(0, 9.0f));

	const float buttonW = 116.0f;
	const float resetW = 130.0f;
	const float gap = 8.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

	ImGui::SetCursorPosX(8.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.14f, 0.14f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.18f, 0.18f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.20f, 0.20f, 1.0f));

	if (ImGui::Button("Reset to Defaults", ImVec2(resetW, 26.0f)))
		ImGui::OpenPopup("##reset_confirm");

	ImGui::PopStyleColor(3);

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y - 66.0f));
	if (ImGui::BeginPopup("##reset_confirm"))
	{
		ImGui::TextUnformatted("Reset all settings to defaults?");
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.18f, 0.18f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.20f, 0.20f, 1.0f));

		if (ImGui::Button("Reset", ImVec2(72.0f, 22.0f)))
		{
			m_draft = ApplicationSettings{};
			m_hasChanges = true;
			SaveSettings();
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine(0, gap);
		if (ImGui::Button("Cancel##reset", ImVec2(72.0f, 22.0f)))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	const float rightX = ImGui::GetContentRegionAvail().x - buttonW * 2 - gap;
	ImGui::SameLine(rightX + 8.0f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.24f, 0.27f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.28f, 0.32f, 1.0f));

	if (ImGui::Button("Cancel", ImVec2(buttonW, 26.0f)))
	{
		m_draft = ProjectState::GetInstance().settings;
		m_hasChanges  = false;
		m_initialized = false;
		SetOpen(false);
	}

	ImGui::PopStyleColor(3);
	ImGui::SameLine(0, gap);

	const bool canApply = m_hasChanges;
	const ImGuiStyle& style = ImGui::GetStyle();
	const float alpha = canApply ? 1.0f : 0.35f;

	auto DimColor = [alpha](ImVec4 c) {
		return ImVec4(c.x, c.y, c.z, c.w * alpha);
	};

	ImGui::PushStyleColor(ImGuiCol_Button, DimColor(style.Colors[ImGuiCol_Button]));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DimColor(style.Colors[ImGuiCol_ButtonHovered]));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, DimColor(style.Colors[ImGuiCol_ButtonActive]));

	if (ImGui::Button("Apply Changes", ImVec2(buttonW, 26.0f)) && canApply)
	{
		SaveSettings();
		SetOpen(false);
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(2);
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void SettingsPanel::SaveSettings()
{
	ProjectState& state = ProjectState::GetInstance();
	const GeneralSettings& oldGeneral = state.settings.generalSettings;
	const GeneralSettings& newGeneral = m_draft.generalSettings;

	if (oldGeneral.languageCode != newGeneral.languageCode || oldGeneral.regionCode != newGeneral.regionCode)
		Naui::Localization::SetLanguage(newGeneral.languageCode + "-" + newGeneral.regionCode);

	if (oldGeneral.theme != newGeneral.theme)
	{
		const std::filesystem::path systemPath = Naui::Directory::BinDirectory() / "Themes" / (newGeneral.theme);
		const std::filesystem::path userPath = Naui::Directory::WorkspaceDirectory().parent_path() / "Themes" / (newGeneral.theme);

		if (std::filesystem::exists(systemPath))
			Naui::Theme::Load(systemPath.string().c_str());
		else if (std::filesystem::exists(userPath))
			Naui::Theme::Load(userPath.string().c_str());
	}

	state.settings = m_draft;
	m_hasChanges = false;
	m_initialized = false;
	EditorSerializer::SaveSettings(state);
}