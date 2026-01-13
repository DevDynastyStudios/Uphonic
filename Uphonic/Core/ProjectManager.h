#pragma once

#include "ProjectState.h"
#include "Plugin/PluginManager.h"

class ProjectManager
{
	public:
	static void NewProject()
	{
    	ProjectState& state = ProjectState::GetInstance();
		PluginManager::UnloadAllEffects();
		state.samples.clear();
		state.patterns.clear();
		state.tracks.clear();
		state.currentMidiPatternIndex = 0;
		state.timelinePositionBeats = 0;
	}

	static void OpenProject()
	{

	}
};