#include "SaveProjectAction.h"
#include "Core/ProjectManager.h"
#include "Core/ProjectState.h"
#include <string>

SaveProjectAction::SaveProjectAction(ProjectState& state) : state(state) {}

void SaveProjectAction::Do()
{
	std::string projectName = state.settings.projectName;
	if(projectName.empty())
	{
		// Prompt name project window here
	}

	ProjectManager::Save();
}

void SaveProjectAction::Undo() {}