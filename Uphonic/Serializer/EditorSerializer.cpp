#include "EditorSerializer.h"
#include "Core/ProjectState.h"
#include "Naui/FileSystem/File.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

std::filesystem::path EditorSerializer::SettingsDir()
{
	return Naui::Directory::WorkspaceDirectory().parent_path() / "Settings";
}

static bool WriteJson(const std::filesystem::path& path, const json& j)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream f(path);
	if (!f.is_open())
	{
		std::cerr << "[EditorSerializer] Failed to write: " << path << "\n";
		return false;
	}
	f << j.dump(4);
	return true;
}

static json ReadJson(const std::filesystem::path& path)
{
	std::ifstream f(path);
	if (!f.is_open())
		return {};
	try
	{
		json j;
		f >> j;
		return j;
	}
	catch (...)
	{
		std::cerr << "[EditorSerializer] Failed to parse: " << path << "\n";
		return {};
	}
}

bool EditorSerializer::SaveSettings(const ProjectState& state)
{
	const std::filesystem::path dir = SettingsDir();
	bool ok = true;
	ok &= SaveGeneralSettings(state, dir);
	ok &= SaveAudioSettings(state, dir);
	ok &= SaveMidiSettings(state, dir);
	ok &= SaveTimelineSettings(state, dir);
	ok &= SavePluginSettings(state, dir);
	return ok;
}

bool EditorSerializer::SaveGeneralSettings(const ProjectState& state, const std::filesystem::path& dir)
{
	const GeneralSettings& g = state.settings.generalSettings;

	json j;
	j["theme"]							= g.theme;
	j["languageCode"]					= g.languageCode;
	j["regionCode"]						= g.regionCode;
	j["uiScale"]						= g.uiScale;
	j["fontSize"]						= g.fontSize;
	j["openLastProjectOnStartup"]		= g.openLastProjectOnStartup;
	j["lastProjectPath"]				= g.lastProjectPath;
	j["autosaveEnabled"]				= g.autosaveEnabled;
	j["undoHistoryLimit"]				= g.undoHistoryLimit;
	j["confirmOnExit"]					= g.confirmOnExit;
	j["confirmOnDelete"]				= g.confirmOnDelete;
	j["showTooltips"]					= g.showTooltips;
	j["lastOpenDirectory"]				= g.lastOpenDirectory;
	j["lastSaveDirectory"]				= g.lastSaveDirectory;
	j["horizontalScrollSensitivity"]	= g.horizontalScrollSensitivity;
	j["verticalScrollSensitivity"]		= g.verticalScrollSensitivity;
	j["horizontalZoomSensitivity"]		= g.horizontalZoomSensitivity;
	j["verticalZoomSensitivity"]		= g.verticalZoomSensitivity;
	j["followPlayhead"]					= g.followPlayhead;
	j["playheadLockPosition"]			= g.playheadLockPosition;
	j["defaultSnap"]					= g.defaultSnap;
	j["defaultHorizontalZoom"]			= g.defaultHorizontalZoom;
	j["defaultVerticalZoom"]			= g.defaultVerticalZoom;
	j["defaultBeatWidth"]				= g.defaultBeatWidth;

	return WriteJson(dir / "general.json", j);
}

bool EditorSerializer::SaveAudioSettings(const ProjectState& state, const std::filesystem::path& dir)
{
	const AudioSettings& a = state.settings.audioSettings;

	json j;
	j["outputDevice"]				= a.outputDevice;
	j["inputDevice"]				= a.inputDevice;
	j["sampleRate"]					= a.sampleRate;
	j["bufferSize"]					= a.bufferSize;
	j["channels"]					= a.channels;
	j["exclusiveMode"]				= a.exclusiveMode;
	j["softwareMonitoring"]			= a.softwareMonitoring;
	j["monitoringGain"]				= a.monitoringGain;
	j["autoRestartOnDeviceChange"]	= a.autoRestartOnDeviceChange;
	j["underrunProtection"]			= a.underrunProtection;
	j["vuMeterHoldTime"]			= a.vuMeterHoldTime;
	j["vuMeterDecayRate"]			= a.vuMeterDecayRate;

	return WriteJson(dir / "audio.json", j);
}

