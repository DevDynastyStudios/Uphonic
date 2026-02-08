#include "UviLoader.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <cstring>

namespace Uvi
{
// ===== PluginLoader Implementation =====

#if defined(_WIN32)
#include <windows.h>
UviLibrary PluginLoader::LoadLib(const char* path)
{
    return LoadLibraryA(path);
}

void PluginLoader::UnloadLib(UviLibrary library)
{
    FreeLibrary((HMODULE)library);
}

UviProcAddress PluginLoader::GetProcAddr(UviLibrary library, const char* name)
{
    return GetProcAddress((HMODULE)library, name);
}
#elif defined(__linux__)
UviLibrary PluginLoader::LoadLib(const char* path)
{
    return dlopen(path, RTLD_LAZY);
}

void PluginLoader::UnloadLib(UviLibrary library)
{
    dlclose(library);
}

UviProcAddress PluginLoader::GetProcAddr(UviLibrary library, const char* name)
{
    return (UviProcAddress)dlsym((void*)library, name);
}
#endif

Plugin *PluginLoader::Load(const char* path, float sampleRate, int32_t blockSize)
{
    std::filesystem::path fsPath(path);
    std::filesystem::path extension = fsPath.extension();
    
    PluginType type;
    if (extension == ".dll" || extension == ".so" || extension == ".dylib")
        type = PluginType::V2;
    else if (extension == ".vst3")
        type = PluginType::V3;
    else if (extension == ".uvi")
        type = PluginType::Uvi;
    else
        return nullptr;
    
    UviLibrary library = LoadLib(path);
    if (!library)
        return nullptr;
    
    std::string name = fsPath.stem().string();
    
    switch (type)
    {
        case PluginType::V2:
            return new V2PluginWrapper(library, name.c_str(), sampleRate, blockSize);
        case PluginType::V3:
        case PluginType::Uvi:
            // Not implemented yet
            UnloadLib(library);
            return nullptr;
    }
    
    return nullptr;
}

// ===== V2PluginWrapper Implementation =====

V2PluginWrapper::V2PluginWrapper(UviLibrary library, const char* name, float sampleRate, int32_t blockSize)
    : m_plugin(nullptr)
    , m_library(library)
    , m_isLoaded(false)
    , m_midiEventCount(0)
{
    strncpy(m_name, name, sizeof(m_name) - 1);
    m_name[sizeof(m_name) - 1] = '\0';
    memset(m_midiEvents, 0, sizeof(m_midiEvents));
    
    typedef V2Plugin* (*PluginMain)(V2AudioMasterCallback audioMaster);
    PluginMain entry = (PluginMain)PluginLoader::GetProcAddr(library, "VSTPluginMain");
    if (!entry)
        entry = (PluginMain)PluginLoader::GetProcAddr(library, "main");
    
    if (!entry)
    {
        printf("[UVI Loader] Not a valid VST 2.x plugin.\n");
        PluginLoader::UnloadLib(library);
        return;
    }
    
    m_plugin = entry(AudioMasterCallback);
    if (!m_plugin)
    {
        printf("[UVI Loader] VST plugin failed to instantiate.\n");
        PluginLoader::UnloadLib(library);
        return;
    }

    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::Open, 0, 0, nullptr, 0);
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::SetSampleRate, 0, 0, nullptr, sampleRate);
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::SetBlockSize, 0, (intptr_t)blockSize, nullptr, 0);
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::MainsChanged, 0, 1, nullptr, 0);
    
    m_isLoaded = true;
}

V2PluginWrapper::~V2PluginWrapper()
{
    if (!m_isLoaded || !m_plugin)
        return;
    
    m_isLoaded = false;
    
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::StopProcess, 0, 0, nullptr, 0.0f);
    
    if (m_plugin->flags & V2PluginFlags::HasEditor)
        m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::EditClose, 0, 0, nullptr, 0.0f);
    
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::MainsChanged, 0, 0, nullptr, 0.0f);
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::Close, 0, 0, nullptr, 0.0f);
    
    std::thread([library = m_library]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        PluginLoader::UnloadLib(library);
    }).detach();
}

intptr_t V2PluginWrapper::AudioMasterCallback(V2Plugin* plugin, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    switch ((V2AudioMasterOpcodes)opcode)
    {
        case V2AudioMasterOpcodes::Version: 
            return 2400;
        case V2AudioMasterOpcodes::Idle:    
            return 0;
        case V2AudioMasterOpcodes::GetSampleRate: 
            return (intptr_t)44100;
        case V2AudioMasterOpcodes::GetBlockSize:  
            return 512;
        case V2AudioMasterOpcodes::CanDo:
        {
            const char* canDo = (const char*)ptr;
            if (!canDo) return 0;
            if (strcmp(canDo, "sendVstEvents") == 0) return 1;
            if (strcmp(canDo, "sendVstMidiEvent") == 0) return 1;
            if (strcmp(canDo, "receiveVstEvents") == 0) return 1;
            if (strcmp(canDo, "receiveVstMidiEvent") == 0) return 1;
            return 0;
        }
        default:
            return 0;
    }
}

