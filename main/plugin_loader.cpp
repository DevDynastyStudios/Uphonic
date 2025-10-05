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

static VstIntPtr VSTCALLBACK audioMasterCallbackFunction(
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
}

static UphPluginInstance uph_load_vst2_internal(const char* path)
{
    typedef AEffect* (*VSTPluginMain)(audioMasterCallback audioMaster);
    
    UphLibrary vst_module = uph_load_library(path);
    if (!vst_module)
    {
        printf("Failed to load library: %s\n", path);
        return {0};
    }
    
    VSTPluginMain entry = (VSTPluginMain)uph_get_proc_address(vst_module, "VSTPluginMain");
    if (!entry)
        entry = (VSTPluginMain)uph_get_proc_address(vst_module, "main");
    
    if (!entry)
    {
        printf("Not a valid VST 2.x plugin.\n");
        uph_unload_library(vst_module);
        return {0};
    }
    
    AEffect* effect = entry(audioMasterCallbackFunction);
    if (!effect)
    {
        printf("VST plugin failed to instantiate.\n");
        uph_unload_library(vst_module);
        return {0};
    }
    
    effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, 44100.0f);
    effect->dispatcher(effect, effSetBlockSize, 0, 512, nullptr, 0);
    effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0);
    
    UphChildWindow child_window = {0};
    if (effect->flags & effFlagsHasEditor)
    {
        ERect* rect = nullptr;
        effect->dispatcher(effect, effEditGetRect, 0, 0, &rect, 0);
        uint32_t width  = rect ? rect->right - rect->left : 400;
        uint32_t height = rect ? rect->bottom - rect->top : 300;
        
        const UphChildWindowCreateInfo child_window_create_info = {width, height, "VstHost"};
        child_window = uph_create_child_window(&child_window_create_info);
        effect->dispatcher(effect, effEditOpen, 0, 0, child_window.handle, 0);
    }
    
    UphPluginInstance plugin = {0};
    plugin.library = vst_module;
    plugin.effect = effect;
    plugin.window = child_window;
    
    return plugin;
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
    plugin->effect = nullptr;
    plugin->library = nullptr;
}

static void uph_unload_vst2_internal(UphPluginInstance* plugin)
{
    if (!plugin || !plugin->effect) return;
    
    AEffect* effect = plugin->effect;
    
    effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
    
    if (effect->flags & effFlagsHasEditor)
    {
        effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);
    }
    
    effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
    effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
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