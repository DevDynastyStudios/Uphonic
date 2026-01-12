#pragma once

#include "Naui.h"
#include "../Core/ProjectState.h"

class PatternRack : public Naui::Panel
{
public:
    PatternRack();

protected:
    void OnRender() override;

private:
    void DeletePattern(uint16_t index);
    void DuplicatePattern(uint16_t index);
    
    int m_renamingIndex;
    char m_renameBuffer[128];
};

