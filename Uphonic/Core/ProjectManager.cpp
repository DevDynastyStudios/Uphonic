#include "ProjectManager.h"
#include "ProjectSerializer.h"
#include "Recovery.h"
#include "Audio/AudioEngine.h"
#include "Naui/Platform/Window.h"
#include "Naui/FileSystem/File.h"
#include "Naui/FileSystem/FileDialog.h"
#include "Naui/FileSystem/Archive.h"
#include "UI/RecoveryPrompt.h"
#include "UI/SongTimeline.h"
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
	ProjectState& state = ProjectState::GetInstance();
	std::filesystem::path workspace = Naui::Directory::WorkspaceDirectory();
	std::filesystem::path snapshot = workspace / ("temp_" + Naui::UUID().Str());
	std::filesystem::create_directories(snapshot);

	Naui::Archive archive(uphPath, Naui::ArchiveMode::Read);
	if(!archive.IsValid())
	{
		std::cout << "Failed to open archive\n";
		std::filesystem::remove_all(snapshot);
		return false;
	}

	if(!archive.ExtractTo(snapshot))
	{
		std::cout << "Failed to extract archive\n";
		std::filesystem::remove_all(snapshot);
		return false;
	}

	ProjectState tempState;
	if(!ProjectSerializer::LoadProject(tempState, snapshot))
	{
		std::cout << "Failed to load " << uphPath.filename() << "\n";
		std::filesystem::remove_all(snapshot);
		return false;
	}
	
	Naui::UUID projectID = state.settings.projectID;
	ProjectManager::ShutdownWorkspace(workspace / state.settings.projectID.Str());
	std::filesystem::path projectFolder = workspace / state.settings.projectID.Str();
	std::filesystem::rename(snapshot, projectFolder);
	
	state.ClearProject();
	state.CopyFrom(tempState);
	state.settings.projectID = projectID;
	std::cout << "Opened Project: "  << uphPath << "\n";
	return true;
}

bool ProjectManager::Save()
{
	ProjectState& state = ProjectState::GetInstance();
	if(state.settings.saveToPath.has_value())
		return SaveProject(state.settings.saveToPath.value());

	FileDialog::SaveFile("save_project", "Save Project", ".uph");
	return true;	// Technically misleading if the dialog cancels mid-way through
}

