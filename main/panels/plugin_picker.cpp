#include "panel_manager.h"
#include "plugin_loader.h"
#include "types.h"

#include <string>
#include <filesystem>
#include <unordered_set>
#include <set>

struct UphPluginPicker
{
    std::set<std::filesystem::path> paths;
};

static UphPluginPicker plugin_picker;

static void uph_plugin_picker_init(UphPanel* panel)
{
    panel->panel_flags |= UphPanelFlags_HiddenFromMenu;
	panel->window_flags = ImGuiWindowFlags_NoSavedSettings;

    const std::unordered_set<std::filesystem::path> default_directories = {
        "C:\\Program Files\\Steinberg\\VstPlugins",
        "C:\\Program Files\\VstPlugins"
    };

    for (const auto& directory : default_directories)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (!entry.is_regular_file())
                continue;
            
            std::filesystem::path path = entry.path();
            std::filesystem::path extension = path.extension();

            if (extension == ".dll" || extension == ".so" || extension == ".dylib" ||
                extension == ".vst3" || extension == ".vst2" || extension == ".vst")
                plugin_picker.paths.insert(entry.path());
        }
    }
}

static void uph_plugin_picker_render(UphPanel* panel)
{
    for (const auto& path : plugin_picker.paths)
    {
        if (ImGui::Selectable(path.filename().string().c_str()))
        {
            UphInstrument *instrument = &app->project.tracks[app->current_instrument_track_index].instrument;
            if (instrument->plugin.effect)
                uph_queue_plugin_unload(&instrument->plugin);
            uph_queue_plugin_load(path.string().c_str(), &app->project.tracks[app->current_instrument_track_index]);
            panel->is_visible = false;
        }
    }

    if (!ImGui::IsAnyItemHovered() && ImGui::IsAnyMouseDown() || ImGui::IsKeyPressed(ImGuiKey_Escape))
        panel->is_visible = false;
}

UPH_REGISTER_PANEL("Select Plugin", UphPanelFlags_Modal, uph_plugin_picker_render, uph_plugin_picker_init);