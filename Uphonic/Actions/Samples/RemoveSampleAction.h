#pragma once
#include "Naui/Actions/IAction.h"
#include <string>

struct AudioSample;

class RemoveSampleAction : public Naui::IAction
{
public:
	RemoveSampleAction(AudioSample& sample);
	void Do() override;
	void Undo() override;
	const char* Name() const override { return "Remove Sample"; }

private:
	AudioSample& sample;
};