void V2PluginWrapper::ProcessEvents()
{
    if (m_midiEventCount == 0)
        return;

    const size_t size =
        sizeof(V2Events) + (m_midiEventCount - 1) * sizeof(V2Event*);

    V2Events* events = (V2Events*)alloca(size);
    events->numEvents = m_midiEventCount;
    events->reserved = 0;

    for (uint32_t i = 0; i < m_midiEventCount; ++i)
        events->events[i] = (V2Event*)&m_midiEvents[i];

    m_plugin->dispatcher(
        m_plugin,
        (int32_t)V2PluginOpcodes::ProcessEvents,
        0, 0,
        events,
        0.0f
    );

    m_midiEventCount = 0;
}

void V2PluginWrapper::Process(float** inputs, float** outputs, int32_t sampleFrames)
{
    ProcessEvents();
    if (m_plugin->flags & V2PluginFlags::CanReplacing)
        m_plugin->processReplacing(m_plugin, inputs, outputs, sampleFrames);
}

void V2PluginWrapper::ApplyNoteEvent(int32_t status, int32_t key, int32_t velocity, int32_t sampleOffset)
{
    if (m_midiEventCount >= 256)
        return;
    
    V2MidiEvent& ev = m_midiEvents[m_midiEventCount++];
    ev.type = 1;
    ev.byteSize = sizeof(ev);
    ev.midiData[0] = (unsigned char)status;
    ev.midiData[1] = (unsigned char)key;
    ev.midiData[2] = (unsigned char)velocity;
    ev.deltaFrames = sampleOffset;
}

void V2PluginWrapper::PlayNote(int32_t key, int32_t velocity, int32_t sampleOffset)
{
    ApplyNoteEvent(0x90, key, velocity, sampleOffset);
}

void V2PluginWrapper::StopNote(int32_t key, int32_t sampleOffset)
{
    ApplyNoteEvent(0x80, key, 0, sampleOffset);
}

void V2PluginWrapper::StopAllNotes()
{
    m_midiEventCount = 0;
    for (int32_t i = 0; i < 128; i++)
    {
        V2MidiEvent& ev = m_midiEvents[m_midiEventCount++];
        ev.type = 1;
        ev.byteSize = sizeof(ev);
        ev.midiData[0] = 0x90;
        ev.midiData[1] = i;
        ev.midiData[2] = 0;
        ev.deltaFrames = 0;
    }
}

void V2PluginWrapper::OpenEditor(void* handle)
{
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::EditOpen, 0, 0, handle, 0);
}

void V2PluginWrapper::IdleEditor()
{
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::EditIdle, 0, 0, nullptr, 0);
}

void V2PluginWrapper::CloseEditor()
{
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::EditClose, 0, 0, nullptr, 0);
}

void V2PluginWrapper::GetEditorSize(uint32_t* width, uint32_t* height)
{
    V2Rect* rect = nullptr;
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::EditGetRect, 0, 0, &rect, 0);
    *width  = (uint32_t)(rect->right - rect->left);
    *height = (uint32_t)(rect->bottom - rect->top);
}

void V2PluginWrapper::Serialize(const char* filePath)
{
    void* data = nullptr;
    uint32_t size = m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::GetChunk, 0, 0, &data, 0.0f);
    
    if (size <= 0 || !data)
    {
        fprintf(stderr, "[UVI Loader] Failed to get chunk.\n");
        return;
    }
    
    std::ofstream out(filePath, std::ios::binary);
    if (!out)
    {
        fprintf(stderr, "[UVI Loader] Failed to open state file.\n");
        return;
    }
    
    out.write(reinterpret_cast<char*>(data), size);
}

void V2PluginWrapper::Deserialize(const char* filePath)
{
    std::ifstream in(filePath, std::ios::binary);
    if (!in)
    {
        fprintf(stderr, "[UVI Loader] Failed to open state file.\n");
        return;
    }
    
    std::vector<char> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    
    if (buffer.empty())
    {
        fprintf(stderr, "[UVI Loader] Empty state file.\n");
        return;
    }
    
    m_plugin->dispatcher(m_plugin, (int32_t)V2PluginOpcodes::SetChunk, 0, static_cast<intptr_t>(buffer.size()), buffer.data(), 0.0f);
}

}
