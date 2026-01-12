#pragma once
#include <cstdint>
#include <memory>

namespace Uvi
{

#if defined(_WIN32)
    typedef void* UviLibrary;
#else
#include <dlfcn.h>
    typedef void* UviLibrary;
#endif

typedef void* UviProcAddress;

enum class PluginType : uint8_t
{
    V2,
    V3,
    Uvi
};

enum V2PluginFlags
{
    HasEditor     = 1 << 0,
    CanReplacing  = 1 << 4,
    ProgramChunks = 1 << 5,
    IsSynth       = 1 << 8,
    NoSoundInStop = 1 << 9
};

enum class V2PluginOpcodes
{
    Open = 0,
    Close = 1,
    SetProgram = 2,
    GetProgram = 3,
    SetProgramName = 4,
    GetProgramName = 5,
    GetParamLabel = 6,
    GetParamDisplay = 7,
    GetParamName = 8,
    SetSampleRate = 10,
    SetBlockSize = 11,
    MainsChanged = 12,
    EditGetRect = 13,
    EditOpen = 14,
    EditClose = 15,
    EditIdle = 19,
    GetChunk = 23,
    SetChunk = 24,
    ProcessEvents = 25,
    StopProcess = 72
};

enum class V2AudioMasterOpcodes
{
    Automate = 0,
    Version,
    CurrentId,
    Idle,
    PinConnected,
    GetSampleRate = 16,
    GetBlockSize = 17,
    CanDo = 37
};

struct V2Plugin;

typedef intptr_t (*V2AudioMasterCallback)(V2Plugin* plugin, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
typedef intptr_t (*V2PluginDispatcherProc)(V2Plugin* plugin, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
typedef void (*V2PluginProcessProc)(V2Plugin* plugin, float** inputs, float** outputs, int32_t sampleFrames);
typedef void (*V2PluginProcessDoubleProc)(V2Plugin* plugin, double** inputs, double** outputs, int32_t sampleFrames);
typedef void (*V2PluginSetParameterProc)(V2Plugin* plugin, int32_t index, float parameter);
typedef float (*V2PluginGetParameterProc)(V2Plugin* plugin, int32_t index);

struct V2Plugin
{
    int32_t magic;
    V2PluginDispatcherProc dispatcher;
    V2PluginProcessProc process;
    V2PluginSetParameterProc setParameter;
    V2PluginGetParameterProc getParameter;
    int32_t numPrograms;
    int32_t numParams;
    int32_t numInputs;
    int32_t numOutputs;
    int32_t flags;
    
    intptr_t resvd1;
    intptr_t resvd2;
    
    int32_t initialDelay;
    
    int32_t realQualities;
    int32_t offQualities;
    float ioRatio;
    void* object;
    void* user;
    int32_t uid;
    int32_t version;
    V2PluginProcessProc processReplacing;
    V2PluginProcessDoubleProc processDoubleReplacing;
    char future[56];
};

struct V2MidiEvent
{
    int32_t type;
    int32_t byteSize;
    int32_t deltaFrames;
    int32_t flags;
    int32_t noteLength;
    int32_t noteOffset;
    char midiData[4];
    char detune;
    char noteOffVelocity;
    char reserved1;
    char reserved2;
};

struct V2Event
{
    int32_t type;
    int32_t byteSize;
    int32_t deltaFrames;
    int32_t flags;
    char data[16];
};

struct V2Events
{
    int32_t numEvents;
    intptr_t reserved;
    V2Event* events[256];
};

struct V2Rect
{
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
};

// Base plugin interface
class Plugin
{
public:
    virtual ~Plugin() = default;
    
    virtual void OpenEditor(void* handle) = 0;
    virtual void IdleEditor() = 0;
    virtual void CloseEditor() = 0;
    virtual void GetEditorSize(uint32_t* width, uint32_t* height) = 0;
    virtual void Process(float** inputs, float** outputs, int32_t sampleFrames) = 0;
    virtual void PlayNote(int32_t key, int32_t velocity, int32_t sampleOffset) = 0;
    virtual void StopNote(int32_t key, int32_t sampleOffset) = 0;
    virtual void StopAllNotes() = 0;
    virtual void Serialize(const char* filePath) = 0;
    virtual void Deserialize(const char* filePath) = 0;
    virtual bool IsLoaded() const = 0;
    virtual const char* GetName() const = 0;
};

// VST 2.x plugin implementation
class V2PluginWrapper : public Plugin
{
public:
    V2PluginWrapper(UviLibrary library, const char* name, float sampleRate = 44100.0f, int32_t blockSize = 512);
    ~V2PluginWrapper() override;
    
    void OpenEditor(void* handle) override;
    void IdleEditor() override;
    void CloseEditor() override;
    void GetEditorSize(uint32_t* width, uint32_t* height) override;
    void Process(float** inputs, float** outputs, int32_t sampleFrames) override;
    void PlayNote(int32_t key, int32_t velocity, int32_t sampleOffset) override;
    void StopNote(int32_t key, int32_t sampleOffset) override;
    void StopAllNotes() override;
    void Serialize(const char* filePath) override;
    void Deserialize(const char* filePath) override;
    bool IsLoaded() const override { return m_isLoaded; }
    const char* GetName() const override { return m_name; }

private:
    void ProcessEvents();
    void ApplyNoteEvent(int32_t status, int32_t key, int32_t velocity, int32_t sampleOffset);
    static intptr_t AudioMasterCallback(V2Plugin* plugin, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

private:
    V2Plugin* m_plugin;
    UviLibrary m_library;
    char m_name[64];
    bool m_isLoaded;
    V2MidiEvent m_midiEvents[256];
    uint32_t m_midiEventCount;
};

// Plugin loader/factory
class PluginLoader
{
public:
    static Plugin *Load(const char* path, float sampleRate = 44100.0f, int32_t blockSize = 512);
    
private:
    static UviLibrary LoadLib(const char* path);
    static void UnloadLib(UviLibrary library);
    static UviProcAddress GetProcAddr(UviLibrary library, const char* name);

    friend class V2PluginWrapper;
};

}