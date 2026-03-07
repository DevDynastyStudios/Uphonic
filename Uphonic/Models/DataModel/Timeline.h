#pragma once
#include <cstdint>

struct MidiTimelineBlock
{
	double startBeat;
	double startOffsetBeats;
	double lengthBeats;
	double reserved;
	uint16_t patternIndex;
};

struct SampleTimelineBlock
{
	double startBeat;
	double startOffsetBeats;
	double lengthBeats;
	double stretchScale;
	uint16_t sampleIndex;
};

union TimelineBlock
{
	MidiTimelineBlock midiBlock;
	SampleTimelineBlock sampleBlock;
	
	TimelineBlock() : midiBlock{} {}
};
