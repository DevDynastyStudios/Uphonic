#pragma once

#include "ProjectState.h"
#include "Plugin/PluginManager.h"

class ProjectManager
{
	public:
	static void NewProject();
	static void OpenProject(std::filesystem::path path);
};