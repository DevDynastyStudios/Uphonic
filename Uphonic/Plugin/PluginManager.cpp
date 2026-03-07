#include "PluginManager.h"
#include "Core/ProjectState.h"
#include "Naui/Platform/Window.h"

void PluginManager::LoadEffect(PluginEffect& effect, const std::filesystem::path& path)
{
    ProjectState& state = ProjectState::GetInstance();
    Uvi::Plugin* plugin = Uvi::PluginLoader::Load(path.string().c_str(), state.settings.audioSettings.sampleRate, 512);

    effect.window = Naui::CreatePlatformWindow(1, 1, path.filename().replace_extension().string().c_str(),
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

    // Step 1 – explicitly detach the editor while the platform window is
    // still alive. The plugin may access the native handle during teardown
    // (e.g. to destroy its embedded child window), so it must happen before
    // we delete the window.
    effect.plugin->DetachEditor();

    // Step 2 – destroy the platform window now that the plugin's embedded
    // UI has been cleanly removed.
    if (effect.window)
    {
        delete effect.window;
        effect.window = nullptr;
    }

    // Step 3 – delete the plugin. The destructor will skip CleanupEditor
    // (already done above) and proceed with audio/component teardown.
    Uvi::Plugin* plugin = effect.plugin;
    effect.plugin = nullptr;
    delete plugin;
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