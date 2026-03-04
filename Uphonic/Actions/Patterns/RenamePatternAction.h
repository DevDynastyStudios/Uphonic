#pragma once
#include "Naui/Actions/IAction.h"
#include <string>

struct MidiPattern;

class RenamePatternAction : public Naui::IAction
{
public:
	RenamePatternAction(MidiPattern& pattern, std::string newName);
	void Do() override;
	void Undo() override;
	const char* Name() const override { return "Rename Pattern"; }

private:
	MidiPattern& pattern;
	std::string newName;
	std::string oldName;
};