bool EditorSerializer::SaveMidiSettings(const ProjectState& state, const std::filesystem::path& dir)
{
	const MIDISettings& m = state.settings.midiSettings;

	json j;
	j["midiInputDeviceId"]		= m.midiInputDeviceId;
	j["midiThruEnabled"]		= m.midiThruEnabled;
	j["recordOverdub"]			= m.recordOverdub;
	j["recordQuantize"]			= m.recordQuantize;
	j["recordQuantizeGrid"]		= m.recordQuantizeGrid;
	j["metronomeEnabled"]		= m.metronomeEnabled;
	j["countInEnabled"]			= m.countInEnabled;
	j["countInBars"]			= m.countInBars;
	j["defaultVelocity"]		= m.defaultVelocity;
	j["defaultNoteLength"]		= m.defaultNoteLength;
	j["defaultBeatWidth"]		= m.defaultBeatWidth;
	j["defaultBeatHeight"]		= m.defaultBeatHeight;
	j["noteHeight"]				= m.noteHeight;
	j["gridDivision"]			= m.gridDivision;
	j["gridSwing"]				= m.gridSwing;
	j["showGridLines"]			= m.showGridLines;
	j["showNoteLabels"]			= m.showNoteLabels;
	j["colorNotesByChannel"]	= m.colorNotesByChannel;
	j["defaultChannel"]			= m.defaultChannel;
	j["filterByChannel"]		= m.filterByChannel;

	return WriteJson(dir / "midi.json", j);
}

bool EditorSerializer::SaveTimelineSettings(const ProjectState& state, const std::filesystem::path& dir)
{
	const TimelineSettings& t = state.settings.timelineSettings;

	json j;
	j["defaultTrackHeight"]		= t.defaultTrackHeight;
	j["trackHeaderWidth"]		= t.trackHeaderWidth;
	j["defaultBeatWidth"]		= t.defaultBeatWidth;
	j["defaultInstanceLength"]	= t.defaultInstanceLength;

	return WriteJson(dir / "timeline.json", j);
}

bool EditorSerializer::SavePluginSettings(const ProjectState& state, const std::filesystem::path& dir)
{
	const PluginSettings& p = state.settings.pluginSettings;

	json j;
	j["pluginPaths"]				= p.pluginPaths;
	j["blackList"]					= p.blackList;
	j["sandboxPlugins"]				= p.sandboxPlugins;
	j["pluginWindowScale"]			= p.pluginWindowScale;
	j["openPluginWindowOnInsert"]	= p.openPluginWindowOnInsert;
	j["dockPluginWindows"]			= p.dockPluginWindows;
	j["sortAlphabetically"]			= p.sortAlphabetically;
	j["groupByVendor"]				= p.groupByVendor;
	j["groupByCategory"]			= p.groupByCategory;

	return WriteJson(dir / "plugins.json", j);
}

bool EditorSerializer::LoadSettings(ProjectState& state)
{
	const std::filesystem::path dir = SettingsDir();
	bool ok = true;
	ok &= LoadGeneralSettings(state, dir);
	ok &= LoadAudioSettings(state, dir);
	ok &= LoadMidiSettings(state, dir);
	ok &= LoadTimelineSettings(state, dir);
	ok &= LoadPluginSettings(state, dir);
	return ok;
}

