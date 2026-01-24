#pragma once

#include "Naui.h"
#include "Naui/Platform/Window.h"
#include "../UVI/UviLoader.h"
#include <string>
#include <filesystem>
#include <vector>
#include <cstdint>

enum class SampleChannelType
{
	Mono,
	Stereo
};

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
	ImVec4 color;

	MidiPattern() = default;
	MidiPattern(const char *name) : name(name) {}
};

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

struct AudioSample
{
	std::string name;
	std::filesystem::path filePath;  // for saving/loading
	SampleChannelType channelType;
	ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	float* frameData;
	uint64_t frameCount;
	uint32_t sampleRate;

	AudioSample() : channelType(SampleChannelType::Mono), frameData(nullptr), frameCount(0), sampleRate(44100) {}
	bool IsValid() const { return frameCount > 0; }
};

struct PluginEffect
{
	Uvi::Plugin* plugin;
	Naui::PlatformWindow* window;
	std::string pluginPath;  // for saving/loading
	
	PluginEffect() : plugin(nullptr), window(nullptr) {}
};

enum class TrackType
{
	None,
	Audio,
	Midi
};

struct AudioTrack
{
	PluginEffect instrument;
	ImVec4 color;
	std::vector<TimelineBlock> blocks;
	std::vector<PluginEffect> effects;
	std::string name;
	TrackType type;
	float volume;
	float pan;
	float peakLeft;
	float peakRight;
	bool muted;
	bool solo;
	bool armed;
	
	AudioTrack() : volume(1.0f), pan(0.5f), peakLeft(0.0f), peakRight(0.0f), muted(false), solo(false), armed(false) {}
};

struct MasterTrack
{
	std::vector<PluginEffect> effects;
	float volume;
	float pan;
	float peakLeft;
	float peakRight;
	bool muted;
	bool solo;
	bool armed;
	
	MasterTrack() : volume(1.0f), pan(0.5f), peakLeft(0.0f), peakRight(0.0f), muted(false), solo(false), armed(false) {}
};

struct ApplicationSettings
{
	std::vector<std::string> pluginSearchPaths;
	uint32_t audioSampleRate;
	
	ApplicationSettings() : audioSampleRate(44100), pluginSearchPaths({
		"C:\\Program Files\\VSTPlugins",
		"C:\\Program Files\\Steinberg\\VSTPlugins"
	}) {}
};

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
