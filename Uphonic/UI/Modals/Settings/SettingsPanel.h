#pragma once

#include "Naui.h"
#include "Models/EditorModel/ApplicationSettings.h"
#include "PluginTab.h"

class SettingsPanel : public Naui::Modal
{
public:
	SettingsPanel();

protected:
	void OnRender() override;
	void OnClose() override;

private:
	void DrawSidebar();
	void DrawContent();
	void DrawFooter();
	void SaveSettings();

	static void PushSettingStyle();
	static void PopSettingStyle();
	static void SectionHeader(const char* label);
	static float BeginRow(const char* label);

	ApplicationSettings m_draft;
	int m_activeTab   = 0;
	bool m_hasChanges  = false;
	bool m_initialized = false;
	PluginTab m_pluginTab;

	static constexpr float SIDEBAR_WIDTH = 130.0f;
	static constexpr float FOOTER_HEIGHT = 44.0f;
};