bool EditorSerializer::LoadGeneralSettings(ProjectState& state, const std::filesystem::path& dir)
{
	const json j = ReadJson(dir / "general.json");
	if (j.empty()) return false;

	GeneralSettings& g = state.settings.generalSettings;

	if (j.contains("theme"))						g.theme							= j["theme"];
	if (j.contains("languageCode"))					g.languageCode					= j["languageCode"];
	if (j.contains("regionCode"))					g.regionCode					= j["regionCode"];
	if (j.contains("uiScale"))						g.uiScale						= j["uiScale"];
	if (j.contains("fontSize"))						g.fontSize						= j["fontSize"];
	if (j.contains("openLastProjectOnStartup"))		g.openLastProjectOnStartup		= j["openLastProjectOnStartup"];
	if (j.contains("lastProjectPath"))				g.lastProjectPath				= j["lastProjectPath"];
	if (j.contains("autosaveEnabled"))				g.autosaveEnabled				= j["autosaveEnabled"];
	if (j.contains("undoHistoryLimit"))				g.undoHistoryLimit				= j["undoHistoryLimit"];
	if (j.contains("confirmOnExit"))				g.confirmOnExit					= j["confirmOnExit"];
	if (j.contains("confirmOnDelete"))				g.confirmOnDelete				= j["confirmOnDelete"];
	if (j.contains("showTooltips"))					g.showTooltips					= j["showTooltips"];
	if (j.contains("lastOpenDirectory"))			g.lastOpenDirectory				= j["lastOpenDirectory"];
	if (j.contains("lastSaveDirectory"))			g.lastSaveDirectory				= j["lastSaveDirectory"];
	if (j.contains("horizontalScrollSensitivity"))	g.horizontalScrollSensitivity	= j["horizontalScrollSensitivity"];
	if (j.contains("verticalScrollSensitivity"))	g.verticalScrollSensitivity		= j["verticalScrollSensitivity"];
	if (j.contains("horizontalZoomSensitivity"))	g.horizontalZoomSensitivity		= j["horizontalZoomSensitivity"];
	if (j.contains("verticalZoomSensitivity"))		g.verticalZoomSensitivity		= j["verticalZoomSensitivity"];
	if (j.contains("followPlayhead"))				g.followPlayhead				= j["followPlayhead"];
	if (j.contains("playheadLockPosition"))			g.playheadLockPosition			= j["playheadLockPosition"];
	if (j.contains("defaultSnap"))					g.defaultSnap					= j["defaultSnap"];
	if (j.contains("defaultHorizontalZoom"))		g.defaultHorizontalZoom			= j["defaultHorizontalZoom"];
	if (j.contains("defaultVerticalZoom"))			g.defaultVerticalZoom			= j["defaultVerticalZoom"];
	if (j.contains("defaultBeatWidth"))				g.defaultBeatWidth				= j["defaultBeatWidth"];

	return true;
}

bool EditorSerializer::LoadAudioSettings(ProjectState& state, const std::filesystem::path& dir)
{
	const json j = ReadJson(dir / "audio.json");
	if (j.empty())
		return false;

	AudioSettings& a = state.settings.audioSettings;

	if (j.contains("outputDevice"))					a.outputDevice				= j["outputDevice"];
	if (j.contains("inputDevice"))					a.inputDevice				= j["inputDevice"];
	if (j.contains("sampleRate"))					a.sampleRate				= j["sampleRate"];
	if (j.contains("bufferSize"))					a.bufferSize				= j["bufferSize"];
	if (j.contains("channels"))						a.channels					= j["channels"];
	if (j.contains("exclusiveMode"))				a.exclusiveMode				= j["exclusiveMode"];
	if (j.contains("softwareMonitoring"))			a.softwareMonitoring		= j["softwareMonitoring"];
	if (j.contains("monitoringGain"))				a.monitoringGain			= j["monitoringGain"];
	if (j.contains("autoRestartOnDeviceChange"))	a.autoRestartOnDeviceChange	= j["autoRestartOnDeviceChange"];
	if (j.contains("underrunProtection"))			a.underrunProtection		= j["underrunProtection"];
	if (j.contains("vuMeterHoldTime"))				a.vuMeterHoldTime			= j["vuMeterHoldTime"];
	if (j.contains("vuMeterDecayRate"))				a.vuMeterDecayRate			= j["vuMeterDecayRate"];

	return true;
}

