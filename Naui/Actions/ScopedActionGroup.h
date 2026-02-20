#pragma once
#include "ActionManager.h"
#include "ActionGroup.h"

namespace Naui
{
class ScopedActionGroup
{
public:
	ScopedActionGroup(ActionManager& manager) : manager(manager)
	{
		group = new ActionGroup();
	}

	~ScopedActionGroup()
	{
		manager.Execute(group);
	}

	void Add(IAction* action)
	{
		if(action)
			group->Add(action);
	}

private:
	ActionManager& manager;
	ActionGroup* group;
};
}