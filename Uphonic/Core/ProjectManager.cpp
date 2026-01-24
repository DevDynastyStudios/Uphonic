#include "ProjectManager.h"
#include "ProjectSerializer.h"
#include "UI/SongTimeline.h"
#include "FileDialog.h"
#include "Audio/AudioEngine.h"
#include "Naui/FileSystem/File.h"
#include "Naui/FileSystem/Archive.h"
#include <iostream>

void ProjectManager::NewProject(bool initialLoad)
{
	PluginManager::UnloadAllEffects();
	ProjectState::ClearProject();
	ProjectState& state = ProjectState::GetInstance();
	SongTimeline::CreateTrack("Track 1");
	state.patterns.push_back(MidiPattern("Pattern 1"));
	InitializeWorkspace(initialLoad);
}

bool ProjectManager::OpenProject(const std::filesystem::path& uphPath)
{
	std::filesystem::path base = Naui::Directory::WorkspaceDirectory();
	std::filesystem::path workspace = base / "current_project";
	std::filesystem::path tempFolder = base / "temp";
	std::filesystem::remove_all(tempFolder);
	std::filesystem::create_directories(tempFolder);

	Naui::Archive archive(uphPath, Naui::ArchiveMode::Read);
	if(!archive.IsValid())
	{
		std::cout << "Failed to open archive\n";
		std::filesystem::remove_all(tempFolder);
		return false;
	}

	if(!archive.ExtractTo(tempFolder))
	{
		std::cout << "Failed to extract archive\n";
		std::filesystem::remove_all(tempFolder);
		return false;
	}

	//ProjectState tempState;
	if(!ProjectSerializer::Load(ProjectState::GetInstance(), tempFolder))
	{
		std::cout << "Failed to load project.json\n";
		std::filesystem::remove_all(tempFolder);
		return false;
	}
	
	std::filesystem::remove_all(workspace);
	std::filesystem::rename(tempFolder, workspace);

	// for(AudioSample& sample : tempState.samples)
	// {
	// 	AudioEngine::AddSample((workspace / "Samples" / sample.name).string().c_str());
	// }

	//ProjectState::GetInstance().CopyFrom(tempState);
	std::cout << "Opened Project: "  << uphPath << "\n";
	return true;
}

bool ProjectManager::Save()
{
	std::filesystem::path workspace = Naui::Directory::WorkspaceDirectory() / "current_project";
	std::filesystem::create_directories(workspace);
	return ProjectSerializer::Save(workspace);
}

bool ProjectManager::SaveProject(std::filesystem::path path, std::string fileName)
{
	std::filesystem::path base = Naui::Directory::WorkspaceDirectory();
	std::filesystem::path workspace = base / "current_project";
	std::filesystem::path snapshot  = base / "save_snapshot_tmp";

	std::filesystem::remove_all(snapshot);
	std::filesystem::create_directories(snapshot);

	if(!ProjectSerializer::Save(workspace))
		return false;

	try
	{
		std::filesystem::copy(workspace, snapshot, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	}
	catch (const std::exception& e)
	{
		std::cout << "Snapshot copy failed: " << e.what() << "\n";
		return false;
	}

	std::filesystem::path archivePath = path / (fileName + ".uph");
	Naui::Archive archive(archivePath, Naui::ArchiveMode::Write);

	if(!archive.IsValid())
	{
		std::cout << "Failed to create archive\n";
		return false;
	}

	if(!archive.AddFolder(snapshot, ""))
	{
		std::cout << "Failed to add folder to archive\n";
		return false;
	}

	std::filesystem::remove_all(snapshot);
	std::cout << "Project saved to: " << archivePath << "\n";
	return true;
}

void ProjectManager::InitializeWorkspace(bool initialLoad, bool clearDir)
{
	std::filesystem::path base = Naui::Directory::WorkspaceDirectory();
	std::filesystem::path projectFolder = base / "current_project";

	// (Chimpchi): Change this to where it looks in a history file to get the last used directory, also make project names unique.
	std::filesystem::create_directories(projectFolder);
	if(clearDir)
		std::filesystem::remove_all(projectFolder);

	std::filesystem::create_directories(projectFolder / "Samples");
	std::filesystem::create_directories(projectFolder / "PluginData");
	std::filesystem::create_directories(projectFolder / "Cache");
	std::cout << "Workspace initialized at: " << projectFolder << "\n";
}

void ProjectManager::ImportSample(const std::filesystem::path& source)
{
	AudioSample& sample = AudioEngine::AddSample(source.string().c_str());
	if(!sample.IsValid())
		return;

	std::filesystem::path base = Naui::Directory::WorkspaceDirectory() / "current_project/Samples";
	std::filesystem::path dest = base / source.filename();

	try {
		std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing);
		sample.filePath = dest;
	}
	catch (const std::exception& e) {
		std::cout << "Failed to import sample: " << e.what() << "\n";
	}
}

void ProjectManager::DeleteSample(AudioSample& sample)
{
	ProjectState& state = ProjectState::GetInstance();
	size_t index = &sample - state.samples.data();
	if(index > state.samples.size())
		return;

	ProjectManager::DeleteSample(index);
}

void ProjectManager::DeleteSample(size_t index)
{
	ProjectState& state = ProjectState::GetInstance();
	if(index >= state.samples.size())
		return;

	AudioSample& sample = state.samples[index];
	AudioEngine::UnloadSample(sample);
	if(sample.filePath.empty() || !std::filesystem::exists(sample.filePath))
		return;

	try
	{
		std::filesystem::remove(sample.filePath);
	}
	catch(const std::exception& e)
	{
		std::cout << "Failed to delete sample file: " << e.what() << "\n";
	}
	
	state.samples.erase(state.samples.begin() + index);
}

bool ProjectManager::RenameSample(AudioSample& sample, const std::string& newName)
{
	ProjectState& state = ProjectState::GetInstance();
	size_t index = &sample - state.samples.data();
	if(index > state.samples.size())
		return false;

	return ProjectManager::RenameSample(index, newName);
}

bool ProjectManager::RenameSample(size_t index, const std::string& newName)
{
	ProjectState& state = ProjectState::GetInstance();
	if(index >= state.samples.size())
		return false;

	AudioSample& sample = state.samples[index];
	std::filesystem::path newPath = sample.filePath.parent_path() / newName;

	if(newPath.extension().empty())
		newPath.replace_extension(sample.filePath.extension());

	try
	{
		std::filesystem::rename(sample.filePath, newPath);
	}
	catch(const std::exception& e)
	{
		std::cout << "Failed to rename sample: " << e.what() << "\n";
		return false;
	}
	
	sample.filePath = newPath;
	sample.name = newPath.filename().string();
	return true;
}