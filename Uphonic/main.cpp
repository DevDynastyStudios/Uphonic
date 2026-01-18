#include "Naui.h"
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
#include "UI/FileExplorer.h"

#include <iostream>

using namespace ImGui;

class UphonicApp : public Naui::App
{
private:
	void OnEnter() override
	{
		Layout::LoadDefault();
		ProjectState& state = ProjectState::GetInstance();
		state.mainWindow = GetPlatformWindow();
		
		state.patterns.push_back(MidiPattern("Pattern 1"));
		
		AudioConfig audioConfig;
		audioConfig.sampleRate = state.settings.audioSampleRate;
		if (!AudioEngine::Initialize(audioConfig))
		{
			std::cerr << "Failed to initialize audio engine\n";
		}
		
		Naui::AddPanel<MidiEditor>();
		Naui::AddPanel<PatternRack>();
		Naui::AddPanel<SampleRack>();
		Naui::AddPanel<SongTimeline>();
		Naui::AddPanel<MixerRack>();
	}
	
	void OnExit() override
	{
		AudioEngine::Shutdown();
	}
	
	void OnFileDrop(const char* path) override
	{
		ProjectState& state = ProjectState::GetInstance();
		state.samples.push_back(AudioEngine::LoadSample(path));
	}
	
	void OnRender() override
	{
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
			if (effect.window)
				effect.plugin->IdleEditor();
		}
		for (AudioTrack& track : state.tracks)
		{
			for (PluginEffect& effect : track.effects)
			{
				if (effect.window)
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
