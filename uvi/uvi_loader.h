#pragma once

#include <cstdint>

#if defined(_WIN32)
#include <windows.h>
    typedef HINSTANCE UviLibrary;
#else defined(__linux__)
#include <dlfcn.h>
    typedef void* UviLibrary;
#endif

typedef void *UviProcAddress;

enum UviPluginType : uint8_t
{
    UviPluginType_V2,
    UviPluginType_V3,
    UviPluginType_Uvi
};

enum UviV2PluginFlags
{
	UviV2PluginFlags_HasEditor     = 1 << 0,
	UviV2PluginFlags_CanReplacing  = 1 << 4,
	UviV2PluginFlags_ProgramChunks = 1 << 5,
	UviV2PluginFlags_IsSynth       = 1 << 8,
	UviV2PluginFlags_NoSoundInStop = 1 << 9
};

enum UviV2PluginOpcodes
{
	UviV2PluginOpcodes_Open = 0,
	UviV2PluginOpcodes_Close = 1,
	UviV2PluginOpcodes_SetProgram = 2,
	UviV2PluginOpcodes_GetProgram = 3,
	UviV2PluginOpcodes_SetProgramName = 4,
	UviV2PluginOpcodes_GetProgramName = 5,
	UviV2PluginOpcodes_GetParamLabel = 6,
	UviV2PluginOpcodes_GetParamDisplay = 7,
	UviV2PluginOpcodes_GetParamName = 8,
	UviV2PluginOpcodes_SetSampleRate = 10,
	UviV2PluginOpcodes_SetBlockSize = 11,
	UviV2PluginOpcodes_MainsChanged = 12,
	UviV2PluginOpcodes_EditGetRect = 13,
	UviV2PluginOpcodes_EditOpen = 14,
	UviV2PluginOpcodes_EditClose = 15,
	UviV2PluginOpcodes_EditIdle = 19,
	UviV2PluginOpcodes_GetChunk = 23,
	UviV2PluginOpcodes_SetChunk = 24,
	UviV2PluginOpcodes_ProcessEvents = 25,
	UviV2PluginOpcodes_StopProcess = 72
 };

typedef struct UviV2Plugin UviV2Plugin;
typedef	intptr_t (*V2AudioMasterCallback) (UviV2Plugin* plugin, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
typedef intptr_t (*V2PluginDispatcherProc) (UviV2Plugin* plugin, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
typedef void (*V2PluginProcessProc) (UviV2Plugin* plugin, float** inputs, float** outputs, int32_t sampleFrames);
typedef void (*V2PluginProcessDoubleProc) (UviV2Plugin* plugin, double** inputs, double** outputs, int32_t sampleFrames);
typedef void (*V2PluginSetParameterProc) (UviV2Plugin* plugin, int32_t index, float parameter);
typedef float (*V2PluginGetParameterProc) (UviV2Plugin* plugin, int32_t index);

struct UviV2Plugin
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

struct UviV2MidiEvent
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

struct UviPlugin
{
    UviPluginType type;
    UviLibrary library;

    union
    {
        struct
        {
			UviV2MidiEvent midi_events[256];
			uint32_t midi_event_count;
            UviV2Plugin *plugin;
        }
		v2;

        struct
        {
            //V2Plugin *plugin;
        }
		v3;

        struct
        {
            //V2Plugin *plugin;
        }
		uvi;
    };

	void (*open_editor)(UviPlugin *plugin, void *handle);
	void (*close_editor)(UviPlugin *plugin);
	void (*get_editor_size)(UviPlugin *plugin, int *width, int *height);
	void (*process)(UviPlugin *plugin, float **inputs, float **outputs, int32_t sample_frames);
	
	void (*play_note)(UviPlugin *plugin, int32_t key, int32_t velocity, int32_t sample_offset);
	void (*stop_note)(UviPlugin *plugin, int32_t key, int32_t sample_offset);
	void (*stop_all_notes)(UviPlugin *plugin);
	void (*process_events)(UviPlugin *plugin);
};

UviPlugin uvi_plugin_load(const char *path);
void uvi_plugin_unload(UviPlugin *plugin);