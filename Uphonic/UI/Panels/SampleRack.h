#pragma once

#include "Naui.h"
#include <filesystem>

struct AudioSample;

class SampleRack : public Naui::Panel
{
public:
    SampleRack();

	static size_t GetSampleIndex(AudioSample& sample);
	static AudioSample& GetSampleAtIndex(size_t index);
	static bool RenameSample(size_t index, const std::string& newName);

protected:
	void OnRegisterShortcuts(Naui::ShortcutTable& table) override;
    void OnRender() override;

private:
	bool DrawSampleItem(AudioSample& sample, float width, float height, bool& colorClicked);
    void DeleteSample(uint16_t index);
    
    int m_renamingIndex;
    char m_renameBuffer[128];
	bool m_justStartedRenaming;
};

