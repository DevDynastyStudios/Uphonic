#include "PluginLoader.h"

void PluginLoader::LoadEffect(Effect &effect, const std::filesystem::path &path)
{
    Uvi::Plugin *plugin = Uvi::PluginLoader::Load(path.string().c_str(), Core::settings.sampleRate);

    uint32_t width, height;
    plugin->GetEditorSize(&width, &height);
    effect.window = Naui::CreatePlatformWindow(width, height, path.filename().replace_extension().string().c_str(), Core::mainWindow);

    plugin->OpenEditor(effect.window->GetNativeHandle());

    effect.plugin = plugin;
    effect.window->Show(true);
}

void PluginLoader::OpenEffect(Effect &effect)
{
    if (!effect.window)
        return;
    effect.window->Show(true);
}

void PluginLoader::UnloadEffect(Effect &effect)
{
    Uvi::Plugin *plugin = effect.plugin;
    effect.plugin = nullptr;

    delete plugin;
    delete effect.window;
}