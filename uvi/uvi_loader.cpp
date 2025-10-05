#include "uvi_loader.h"
#include <filesystem>

#if defined(_WIN32)
static UviLibrary uvi_library_load(const char *path)
{
    return LoadLibraryA(path);
}

static void uvi_library_unload(UviLibrary library)
{
    FreeLibrary((HMODULE)library);
}

static UviProcAddress uvi_get_proc_address(UviLibrary library, const char *name)
{
    return (UviProcAddress)GetProcAddress((HMODULE)library, name);
}
#elif defined(__linux__)
static UviLibrary uvi_library_load(const char *path)
{
    return dlopen(path, RTLD_LAZY);
}

static void uvi_library_unload(UviLibrary library)
{
    dlclose(library);
}

static UviProcAddress uvi_get_proc_address(UviLibrary library, const char *name)
{
    return (UviProcAddress)dlsym((void*)library, name);
}
#endif

enum UviV2AudioMasterOpcodes
{
	UviV2AudioMasterOpcodes_Automate = 0,
	UviV2AudioMasterOpcodes_Version,
	UviV2AudioMasterOpcodes_CurrentId,
	UviV2AudioMasterOpcodes_Idle,
	UviV2AudioMasterOpcodes_PinConnected
};

static intptr_t uvi_v2_audio_master_callback_function(
    UviV2Plugin* plugin, int32_t opcode, int32_t index,
    intptr_t value, void* ptr, float opt
)
{
    switch (opcode)
    {
        case UviV2AudioMasterOpcodes_Version: return 2400;
        case UviV2AudioMasterOpcodes_Idle:    return 0;
    }
}

static void uvi_v2_plugin_process(UviPlugin *plugin, float **inputs, float **outputs, int32_t sample_frames)
{
    UviV2Plugin *p = plugin->v2.plugin;
    if (p->flags & UviV2PluginFlags_CanReplacing)
        p->processReplacing(p, inputs, outputs, sample_frames);
    //else p->process(p, inputs, outputs, sample_frames);
}

struct UviV2Events
{
	int32_t numEvents;
	intptr_t reserved;
	void* events[2];
};

static void uvi_v2_plugin_process_events(UviPlugin *plugin)
{
    const uint32_t midi_event_count = plugin->v2.midi_event_count;
    if (midi_event_count > 0)
    {
        UviV2Plugin *p = plugin->v2.plugin;
        UviV2Events events{};
        events.numEvents = midi_event_count;
        for (int i = 0; i < midi_event_count; ++i)
            events.events[i] = (void*)&plugin->v2.midi_events[i];
        p->dispatcher(p, UviV2PluginOpcodes_ProcessEvents, 0, 0, &events, 0.0f);
        plugin->v2.midi_event_count = 0;
    }
}

static void uvi_v2_plugin_apply_note_event(UviPlugin *plugin, int32_t status, int32_t key, int32_t velocity, int32_t sample_offset)
{
    if (plugin->v2.midi_event_count >= 256)
        return;

    UviV2MidiEvent &ev = plugin->v2.midi_events[plugin->v2.midi_event_count++];
    ev.type = 1;
    ev.byteSize = sizeof(ev);
    ev.midiData[0] = (unsigned char)status;
    ev.midiData[1] = (unsigned char)key;
    ev.midiData[2] = (unsigned char)velocity;
    ev.deltaFrames = sample_offset;
}

static void uvi_v2_plugin_play_note(UviPlugin *plugin, int32_t key, int32_t velocity, int32_t sample_offset)
{
    uvi_v2_plugin_apply_note_event(plugin, 0x90, key, velocity, sample_offset);
}

static void uvi_v2_plugin_stop_note(UviPlugin *plugin, int32_t key, int32_t sample_offset)
{
    uvi_v2_plugin_apply_note_event(plugin, 0x80, key, 0, sample_offset);
}

static void uvi_v2_stop_all_notes(UviPlugin *plugin)
{
    UviV2Plugin *p = plugin->v2.plugin;
    if (!p)
        return;

    UviV2Events events{};
    UviV2MidiEvent midiEvents[16]{};
    
    events.numEvents = 16;
    events.reserved = 0;
    
    for (uint8_t channel = 0; channel < 16; channel++)
    {
        midiEvents[channel].type = 1;
        midiEvents[channel].byteSize = sizeof(UviV2MidiEvent);
        midiEvents[channel].midiData[0] = 0xB0 | channel;
        midiEvents[channel].midiData[1] = 123;
        midiEvents[channel].midiData[2] = 0;

        events.events[channel] = (void*)&midiEvents[channel];
    }
    
    p->dispatcher(p, UviV2PluginOpcodes_ProcessEvents, 0, 0, &events, 0.0f);
}

