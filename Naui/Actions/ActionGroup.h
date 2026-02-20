#pragma once
#include "IAction.h"
#include <vector>

namespace Naui
{
class ActionGroup : public IAction
{
public:
	void Add(IAction* action) { actions.push_back(action); }

	void Do() override
	{
		for(IAction* action : actions)
			action->Do();
	}

	void Undo() override
	{
		for(int i = (int) actions.size() - 1; i >= 0; --i)
			actions[i]->Undo();
	}

	const char* Name() const override { return "Action Group"; }

private:
	std::vector<IAction*> actions;
};
}