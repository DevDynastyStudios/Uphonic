#pragma once
#include <filesystem>

class ProjectState;
struct AudioTrack;

class ProjectSerializer
{
public:
	static bool SaveProject(const ProjectState& state, const std::filesystem::path& projectPath);
	static bool SaveSettings(const ProjectState& state, const std::filesystem::path& projectPath);
	
	static bool SavePatterns(const ProjectState& state, const std::filesystem::path& projectPath);
	static bool SaveSamples(const ProjectState& state, const std::filesystem::path& projectPath);
	static bool SaveAutomation(const ProjectState& state, const std::filesystem::path& projectPath);
	
	static bool SaveTracks(const ProjectState& state, const std::filesystem::path& projectPath);
	static bool SaveTrack(const AudioTrack& track, int trackIndex, const std::filesystem::path& trackDir);
	static bool SaveTrackMetadata(const AudioTrack& track, int trackIndex, const std::filesystem::path& trackDir);
	static bool SaveTrackBlocks(const AudioTrack& track, int trackIndex, const std::filesystem::path& trackDir);
	static bool SaveTrackAutomation(const AudioTrack& track, int trackIndex, const std::filesystem::path& trackDir);
	
	static bool LoadProject(ProjectState& state, const std::filesystem::path& projectPath);
	static bool LoadSettings(ProjectState& state, const std::filesystem::path& projectPath);
	static bool LoadPatterns(ProjectState& state, const std::filesystem::path& projectPath);
	static bool LoadSamples(ProjectState& state, const std::filesystem::path& projectPath);
	
	static bool LoadTracks(ProjectState& state, const std::filesystem::path& projectPath);
	static bool LoadTrack(AudioTrack& track, const std::filesystem::path& trackDir);
	static bool LoadTrackMetadata(AudioTrack& track, const std::filesystem::path& trackDir);
	static bool LoadTrackBlocks(AudioTrack& track, const std::filesystem::path& trackDir);
	static bool LoadTrackAutomation(AudioTrack& state, const std::filesystem::path& projectPath);
};
