#pragma once
#include <vector>
#include <cstddef>
#include "IAction.h"

namespace Naui
{
class ActionManager
{
public:
	ActionManager(int historyLimit = 256) : m_historyLimit(historyLimit) {}

	~ActionManager() { Clear(); }

	template<typename T, typename... Args>
	void Execute(Args&&... args)
	{
		IAction* action = new T(std::forward<Args>(args)...);
		action->Do();
		undoStack.push_back(action);

		for (auto* a : redoStack)
			delete a;

		redoStack.clear();
		EnforceHistoryLimit();
	}

	void Execute(IAction* action)
	{
		if (!action)
			return;
	
		action->Do();
		undoStack.push_back(action);
	
		for (auto* a : redoStack)
			delete a;

		redoStack.clear();
		EnforceHistoryLimit();
	}

	template<typename T, typename... Args>
	void ExecuteWithoutHistory(Args&&... args)
	{
		IAction* action = new T(std::forward<Args>(args)...);
		action->Do();
		delete action;
	}

	void ExecuteWithoutHistory(IAction* action)
	{
		if(!action)
			return;

		action->Do();
		delete action;
	}

	void Undo()
	{
		if (undoStack.empty())
			return;

		IAction* action = undoStack.back();
		undoStack.pop_back();

		action->Undo();
		redoStack.push_back(action);
	}

	void Redo()
	{
		if (redoStack.empty())
			return;

		IAction* action = redoStack.back();
		redoStack.pop_back();

		action->Do();
		undoStack.push_back(action);
	}

	bool CanUndo() const { return !undoStack.empty(); }
	bool CanRedo() const { return !redoStack.empty(); }

	void Clear()
	{
		for (auto* a : undoStack)
			delete a;

		for (auto* a : redoStack)
			delete a;

		undoStack.clear();
		redoStack.clear();
	}

	void ClearHistory()
	{
		for (auto* a : undoStack)
			delete a;

		undoStack.clear();
	}

	void SetHistoryLimit(int limit)
	{
		m_historyLimit = limit;
		EnforceHistoryLimit();
	}

private:
	void EnforceHistoryLimit()
	{
		while (undoStack.size() > m_historyLimit)
		{
			delete undoStack.front();
			undoStack.erase(undoStack.begin());
		}
	}

	int m_historyLimit;
	std::vector<IAction*> undoStack;
	std::vector<IAction*> redoStack;
};
}