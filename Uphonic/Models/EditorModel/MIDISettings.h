#pragma once
#include <string>
#include <cstdint>

struct MIDISettings{
	std::string midiInputDeviceId;
	bool midiThruEnabled = true;

	bool recordOverdub = true;
	bool recordQuantize = false;
	uint32_t recordQuantizeGrid = 16;	// 1/16 notes
	bool metronomeEnabled = true;
	bool countInEnabled = false;
	uint32_t countInBars = 1;

	uint8_t defaultVelocity = 100;
	float defaultNoteLength = 1.0f;
	float defaultBeatWidth = 40.0f;
	float defaultBeatHeight = 14.0f;

	uint32_t gridDivision = 16;	// Refer to recordQuantizeGrid
	float gridSwing = 0.0f;	// 0-1

	float noteHeight = 12.0f;
	bool showGridLines = true;
	bool showNoteLabels = false;
	bool colorNotesByChannel = false;

	uint8_t defaultChannel = 1;
	bool filterByChannel = false;
};