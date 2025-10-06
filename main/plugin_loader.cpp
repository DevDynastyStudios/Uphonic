#include "plugin_loader.h"
#include "platform/platform.h"
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>

struct UphPluginLoadRequest
{
    std::string path;
    uint32_t track_index;
};

static std::queue<UphPluginLoadRequest> queued_plugin_loads;
static std::queue<uint32_t> queued_plugin_unloads;

static UphInstrument uph_load_vst2_internal(const char* path)
{
    UviPlugin plugin = uvi_plugin_load(path);
    UphChildWindow child_window = {0};
    uint32_t width, height;
    plugin.get_editor_size(&plugin, &width, &height);
    
    const UphChildWindowCreateInfo child_window_create_info = {width, height, "VstHost"};
    child_window = uph_create_child_window(&child_window_create_info);
    plugin.open_editor(&plugin, child_window.handle);

    UphInstrument instrument;
    instrument.plugin = plugin;
    instrument.window = child_window;
    strncpy_s(instrument.path, path, sizeof(instrument.path));

    return instrument;
}

void uph_queue_instrument_load(const char *path, uint32_t track_index)
{
    if (!path) return;
    
    UphPluginLoadRequest request;
    request.path = path;
    request.track_index = track_index;
    queued_plugin_loads.push(request);
}

void uph_queue_instrument_unload(uint32_t track_index)
{
    queued_plugin_unloads.push(track_index);
    app->project.tracks[track_index].instrument.plugin.is_loaded = false;
}

void uph_process_instrument_loads(void)
{
    while (!queued_plugin_loads.empty())
    {
        UphPluginLoadRequest request = queued_plugin_loads.front();
        queued_plugin_loads.pop();
        
        UphInstrument &instrument = app->project.tracks[request.track_index].instrument;
        instrument = uph_load_vst2_internal(request.path.c_str());
    }
}

void uph_process_instrument_unloads(void)
{
    while (!queued_plugin_unloads.empty())
    {
        uint32_t track_index = queued_plugin_unloads.front();
        queued_plugin_unloads.pop();

        UphInstrument &instrument = app->project.tracks[track_index].instrument;
        uvi_plugin_unload(&instrument.plugin);
        
        if (instrument.window.handle)
            uph_destroy_child_window(&instrument.window);
    }
}

void uph_process_plugin_loader(void)
{
    uph_process_instrument_unloads();
    uph_process_instrument_loads();
}