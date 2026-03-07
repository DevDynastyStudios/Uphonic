#pragma once
#include "Naui/Util/UUID.h"
#include "Models/EditorModel/GeneralSettings.h"
#include "Models/EditorModel/AudioSettings.h"
#include "Models/EditorModel/MIDISettings.h"
#include "Models/EditorModel/TimelineSettings.h"
#include "Models/EditorModel/PluginSettings.h"
#include "Config/EditorConfig.h"
#include <string>

struct ApplicationSettings
{
	Naui::UUID projectID;
	std::string projectName;
	
	AudioConfig config;
	GeneralSettings generalSettings;
	AudioSettings audioSettings;
	MIDISettings midiSettings;
	TimelineSettings timelineSettings;
	PluginSettings pluginSettings;
};