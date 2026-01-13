#pragma once

#include "Naui.h"
#include "../Core/ProjectState.h"

class SampleRack : public Naui::Panel
{
public:
    SampleRack();

protected:
    void OnRender() override;

private:
    void DeleteSample(uint16_t index);
    
    int m_renamingIndex;
    char m_renameBuffer[128];
};

