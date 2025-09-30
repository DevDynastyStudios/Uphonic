#pragma once

#include <cstdint>
#include <vector>

#include <pluginterfaces/vst2.x/aeffectx.h>

struct UphMixerEffect
{

};

struct UphMidiPatternInstance
{
    uint16_t pattern_index;
    double start_time;
    float start_offset;
    float length;
};

struct UphInstrument
{
    AEffect *effect;
    float volume, pan, pitch;
};

struct UphNote
{
    float start, length;
    uint8_t key;
    uint8_t velocity;
};

struct UphMidiPattern
{
    char name[64];
    std::vector<UphNote> notes;
};

struct UphMidiTrack
{
    UphInstrument instrument;
    std::vector<UphMidiPatternInstance> pattern_instances;
};

struct UphSampleTrack
{
    // UphSample sample;
    // std::vector<UphSampleInstance> sample_instances;
};

enum class UphTrackType : uint8_t
{
    Midi,
    Sample
};

struct UphTrack
{
    char name[64];
    float volume = 1.0f, pan = 0.5f;
    bool muted = false;
    UphTrackType track_type;
    UphMidiTrack midi_track;
    UphSampleTrack sample_track;
};

struct UphProject
{
    float volume = 0.5f, bpm = 120.0f;
    std::vector<UphMidiPattern> patterns;
    std::vector<UphTrack> tracks = std::vector<UphTrack>(32);
};

struct UphApplication
{
    UphProject project;
    uint32_t current_pattern_index = 0;
    uint32_t current_track_index = 0;
    float midi_editor_song_position = 0.0f;
    float song_timeline_song_position = 0.0f;
    bool is_midi_editor_playing = false;
    bool is_song_timeline_playing = false;
};

inline UphApplication *app = nullptr;