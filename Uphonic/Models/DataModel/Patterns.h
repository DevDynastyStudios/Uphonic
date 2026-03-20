#pragma once
#include "Color.h"
#include <cstdint>
#include <vector>
#include <string>

struct MidiNote
{
	double startBeat;
	double lengthBeats;
	uint8_t keyNumber;
	uint8_t velocity;
};

struct MidiPattern
{
	std::string name;
	std::vector<MidiNote> notes;
	Color4 color;

	MidiPattern() = default;
	MidiPattern(const char *name) : name(name) {}
};
