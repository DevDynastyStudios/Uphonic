#include "Naui.h"
#include "Naui/FileSystem/File.h"
#include "Naui/FileSystem/FileDialog.h"
#include "Core/ProjectManager.h"
#include "Core/ProjectState.h"
#include "Audio/AudioEngine.h"
#include "Plugin/PluginManager.h"
#include "Config/EditorConfig.h"

#include "UI/Layout.h"
#include "UI/MainMenuBar.h"
#include "UI/MidiEditor.h"
#include "UI/PatternRack.h"
#include "UI/SampleRack.h"
#include "UI/SongTimeline.h"
#include "UI/MixerRack.h"
#include "UI/RecoveryPrompt.h"
#include "UI/FileExplorer.h"


#include <cstdio>
#include <iostream>

using namespace ImGui;

class UphonicApp : public Naui::App
{
private:
	void OnEnter() override
	{
		Naui::Directory::SetWorkspaceDirectory(Naui::Directory::AppDataDirectory() / "Uphonic/.workspace", true);
		Naui::Localization::SetLanguage("en-US");	// (Chimpchi): This is temporary

		Naui::RegisterPanel<MidiEditor>();
		Naui::RegisterPanel<PatternRack>();
		Naui::RegisterPanel<SampleRack>();
		Naui::RegisterPanel<SongTimeline>();
		Naui::RegisterPanel<MixerRack>();
		Naui::RegisterPanel<RecoveryPrompt>();

		Layout::LoadDefault();

		ProjectState& state = ProjectState::GetInstance();
		state.mainWindow = GetPlatformWindow();
		
		AudioConfig audioConfig;
		audioConfig.sampleRate = state.settings.audioSampleRate;
		if (!AudioEngine::Initialize(audioConfig))
		{
			std::cerr << "Failed to initialize audio engine\n";
		}

		ProjectManager::NewProject(true);
	}
	
	void OnExit() override
	{
		AudioEngine::Shutdown();
		ProjectManager::CloseProject();
	}
	
	void OnFileDrop(const char* path) override
	{
		ProjectManager::ImportSample(path);
	}
	
	void OnRender() override
	{
		FileDialog::Display("open_project", [](const std::filesystem::path& path)
		{
			ProjectManager::OpenProject(path);
		});

		FileDialog::Display("save_project", [](const std::filesystem::path& path)
		{
			ProjectManager::SaveProject(path);
		});

		FileDialog::Display("export_project", [](const std::filesystem::path& path)
		{
			if(path.extension() == ".wav")
				AudioEngine::ExportToWav(path.string().c_str(), 0, SongTimeline::GetTimelineDuration());
		});

		if(ImGui::BeginMainMenuBar())
		{	
			MainMenuBar::FileMenu();
			MainMenuBar::EditMenu();
			MainMenuBar::ViewMenu();
			MainMenuBar::LayoutMenu();
			MainMenuBar::HelpMenu();
			ImGui::EndMainMenuBar();
		}

		MainMenuBar::RenderPopups();

		ProjectState& state = ProjectState::GetInstance();
		for (PluginEffect& effect : state.masterTrack.effects)
		{
			if (effect.plugin && effect.window)
				effect.plugin->IdleEditor();
		}
		for (AudioTrack& track : state.tracks)
		{
			if (track.instrument.plugin && track.instrument.window)
				track.instrument.plugin->IdleEditor();
			for (PluginEffect& effect : track.effects)
			{
				if (effect.plugin && effect.window)
					effect.plugin->IdleEditor();
			}
		}
	}
};

int main()
{
	UphonicApp app;
	app.Run("Uphonic");
	return 0;
}