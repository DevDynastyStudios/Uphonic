#pragma once
#include "Naui/Platform/Window.h"
#include "DataModel/Tracks.h"
#include "DataModel/Samples.h"
#include "DataModel/Patterns.h"
#include "Config/SettingsConfig.h"

namespace Naui { class PlatformWindow; }

class ProjectState
{
public:
	static ProjectState& GetInstance()
	{
		static ProjectState instance;
		return instance;
	}
	
	static void ClearProject();
	void CopyFrom(const ProjectState& other);

	ApplicationSettings settings;
	MasterTrack masterTrack;
	std::vector<AudioTrack> tracks;
	std::vector<MidiPattern> patterns;
	std::vector<AudioSample> samples;
	Naui::PlatformWindow* mainWindow;
	double timelinePositionBeats;
	float beatsPerMinute;
	float masterVolume;
	uint16_t currentMidiPatternIndex;
	bool isPlaying;
	bool isDraggingPlayhead;
	
	ProjectState() : mainWindow(nullptr), timelinePositionBeats(0.0),
					  beatsPerMinute(120.0f), masterVolume(0.8f),
					  currentMidiPatternIndex(0), isPlaying(false),
					  isDraggingPlayhead(false) {}
	
	ProjectState(const ProjectState&) = delete;
	ProjectState& operator=(const ProjectState&) = delete;
};
