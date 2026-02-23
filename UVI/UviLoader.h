#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Uvi
{

class Plugin
{
public:
    virtual ~Plugin() = default;

    virtual void AttachEditor(void* handle) = 0;
    virtual void DetachEditor() = 0;
    virtual void IdleEditor() = 0;
    virtual void Process(float** inputs, float** outputs, int32_t sampleFrames, float bpm) = 0;
    virtual void PlayNote(int32_t key, int32_t velocity, int32_t sampleOffset) = 0;
    virtual void StopNote(int32_t key, int32_t sampleOffset) = 0;
    virtual void StopAllNotes() = 0;
    virtual void Serialize(const char* filePath) = 0;
    virtual void Deserialize(const char* filePath) = 0;
    virtual bool IsLoaded() const = 0;
    virtual const char* GetName() const = 0;
    virtual const char *GetVendor() const = 0;
    virtual const char *GetVersion() const = 0;
    virtual const char *GetUID() const = 0;
};

class PluginLoader
{
public:
    static Plugin* Load(const char* path, float sampleRate = 44100.0f, int32_t blockSize = 512);
};

}