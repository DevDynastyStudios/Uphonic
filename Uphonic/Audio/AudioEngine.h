#pragma once

#include "../Core/ProjectState.h"
#include "../Config/EditorConfig.h"
#include <miniaudio.h>
#include <cstdint>
#include <vector>
#include <atomic>

struct AudioEngineData
{
    ma_device device;
    double lastProcessedBeat;
    bool needsNoteRetrigger;
    std::vector<std::pair<AudioTrack*, bool>> trackActiveBlockState;
};

class AudioEngine
{
public:
    static bool Initialize(const AudioConfig& config);
    static void Shutdown();
    static void StopAllNotes();
    static void AddSample(const char* filepath);
    static bool LoadSample(const char* filepath, AudioSample &sample);
    static void UnloadSample(AudioSample& sample);
    static bool ExportToWav(const char* filepath, double startBeat, double endBeat);
    static AudioConfig& GetConfig() { return s_config; }
    static void SetConfig(const AudioConfig& config) { s_config = config; }

private:
    static void AudioCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount);
    static float CubicInterpolate(float y0, float y1, float y2, float y3, float mu);
    static void CalculatePanGains(float pan, float& leftGain, float& rightGain);
    static void UpdatePeaks(float& peakLeft, float& peakRight, float newLeft, float newRight);
    static void ProcessMidiTrack(AudioTrack& track, double prevBeat, double newBeat, 
                                 double beatsPerFrame, ma_uint32 frameCount, 
                                 float** inputs, float** outputs);
    static void ProcessSampleTrack(AudioTrack& track, double prevBeat, double newBeat,
                                  double beatsPerFrame, ma_uint32 frameCount,
                                  float* trackBuffer, float** inputs, float** outputs);
    static void ProcessTrackEffects(std::vector<PluginEffect>& effects, float* trackBuffer,
                                    ma_uint32 frameCount, float** inputs, float** outputs);
    static void ApplyTrackVolumeAndPan(AudioTrack& track, float* trackBuffer, float* output,
                                       ma_uint32 frameCount, float leftGain, float rightGain);

private:
    static AudioEngineData s_data;
    static AudioConfig s_config;
};

