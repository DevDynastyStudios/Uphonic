#pragma once

#include "ProjectState.h"
#include "Plugin/PluginManager.h"

class ProjectManager
{
public:
	static void NewProject(bool initialLoad = false);
	static bool OpenProject(const std::filesystem::path& path);
	static bool Save();
	static bool SaveProject(std::filesystem::path path);
	static void CloseProject();
	static bool LoadFromWorkspace(const std::filesystem::path& folder);

	static void ImportSample(const std::filesystem::path& source);
	static void DeleteSample(AudioSample& sample);
	static void DeleteSample(size_t index);
	static bool RenameSample(AudioSample& sample, const std::string& newName);
	static bool RenameSample(size_t index, const std::string& newName);
	
private:
	static void InitializeWorkspace(bool initialLoad = false, bool clearDir = false);
	static void ShutdownWorkspace(std::filesystem::path path);
};