bool ProjectManager::SaveProject(std::filesystem::path path)
{
	if(!path.has_filename())
	{
		std::cout << "Unable to save project. No filename was given\n";
		return false;
	}

	std::string fileName = path.stem().string();
	std::string extension = path.extension().string();
	if(extension != ".uph")
		extension = ".uph";

	std::filesystem::path saveToPath = path.parent_path() / (fileName + extension);
	
	ProjectState& state = ProjectState::GetInstance();
	state.settings.projectName = fileName;
	state.settings.saveToPath = saveToPath;

	std::filesystem::path workspace = Naui::Directory::WorkspaceDirectory();
	std::filesystem::path projectFolder = workspace / state.settings.projectID.Str();
	std::string snapshotFolderName = "temp_" + state.settings.projectID.Str();
	std::filesystem::path snapshot  = workspace / snapshotFolderName;
	std::filesystem::remove_all(snapshot);
	std::filesystem::create_directories(snapshot);

	if(!ProjectSerializer::SaveProject(state, projectFolder))
		return false;

	Naui::Directory::UnlockPath(projectFolder);
	try
	{
		std::filesystem::copy(projectFolder, snapshot, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	}
	catch (const std::exception& e)
	{
		std::cout << "Snapshot copy failed: " << e.what() << "\n";
		std::filesystem::remove_all(snapshot);
		return false;
	}

	Naui::Archive archive(saveToPath, Naui::ArchiveMode::Write);
	if(!archive.IsValid())
	{
		std::cout << "Failed to create archive\n";
		std::filesystem::remove_all(snapshot);
		return false;
	}

	if(!archive.AddFolder(snapshot, ""))
	{
		std::cout << "Failed to add folder to archive\n";
		std::filesystem::remove_all(snapshot);
		return false;
	}

	Naui::Directory::LockPath(projectFolder);
	std::filesystem::remove_all(snapshot);
	std::cout << "Project saved to: " << saveToPath << "\n";
	return true;
}

void ProjectManager::CloseProject()
{
	std::filesystem::path projectPath = std::filesystem::weakly_canonical(Naui::Directory::WorkspaceDirectory() / ProjectState::GetInstance().settings.projectID.Str());
	std::cout << "Shutdown request for project: " << ProjectState::GetInstance().settings.projectName << "\n";
	ProjectManager::ShutdownWorkspace(projectPath);
	ProjectState::GetInstance().mainWindow->Close();
	std::cout << "Uphonic Successfully Closed\n";
}

bool ProjectManager::LoadFromWorkspace(const std::filesystem::path& folder)
{
	std::filesystem::path canonicalFolder = std::filesystem::weakly_canonical(folder);
	ProjectState& state = ProjectState::GetInstance();
	if(!std::filesystem::exists(canonicalFolder / "project.json"))
	{
		std::cout << "Failed to find project file.\n";
		return false;
	}
	
	if(Naui::Directory::IsLocked(canonicalFolder))
	{
		std::cout << "Path currently in use by another project instance: " << canonicalFolder << '\n';
		return false;	
	}

	Naui::UUID projectID = state.settings.projectID;
	std::filesystem::path currentWorkspace = Naui::Directory::WorkspaceDirectory() / projectID.Str();
	ProjectState tempState;
	if(!ProjectSerializer::LoadProject(tempState, canonicalFolder))
	{
		std::cout << "Failed to load project\n";
		return false;
	}

	Naui::Directory::UnlockPath(currentWorkspace);
	std::filesystem::remove_all(currentWorkspace);
	ProjectState::ClearProject();
	state.CopyFrom(tempState);
	std::filesystem::rename(canonicalFolder, currentWorkspace);
	Naui::Directory::LockPath(currentWorkspace);
	state.settings.projectID = projectID;
	std::cout << "Loaded project: " << canonicalFolder << "\n";
	return true;
}

void ProjectManager::ImportSample(const std::filesystem::path& source)	// (Chimpchi): Change this to rename samples with duplicate names
{
	ProjectState& state = ProjectState::GetInstance();
	std::filesystem::path sampleFolder = Naui::Directory::WorkspaceDirectory() / state.settings.projectID.Str() /  "Samples";
	std::filesystem::path dest = sampleFolder / source.filename();

	try {
		std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing);
	}
	catch (const std::exception& e) {
		std::cout << "Failed to import sample: " << e.what() << "\n";
	}

	AudioSample& sample = AudioEngine::AddSample(dest.string().c_str());
	if(sample.IsValid())
		return;

	try
	{
		std::filesystem::remove_all(dest);
	}
	catch(const std::exception& e)
	{
		std::cerr << "Failed to clean up sample data. " << e.what() << '\n';
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
	std::filesystem::path filePath = Naui::Directory::WorkspaceDirectory() / ProjectState::GetInstance().settings.projectID.Str() / "Samples" / sample.filename;
	AudioEngine::UnloadSample(sample);
	if(filePath.empty() || !std::filesystem::exists(filePath))
		return;

	try
	{
		std::filesystem::remove(filePath);
	}
	catch(const std::exception& e)
	{
		std::cout << "Failed to delete sample file: " << e.what() << "\n";
	}
	
	state.samples.erase(state.samples.begin() + index);
}

void ProjectManager::InitializeWorkspace(bool initialLoad, bool clearDir)
{
	ProjectState& state = ProjectState::GetInstance();
	ProjectManager::ShutdownWorkspace(Naui::Directory::WorkspaceDirectory() / state.settings.projectID.Str());
	state.settings.projectID = Naui::UUID();

	std::filesystem::path workspacePath = Naui::Directory::WorkspaceDirectory();
	std::filesystem::path workspace = Naui::Directory::HideDirectory(workspacePath, true);
	std::filesystem::path projectFolder = std::filesystem::weakly_canonical(workspace / state.settings.projectID.Str());
	auto recoveredPath = Recovery::TryGetLastSession();

	RecoveryPrompt* recover = Naui::GetPanelOfType<RecoveryPrompt>();	// (Chimpchi): This will change in the future, I plan to add Modal's to the Naui engine.
	if(recover == nullptr)
	{
		std::cout << "Unable to retrieve Recovery Prompt. Uphonic will be unable to recover projects.\n";
	}
	else if(recoveredPath.has_value())
	{
		recover->recoverPath = recoveredPath;
		recover->SetOpen(true);
	} else
		std::cout << "No recovery projects found.\n";

	if(clearDir)
		std::filesystem::remove_all(projectFolder);
	
	std::filesystem::create_directories(projectFolder);
	state.settings.projectName = "Untitled";
	ProjectSerializer::SaveProject(state, projectFolder);
	Naui::Directory::LockPath(projectFolder);
	std::cout << "Workspace initialized at: " << projectFolder << "\n";
}

void ProjectManager::ShutdownWorkspace(std::filesystem::path path)
{
	if(path.empty())
		return;

	std::cout << "Closing workspace:" << path << "...\n";
	if(std::filesystem::exists(path))
	{
		try
		{
			Naui::Directory::UnlockPath(path);
			std::filesystem::remove_all(path);
		}
		catch(const std::exception& e)
		{
			std::cout << "Failed to remove project workspace: " << e.what() << '\n';
		}
	}

	std::string uuid = path.filename().string();
	std::filesystem::path snapshotFolder = path.parent_path() / ("temp_" + uuid);
	if(std::filesystem::exists(snapshotFolder))
	{
		try
		{
			std::filesystem::remove_all(snapshotFolder);
		}
		catch(const std::exception& e)
		{
			std::cout << "Failed to remove temp directories: " << e.what() << '\n';
		}
	}
}
