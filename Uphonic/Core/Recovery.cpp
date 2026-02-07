#include "Recovery.h"
#include "ProjectManager.h"
#include "Naui/FileSystem/File.h"
#include <iostream>

void Recovery::Recover(const std::filesystem::path& projectDir)
{
	ProjectManager::LoadFromWorkspace(projectDir);
}

void Recovery::Discard(const std::filesystem::path& projectDir)
{
	std::filesystem::remove_all(projectDir);
}

std::optional<std::filesystem::path> Recovery::TryGetLastSession()
{
	std::filesystem::path workspace = Naui::Directory::WorkspaceDirectory();
	std::vector<std::filesystem::path> folders;

	std::string currentID = ProjectState::GetInstance().settings.projectID.Str();
	std::filesystem::path currentFolder = std::filesystem::weakly_canonical(workspace / currentID);

	for(auto& entry : std::filesystem::directory_iterator(workspace))
	{
		if(!entry.is_directory())
			continue;

		std::filesystem::path folder = std::filesystem::weakly_canonical(entry.path());
		if(folder == currentFolder)
			continue;

		if(!std::filesystem::exists(folder / "project.json") || Naui::Directory::IsLocked(folder))
			continue;

		folders.push_back(folder);
	}

	if(folders.size() > 0)
		return folders.front();

	return std::nullopt;
}