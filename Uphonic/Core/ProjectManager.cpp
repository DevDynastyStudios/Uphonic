#include "ProjectManager.h"
#include "FileDialog.h"
#include <iostream>

void ProjectManager::NewProject()
{
	ProjectState& state = ProjectState::GetInstance();
	PluginManager::UnloadAllEffects();
	state.samples.clear();
	state.patterns.clear();
	state.tracks.clear();
	state.currentMidiPatternIndex = 0;
	state.timelinePositionBeats = 0;
}

void ProjectManager::OpenProject(std::filesystem::path path)
{
	std::cout << "Opening Project: "  << path << "\n";
}