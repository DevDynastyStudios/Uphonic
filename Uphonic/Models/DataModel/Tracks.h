#pragma once
#include "Timeline.h"
#include "Naui.h"
#include "UVI/UviLoader.h"
#include <imgui.h>
#include <string>
#include <vector>

enum class TrackType
{
	None,
	Audio,
	Midi
};

struct PluginEffect
{
	Uvi::Plugin* plugin;
	Naui::PlatformWindow* window;
	std::string pluginPath;  // for saving/loading
	
	PluginEffect() : plugin(nullptr), window(nullptr) {}
};

struct MasterTrack
{
	std::vector<PluginEffect> effects;
	float volume;
	float pan;
	float peakLeft, peakRight;
	float smoothPeakLeft, smoothPeakRight;
	bool muted;
	bool solo;
	bool armed;
	
	MasterTrack() : volume(1.0f), pan(0.5f), peakLeft(0.0f), peakRight(0.0f), smoothPeakLeft(0.0f), smoothPeakRight(0.0f), muted(false), solo(false), armed(false) {}
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
	float peakLeft, peakRight;
	float smoothPeakLeft, smoothPeakRight;
	bool muted;
	bool solo;
	bool armed;
	
	AudioTrack() : volume(1.0f), pan(0.5f), peakLeft(0.0f), peakRight(0.0f), smoothPeakLeft(0.0f), smoothPeakRight(0.0f), muted(false), solo(false), armed(false) {}
};