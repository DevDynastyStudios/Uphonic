#include "RemoveSampleAction.h"
#include "Actions/ActionManager.h"
#include "Core/ProjectManager.h"

RemoveSampleAction::RemoveSampleAction(AudioSample& sample) : sample(sample) {}

void RemoveSampleAction::Do()
{ 
	ProjectManager::DeleteSample(sample);		// Change the backend to not automatically sync sample to file lifetime
	ProjectState::GetInstance().actionManager.ClearHistory();
}

void RemoveSampleAction::Undo()
{
}