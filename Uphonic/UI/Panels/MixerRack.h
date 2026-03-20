#pragma once

#include "Naui.h"
#include "Config/EditorConfig.h"
#include "Vendor/imgui-knobs/imgui-knobs.h"
#include <vector>

class MixerRack : public Naui::Panel
{
public:
    MixerRack();

protected:
	void OnRegisterShortcuts(Naui::ShortcutTable& table) override;
    void OnRender() override;

private:
    void RenderMasterStrip(bool isSelected);
    void RenderChannelStrip(size_t idx, bool isSelected);
    void DrawVUMeterWithFader(float vuLevelLeft, float vuLevelRight, float& volume, float width, float height, ImU32 channelColor);
    void RenderEffectsPanel();
    
    MixerConfig m_config;
    int m_selectedTrack;
	int m_scrollToSelectedFrames = 0;
	bool m_effectsPanelOpen = false;
	bool m_scrollToSelected;
};