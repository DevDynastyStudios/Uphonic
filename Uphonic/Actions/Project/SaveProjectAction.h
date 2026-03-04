#pragma once
#include "Naui/Actions/IAction.h"
#include <string>

class ProjectState;

class SaveProjectAction : public Naui::IAction
{
public:
	SaveProjectAction(ProjectState& state);
	void Do() override;
	void Undo() override;
	const char* Name() const override { return "Save Project"; }

private:
	ProjectState& state;
};