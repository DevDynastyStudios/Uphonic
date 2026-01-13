#include "PluginManager.h"
#include "../Core/ProjectState.h"

void PluginManager::LoadEffect(PluginEffect& effect, const std::filesystem::path& path)
{
    ProjectState& state = ProjectState::GetInstance();
    Uvi::Plugin* plugin = Uvi::PluginLoader::Load(path.string().c_str(), state.settings.audioSampleRate);

    uint32_t width, height;
    plugin->GetEditorSize(&width, &height);
    effect.window = Naui::CreatePlatformWindow(width, height, path.filename().replace_extension().string().c_str(), state.mainWindow);

    plugin->OpenEditor(effect.window->GetNativeHandle());

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
    Uvi::Plugin* plugin = effect.plugin;
    effect.plugin = nullptr;

    delete plugin;
    delete effect.window;
}

void PluginManager::UnloadAllEffects()
{
	ProjectState& state = ProjectState::GetInstance();

	for(PluginEffect plugin : state.masterTrack.effects)
	{
		UnloadEffect(plugin);
	}

	for(AudioTrack track : state.tracks)
	{
		for(PluginEffect plugin : track.effects)
		{
			UnloadEffect(plugin);
		}
	}
}
