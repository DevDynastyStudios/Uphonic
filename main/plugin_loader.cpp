#include "plugin_loader.h"
#include "platform/platform.h"
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>

struct UphPluginLoadRequest
{
    std::string path;
    UphTrack* track;
};

static std::queue<UphPluginLoadRequest> queued_plugin_loads;
static std::queue<UphPluginInstance> queued_plugin_unloads;

/*static VstIntPtr VSTCALLBACK audioMasterCallbackFunction(
    AEffect* effect, VstInt32 opcode, VstInt32 index,
    VstIntPtr value, void* ptr, float opt
)
{
    switch (opcode)
    {
        case audioMasterVersion: return 2400;
        case audioMasterIdle:    return 0;
        case audioMasterGetSampleRate: return (VstIntPtr)44100;
        case audioMasterGetBlockSize:  return (VstIntPtr)512;
        case audioMasterCanDo:
        {
            const char* canDo = (const char*)ptr;
            if (!canDo) return 0;
            if (strcmp(canDo, "sendVstEvents") == 0) return 1;
            if (strcmp(canDo, "sendVstMidiEvent") == 0) return 1;
            if (strcmp(canDo, "receiveVstEvents") == 0) return 1;
            if (strcmp(canDo, "receiveVstMidiEvent") == 0) return 1;
            return 0;
        }
        case audioMasterGetTime:
        {
            VstTimeInfo* timeInfo = (VstTimeInfo*)ptr;
            if (!timeInfo) return 0;
            timeInfo->samplePos = 0.0;
            timeInfo->tempo = app->project.bpm;
            timeInfo->flags = kVstTempoValid;
            return (VstIntPtr)timeInfo;
        }
        default: return 0;
    }
}*/

static UphPluginInstance uph_load_vst2_internal(const char* path)
{
    UviPlugin plugin = uvi_plugin_load(path);
    UphChildWindow child_window = {0};
    uint32_t width = 0, height = 0;
    plugin.get_editor_size(&plugin, &width, &height);

    const UphChildWindowCreateInfo child_window_create_info = {width, height, plugin.name};
    child_window = uph_create_child_window(&child_window_create_info);
    plugin.open_editor(&plugin, child_window.handle);
    
    UphPluginInstance instance = {0};
    instance.handle = plugin;
    instance.window = child_window;
    
    return instance;
}

void uph_queue_plugin_load(const char *path, UphTrack *track)
{
    if (!path || !track) return;
    
    UphPluginLoadRequest request;
    request.path = path;
    request.track = track;
    queued_plugin_loads.push(request);
}

void uph_queue_plugin_unload(UphPluginInstance* plugin)
{
    if (!plugin) return;
    queued_plugin_unloads.push(*plugin);
    plugin->handle.is_loaded = false;
}

static void uph_unload_vst2_internal(UphPluginInstance* plugin)
{
    uvi_plugin_unload(&plugin->handle);
}

void uph_process_plugin_loads(void)
{
    while (!queued_plugin_loads.empty())
    {
        UphPluginLoadRequest request = queued_plugin_loads.front();
        queued_plugin_loads.pop();
        
        UphPluginInstance plugin = request.track->instrument.plugin;
        request.track->instrument.plugin = uph_load_vst2_internal(request.path.c_str());
    }
}

void uph_process_plugin_unloads(void)
{
    while (!queued_plugin_unloads.empty())
    {
        UphPluginInstance plugin = queued_plugin_unloads.front();
        queued_plugin_unloads.pop();
            
        uph_unload_vst2_internal(&plugin);
        
        if (plugin.window.handle)
            uph_destroy_child_window(&plugin.window);
    }
}

void uph_process_plugins(void)
{
    uph_process_plugin_unloads();
    uph_process_plugin_loads();
}