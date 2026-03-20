#include "SaveProjectAction.h"
#include "Core/ProjectManager.h"
#include "Core/ProjectState.h"
#include "Naui/FileSystem/FileDialog.h"
#include <string>

SaveProjectAction::SaveProjectAction(ProjectState& state, bool forceDialogSave) : state(state), forceDialogSave(forceDialogSave) {}

void SaveProjectAction::Do()
{
	if(forceDialogSave)
	{
		FileDialog::SaveFile("save_project", "Save Project", ".uph");
		return;
	}

	std::string projectName = state.settings.projectName;
	if(projectName.empty())
	{
		// Prompt name project window here
	}

	ProjectManager::Save();
}

void SaveProjectAction::Undo() {}