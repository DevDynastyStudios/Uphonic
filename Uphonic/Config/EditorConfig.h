#pragma once
#include <cstdint>

struct MidiEditorConfig
{
    float minZoom = 0.25f;
    float maxZoom = 4.0f;
    float minVerticalZoom = 0.5f;
    float maxVerticalZoom = 3.0f;
    float pianoWidth = 60.0f;
    int totalKeys = 128;
    int defaultSnap = 16;
    float minNoteLength = 0.125f;
};

struct TimelineConfig
{
    float minZoom = 0.20f;
    float maxZoom = 4.0f;
    float minTrackHeight = 40.0f;
    float maxTrackHeight = 200.0f;
    float rulerHeight = 30.0f;
    float minInstanceLength = 0.25f;
    float instancePadding = 5.0f;
    float resizeHandleWidth = 4.0f;
    float resizeHandleTolerance = 6.0f;
    float blockEndEpsilon = 0.005f;
};

struct MixerConfig
{
    float stripWidth = 80.0f;
    float vuMeterWidth = 24.0f;
    float vuMeterHeight = 150.0f;
    int vuSegments = 30;
    float effectsPanelWidth = 250.0f;
    float knobSize = 40.0f;
    float buttonSize = 25.0f;
    float vuRedThreshold = 0.85f;
    float vuYellowThreshold = 0.7f;
    float minDecibel = -60.0f;
};

struct AudioConfig
{
    float peakDecayRate = 0.9995f;
    float panCenterValue = 0.5f;
    float masterVolumeNormalization = 0.707106781f;
    int maxMidiEvents = 256;
    int midiKeyOffset = 127;
    float blockEndEpsilon = 0.005f;
};
