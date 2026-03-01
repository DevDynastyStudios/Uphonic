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
	
    // https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html
    ApplicationSettings() : audioSampleRate(44100), pluginSearchPaths({
#if NAUI_PLATFORM_WINDOWS
        "C:\\Program Files\\Common Files\\VST3",
        "C:\\Program Files\\Steinberg\\VST3",
#elif NAUI_PLATFORM_LINUX
        Naui::Directory::HomeDirectory() / ".vst3",
        "/usr/lib/vst3",
        "/usr/local/lib/vst3",
        Naui::Directory::AppDataDirectory() / "vst3", // (Smoke): not sure about this one...
#endif
    }) {}
};