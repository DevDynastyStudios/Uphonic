#include "RenamePatternAction.h"
#include "UI/PatternRack.h"
#include "Models/DataModel/Patterns.h"

RenamePatternAction::RenamePatternAction(MidiPattern& pattern, std::string newName) : pattern(pattern), newName(newName), oldName(pattern.name) {}

void RenamePatternAction::Do()
{
	size_t patternIndex = PatternRack::GetPatternIndex(pattern);
	PatternRack::RenamePattern(patternIndex, newName);
}

void RenamePatternAction::Undo()
{
	size_t patternIndex = PatternRack::GetPatternIndex(pattern);
	PatternRack::RenamePattern(patternIndex, oldName);
}