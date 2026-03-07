#pragma once
#include <filesystem>

class ProjectState;

class EditorSerializer
{
public:
	static bool SaveSettings(const ProjectState& state);
	static bool SaveGeneralSettings(const ProjectState& state, const std::filesystem::path& dir);
	static bool SaveAudioSettings(const ProjectState& state, const std::filesystem::path& dir);
	static bool SaveMidiSettings(const ProjectState& state, const std::filesystem::path& dir);
	static bool SaveTimelineSettings(const ProjectState& state, const std::filesystem::path& dir);
	static bool SavePluginSettings(const ProjectState& state, const std::filesystem::path& dir);

	static bool LoadSettings(ProjectState& state);
	static bool LoadGeneralSettings(ProjectState& state, const std::filesystem::path& dir);
	static bool LoadAudioSettings(ProjectState& state, const std::filesystem::path& dir);
	static bool LoadMidiSettings(ProjectState& state, const std::filesystem::path& dir);
	static bool LoadTimelineSettings(ProjectState& state, const std::filesystem::path& dir);
	static bool LoadPluginSettings(ProjectState& state, const std::filesystem::path& dir);

private:
	static std::filesystem::path SettingsDir();
};