static void uvi_v2_plugin_open_editor(UviPlugin *plugin, void *handle)
{
    UviV2Plugin *p = plugin->v2.plugin;
    p->dispatcher(p, UviV2PluginOpcodes_EditOpen, 0, 0, handle, 0);
}

static void uvi_v2_plugin_close_editor(UviPlugin *plugin)
{
    UviV2Plugin *p = plugin->v2.plugin;
    p->dispatcher(p, UviV2PluginOpcodes_EditClose, 0, 0, nullptr, 0);
}

static void uvi_v2_plugin_get_editor_size(UviPlugin *plugin, uint32_t *width, uint32_t *height)
{
    struct UviV2Rect
    {
        int16_t top;
        int16_t left;
        int16_t bottom;
        int16_t right;
    };

    UviV2Rect* rect = nullptr;

    UviV2Plugin *p = plugin->v2.plugin;
    p->dispatcher(p, UviV2PluginOpcodes_EditGetRect, 0, 0, &rect, 0);
    *width  = (uint32_t)(rect->right - rect->left);
    *height = (uint32_t)(rect->bottom - rect->top);
}

static void uvi_v2_plugin_load(UviPlugin *plugin, float sample_rate = 44100.0f, int32_t block_size = 512)
{
    typedef UviV2Plugin* (*PluginMain)(V2AudioMasterCallback audioMaster);
    PluginMain entry = (PluginMain)uvi_get_proc_address(plugin->library, "VSTPluginMain");
    if (!entry)
        entry = (PluginMain)uvi_get_proc_address(plugin->library, "main");
    
    if (!entry)
    {
        printf("[UVI Loader] Not a valid VST 2.x plugin.\n");
        uvi_library_unload(plugin->library);
        return;
    }
    
    UviV2Plugin* p = entry(uvi_v2_audio_master_callback_function);
    if (!p)
    {
        printf("[UVI Loader] VST plugin failed to instantiate.\n");
        uvi_library_unload(plugin->library);
        return;
    }
    
    p->dispatcher(p, UviV2PluginOpcodes_SetSampleRate, 0, 0, nullptr, sample_rate);
    p->dispatcher(p, UviV2PluginOpcodes_SetBlockSize, 0, block_size, nullptr, 0);
    p->dispatcher(p, UviV2PluginOpcodes_MainsChanged, 0, 1, nullptr, 0);

    plugin->open_editor = uvi_v2_plugin_open_editor;
    plugin->close_editor = uvi_v2_plugin_close_editor;
    plugin->get_editor_size = uvi_v2_plugin_get_editor_size;
    plugin->process = uvi_v2_plugin_process;
    plugin->play_note = uvi_v2_plugin_play_note;
    plugin->stop_note = uvi_v2_plugin_stop_note;
    plugin->stop_all_notes = uvi_v2_stop_all_notes;
    plugin->process_events = uvi_v2_plugin_process_events;

    plugin->v2.plugin = p;
    plugin->is_loaded = true;
}

UviPlugin uvi_plugin_load(const char *path)
{
    UviPlugin plugin;

    std::filesystem::path p(path);
    std::filesystem::path extension = p.extension();

    if (extension == ".dll" || extension == ".so" || extension == ".dylib")
        plugin.type = UviPluginType_V2;
    else if (extension == ".vst3")
        plugin.type = UviPluginType_V3;
    else if (extension == ".uvi")
        plugin.type = UviPluginType_Uvi;
    else
        return {};
        
    plugin.library = uvi_library_load(path);
    strncpy(plugin.name, p.stem().string().c_str(), sizeof(plugin.name));

    switch (plugin.type)
    {
    case UviPluginType_V2: uvi_v2_plugin_load(&plugin); break;
    }

    return plugin;
}

void uvi_plugin_unload(UviPlugin *plugin)
{
    UviV2Plugin *p = plugin->v2.plugin;
    plugin->is_loaded = false;
    p->dispatcher(p, UviV2PluginOpcodes_StopProcess, 0, 0, nullptr, 0.0f);
    
    if (p->flags & UviV2PluginFlags_HasEditor)
        p->dispatcher(p, UviV2PluginOpcodes_EditClose, 0, 0, nullptr, 0.0f);
    
    p->dispatcher(p, UviV2PluginOpcodes_MainsChanged, 0, 0, nullptr, 0.0f);
    p->dispatcher(p, UviV2PluginOpcodes_Close, 0, 0, nullptr, 0.0f);
}