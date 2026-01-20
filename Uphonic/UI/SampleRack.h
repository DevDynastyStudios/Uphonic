#pragma once

#include "Naui.h"
#include "Core/ProjectState.h"
#include <filesystem>

class SampleRack : public Naui::Panel
{
public:
    SampleRack();

protected:
    void OnRender() override;

private:
	bool DrawSampleItem(AudioSample& sample, float width, float height, bool& colorClicked);
    void DeleteSample(uint16_t index);
    
    int m_renamingIndex;
    char m_renameBuffer[128];
	bool m_justStartedRenaming;
};

