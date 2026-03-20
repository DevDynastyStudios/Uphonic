#pragma once

#include "Naui.h"

struct MidiPattern;

class PatternRack : public Naui::Panel
{
public:
    PatternRack();
	static size_t GetPatternIndex(MidiPattern& pattern);
	static MidiPattern& GetPatternAtIndex(size_t index);
	static bool RenamePattern(size_t index, const std::string& newName);


protected:
	void OnRegisterShortcuts(Naui::ShortcutTable& table) override;
    void OnRender() override;

private:
    void DeletePattern(uint16_t index);
    void DuplicatePattern(uint16_t index);
    
    int m_renamingIndex;
    char m_renameBuffer[128];
};

