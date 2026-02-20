#pragma once
#include "Naui.h"
#include <filesystem>
#include <optional>

struct ApplicationSettings
{
	Naui::UUID projectID;
	std::string projectName;
	std::optional<std::filesystem::path> saveToPath;
	std::vector<std::string> pluginSearchPaths;
	uint32_t undoHistory;
	uint32_t audioSampleRate;
	
	ApplicationSettings() : audioSampleRate(44100), pluginSearchPaths({
		"C:\\Program Files\\Common Files\\VST3",
		"C:\\Program Files\\Steinberg\\VST3"
	}) {}
};