bool EditorSerializer::LoadMidiSettings(ProjectState& state, const std::filesystem::path& dir)
{
	const json j = ReadJson(dir / "midi.json");
	if (j.empty())
		return false;

	MIDISettings& m = state.settings.midiSettings;

	if (j.contains("midiInputDeviceId"))	m.midiInputDeviceId		= j["midiInputDeviceId"];
	if (j.contains("midiThruEnabled"))		m.midiThruEnabled		= j["midiThruEnabled"];
	if (j.contains("recordOverdub"))		m.recordOverdub			= j["recordOverdub"];
	if (j.contains("recordQuantize"))		m.recordQuantize		= j["recordQuantize"];
	if (j.contains("recordQuantizeGrid"))	m.recordQuantizeGrid	= j["recordQuantizeGrid"];
	if (j.contains("metronomeEnabled"))		m.metronomeEnabled		= j["metronomeEnabled"];
	if (j.contains("countInEnabled"))		m.countInEnabled		= j["countInEnabled"];
	if (j.contains("countInBars"))			m.countInBars			= j["countInBars"];
	if (j.contains("defaultVelocity"))		m.defaultVelocity		= j["defaultVelocity"];
	if (j.contains("defaultNoteLength"))	m.defaultNoteLength		= j["defaultNoteLength"];
	if (j.contains("defaultBeatWidth"))		m.defaultBeatWidth		= j["defaultBeatWidth"];
	if (j.contains("defaultBeatHeight"))	m.defaultBeatHeight		= j["defaultBeatHeight"];
	if (j.contains("noteHeight"))			m.noteHeight			= j["noteHeight"];
	if (j.contains("gridDivision"))			m.gridDivision			= j["gridDivision"];
	if (j.contains("gridSwing"))			m.gridSwing				= j["gridSwing"];
	if (j.contains("showGridLines"))		m.showGridLines			= j["showGridLines"];
	if (j.contains("showNoteLabels"))		m.showNoteLabels		= j["showNoteLabels"];
	if (j.contains("colorNotesByChannel"))	m.colorNotesByChannel	= j["colorNotesByChannel"];
	if (j.contains("defaultChannel"))		m.defaultChannel		= j["defaultChannel"];
	if (j.contains("filterByChannel"))		m.filterByChannel		= j["filterByChannel"];

	return true;
}

bool EditorSerializer::LoadTimelineSettings(ProjectState& state, const std::filesystem::path& dir)
{
	const json j = ReadJson(dir / "timeline.json");
	if (j.empty()) return false;

	TimelineSettings& t = state.settings.timelineSettings;

	if (j.contains("defaultTrackHeight"))		t.defaultTrackHeight	= j["defaultTrackHeight"];
	if (j.contains("trackHeaderWidth"))			t.trackHeaderWidth		= j["trackHeaderWidth"];
	if (j.contains("defaultBeatWidth"))			t.defaultBeatWidth		= j["defaultBeatWidth"];
	if (j.contains("defaultInstanceLength"))	t.defaultInstanceLength	= j["defaultInstanceLength"];

	return true;
}

bool EditorSerializer::LoadPluginSettings(ProjectState& state, const std::filesystem::path& dir)
{
	const json j = ReadJson(dir / "plugins.json");
	if (j.empty()) return false;

	PluginSettings& p = state.settings.pluginSettings;

	if (j.contains("pluginPaths"))				p.pluginPaths				= j["pluginPaths"].get<std::vector<std::string>>();
	if (j.contains("blackList"))				p.blackList					= j["blackList"].get<std::vector<std::string>>();
	if (j.contains("sandboxPlugins"))			p.sandboxPlugins			= j["sandboxPlugins"];
	if (j.contains("pluginWindowScale"))		p.pluginWindowScale			= j["pluginWindowScale"];
	if (j.contains("openPluginWindowOnInsert"))	p.openPluginWindowOnInsert	= j["openPluginWindowOnInsert"];
	if (j.contains("dockPluginWindows"))		p.dockPluginWindows			= j["dockPluginWindows"];
	if (j.contains("sortAlphabetically"))		p.sortAlphabetically		= j["sortAlphabetically"];
	if (j.contains("groupByVendor"))			p.groupByVendor				= j["groupByVendor"];
	if (j.contains("groupByCategory"))			p.groupByCategory			= j["groupByCategory"];

	return true;
}