#pragma once

#include "platform/platform.h"

#include <vector>
#include <atomic>
#include <cstdint>

#include <pluginterfaces/vst2.x/aeffectx.h>
#include <uvi_loader.h>

enum UphTrackType : uint8_t
{
    UphTrackType_Midi,
    UphTrackType_Sample
};

union UphTimelineBlock
{
    struct
    {
        UphTrackType track_type;
        uint16_t pattern_index;
        double start_time;
        float start_offset;
        float length;
        float reserved;
    };
    struct
    {
        UphTrackType track_type;
        uint16_t sample_index;
        double start_time;
        float start_offset;
        float length;
        float stretch_scale;
    };
};

struct UphPluginInstance
{
    UphChildWindow window;
    UphLibrary library = nullptr;
    AEffect* effect = nullptr;
};

struct UphInstrument
{
    UphPluginInstance plugin;
    float volume, pan, pitch;
};

struct UphNote
{
    float start, length;
    uint8_t key;
    uint8_t velocity = 100;
};

struct UphMidiPattern
{
    char name[64];
    std::vector<UphNote> notes;
};

enum UphSampleType : uint8_t
{
    UphSampleType_Mono,
    UphSampleType_Stereo
};

struct UphSample
{
    char name[64];
    UphSampleType type;

    float sample_rate;

    float *frames;
    uint64_t frame_count;
};

struct UphTrack
{
    char name[64] = "Untitled Track";
    float volume = 1.0f, pan = 0.0f;
    float peak_left = 0.0f, peak_right = 0.0f;
    bool muted = false;
    uint32_t color = 0xFFFFFFFF;
    UphTrackType track_type;
    UphInstrument instrument;
    std::vector<UphTimelineBlock> timeline_blocks;
};

struct UphProject
{
    float volume = 0.5f, bpm = 300.0f;
    std::vector<UphMidiPattern> patterns;
    std::vector<UphSample> samples;
    std::vector<UphTrack> tracks = std::vector<UphTrack>(32);
};

struct UphApplication
{
    UphProject project{};
    uint32_t current_pattern_index = 0;
    uint32_t current_track_index = 0;
    uint32_t current_instrument_track_index = 0;
    float midi_editor_song_position = 0.0f;
    float song_timeline_song_position = 0.0f;
    bool is_midi_editor_playing = false;
    bool is_song_timeline_playing = false;
    std::atomic<bool> should_stop_all_notes = false;
};

inline UphApplication *app = nullptr;