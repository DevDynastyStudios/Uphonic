#pragma once

#include "Naui.h"
#include "Naui/Platform/Window.h"

#include "UviLoader.h"

#include <string>
#include <vector>
#include <cstdint>

struct MidiNote
{
    double start;
    double length;
    uint8_t key;
    uint8_t velocity;
};

struct MidiPattern
{
    std::string name = "Pattern 1";
    std::vector<MidiNote> notes;
};

union TimelineBlock
{
    double start;
    double startOffset;
    double length;

    struct 
    {
        double reserved0;
        uint16_t patternIndex;
    };
    struct 
    {
        double stretchScale;
        uint16_t sampleIndex;
    };
};

enum class SampleType
{
    Mono,
    Stereo
};

struct Sample
{
    std::string name;

    SampleType type;
    float* frames;

    uint64_t frameCount;
    uint32_t sampleRate;
};

struct Effect
{
    Uvi::Plugin *plugin = nullptr;
    Naui::PlatformWindow* window;
};

enum TrackType
{
	TrackType_None,
    TrackType_Sample,
    TrackType_Midi
};

struct Track
{
    Effect instrument;
    ImVec4 color;

    std::vector<TimelineBlock> blocks;
    std::vector<Effect> effects;
    std::string name;

    TrackType type;

    float volume = 1.0f;
    float pan = 0.5f;
    float peakLeft = 0.0f, peakRight = 0.0f; // for the mixer display

    bool muted = false;
    bool solo = false;
    bool armed = false;
};

struct MasterTrack
{
    std::vector<Effect> effects;

    float volume = 1.0f;
    float pan = 0.5f;
    float peakLeft = 0.0f, peakRight = 0.0f;

    bool muted = false;
    bool solo = false;
    bool armed = false;
};

struct Settings
{
    std::vector<std::string> pluginPaths = {
#if NAUI_PLATFORM_WINDOWS
        "C:\\Program Files\\VSTPlugins",
        "C:\\Program Files\\Steinberg\\VSTPlugins",
        "E:\\VST2",
#endif
#if NAUI_PLATFORM_LINUX
        "~/.vst",
#endif
    };
    uint32_t sampleRate = 44100;
};

struct Core
{
    static Settings settings;
    static MasterTrack masterTrack;
    static std::vector<Track> tracks;
    static std::vector<MidiPattern> patterns;
    static std::vector<Sample> samples;
    static Naui::PlatformWindow *mainWindow;
    static double timelinePosition; // for the song timeline handle
    static float bpm;
    static float volume;
    static uint16_t currentMidiPattern; // index for the midi editor
    static bool isPlayingTimeline;
    static bool isDraggingHandle;
};
