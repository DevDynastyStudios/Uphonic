#pragma once
#include "Naui/Actions/IAction.h"
#include <string>

class AudioSample;

class RenameSampleAction : public Naui::IAction
{
public:
	RenameSampleAction(AudioSample& sample, std::string newName);
	void Do() override;
	void Undo() override;
	const char* Name() const override { return "Rename Sample"; }

private:
	AudioSample& sample;
	std::string newName;
	std::string oldName;
};