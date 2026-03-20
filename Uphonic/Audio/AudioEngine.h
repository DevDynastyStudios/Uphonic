#pragma once

#include "Core/ProjectState.h"
#include "Config/EditorConfig.h"
#include <cstdint>
#include <vector>

struct AudioContext
{
	const AudioConfig* config;
	AudioSettings* settings;
};

class AudioEngine
{
public:
    static bool Initialize(const AudioContext& ctx);
    static void Shutdown();
    static void StopAllNotes();
    static AudioSample& AddSample(const char* filepath, ProjectState& state = ProjectState::GetInstance());
    static bool LoadSample(const char* filepath, AudioSample &sample);
    static void UnloadSample(AudioSample& sample);
    static bool ExportToWav(const char* filepath, double startBeat, double endBeat);
    static AudioContext& GetContext();
    static void SetConfig(const AudioContext& ctx);
};

