#pragma once
#include "Naui/Base.h"
#include "Naui/FileSystem/File.h"
#include <string>
#include <vector>

struct PluginSettings
{
	std::vector<std::string> pluginPaths;
	std::vector<std::string> blackList;	// Plugin ID's or paths
	bool sandboxPlugins = false;	// Run in separate process

	float pluginWindowScale = 1.0f;
	bool openPluginWindowOnInsert = true;
	bool dockPluginWindows = false;

	bool sortAlphabetically = true;
	bool groupByVendor = false;
	bool groupByCategory = false;

	    // https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html
    PluginSettings() : pluginPaths({
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

