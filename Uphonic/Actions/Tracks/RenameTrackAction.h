#pragma once
#include "Naui/Actions/IAction.h"
#include <string>

struct AudioTrack;

class RenameTrackAction : public Naui::IAction
{
public:
	RenameTrackAction(AudioTrack& track, std::string newName);
	void Do() override;
	void Undo() override;
	const char* Name() const override { return "Track Volume"; }

private:
	AudioTrack& track;
	std::string newName;
	std::string oldName;
};