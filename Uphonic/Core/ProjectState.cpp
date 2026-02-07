#include "ProjectState.h"

ProjectState& g_ProjectState = ProjectState::GetInstance();

void ProjectState::ClearProject()
{
	g_ProjectState.timelinePositionBeats = 0.0;
	g_ProjectState.beatsPerMinute = 120.0f;
	g_ProjectState.masterVolume = 0.8f;
	g_ProjectState.currentMidiPatternIndex = 0;
	g_ProjectState.isPlaying = false;
	g_ProjectState.isDraggingPlayhead = false;
	g_ProjectState.currentMidiPatternIndex = 0;
	g_ProjectState.timelinePositionBeats = 0;

	g_ProjectState.samples.clear();
	g_ProjectState.patterns.clear();
	g_ProjectState.tracks.clear();
}

void ProjectState::CopyFrom(const ProjectState& other)
{
	settings = other.settings;
	masterTrack = other.masterTrack;

	timelinePositionBeats = other.timelinePositionBeats;
	beatsPerMinute = other.beatsPerMinute;
	masterVolume = other.masterVolume;
	currentMidiPatternIndex = other.currentMidiPatternIndex;
	isPlaying = other.isPlaying;
	isDraggingPlayhead = other.isDraggingPlayhead;
	currentMidiPatternIndex = other.currentMidiPatternIndex;
	timelinePositionBeats = other.timelinePositionBeats;

	samples.clear();
	samples.reserve(other.samples.size());
	for(const AudioSample& sample : other.samples)
	{
		samples.push_back(sample);
	}

	patterns.clear();
	patterns.reserve(other.patterns.size());
	for(const MidiPattern& pattern : other.patterns)
	{
		patterns.push_back(pattern);
	}

	tracks.clear();
	tracks.reserve(other.tracks.size());
	for(const AudioTrack& track : other.tracks)
	{
		tracks.push_back(track);
	}

}