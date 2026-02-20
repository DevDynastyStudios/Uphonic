#include "PluginManager.h"
#include "Core/ProjectState.h"
#include "Naui/Platform/Window.h"

void PluginManager::LoadEffect(PluginEffect& effect, const std::filesystem::path& path)
{
    ProjectState& state = ProjectState::GetInstance();
    Uvi::Plugin* plugin = Uvi::PluginLoader::Load(path.string().c_str(), state.settings.audioSampleRate, 512);

    effect.window = Naui::CreatePlatformWindow(0, 0, path.filename().replace_extension().string().c_str(),
        state.mainWindow);

    plugin->AttachEditor(effect.window->GetNativeHandle());

    effect.plugin = plugin;
    effect.pluginPath = path.string();
    effect.window->Show(true);
}

void PluginManager::OpenEffect(PluginEffect& effect)
{
    if (!effect.window)
        return;
    effect.window->Show(true);
}

void PluginManager::UnloadEffect(PluginEffect& effect)
{
    if (!effect.plugin)
        return;

    Uvi::Plugin *plugin = effect.plugin;
    effect.plugin = nullptr;
    delete plugin;

    if (effect.window)
    {
        delete effect.window;
        effect.window = nullptr;
    }
}

void PluginManager::UnloadAllEffects()
{
	ProjectState& state = ProjectState::GetInstance();

	for(PluginEffect& plugin : state.masterTrack.effects)
		UnloadEffect(plugin);

	for(AudioTrack& track : state.tracks)
	{
        if (track.instrument.plugin)
            UnloadEffect(track.instrument);
		for(PluginEffect& plugin : track.effects)
		{
			UnloadEffect(plugin);
		}
	}
}