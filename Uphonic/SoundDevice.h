#pragma once

#include "Core.h"
#include "UviLoader.h"

#include <miniaudio.h>
#include <cstdint>
#include <vector>
#include <atomic>

class AudioIOBuffer
{

};

class SoundDevice
{
public:
    static bool Initialize(void);
    static void Shutdown(void);
    static void StopAllNotes(void);

    static Sample LoadSample(const char* filepath);
    static void UnloadSample(Sample& sample);

    static bool ExportToWav(const char* filepath, double startBeat, double endBeat);

private:
    static void AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};
