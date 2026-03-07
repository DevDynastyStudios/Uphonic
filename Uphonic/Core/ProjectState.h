#pragma once
#include "Naui/Actions/ActionManager.h"
#include "Models/EditorModel/ApplicationSettings.h"
#include "Models/DataModel/Tracks.h"
#include "Models/DataModel/Samples.h"
#include "Models/DataModel/Patterns.h"

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
	Naui::ActionManager actionManager;

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
	
	ProjectState();	
	ProjectState(const ProjectState&) = delete;
	ProjectState& operator=(const ProjectState&) = delete;
};
