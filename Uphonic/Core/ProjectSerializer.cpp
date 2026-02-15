#include "ProjectSerializer.h"
#include "../Plugin/PluginManager.h"
#include "../Audio/AudioEngine.h"
#include "Naui/FileSystem/File.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <unordered_set>
#include <iostream>

using json = nlohmann::json;

#define PROJECT_FILENAME "project.json"
#define PATTERN_FILENAME "patterns.json"
#define SAMPLE_FILENAME "samples.json"
#define TRACK_META_FILENAME "metadata.json"
#define TRACK_BLOCK_FILENAME "blocks.json"
#define TRACK_AUTOMATION_FILENAME "automation.json"
#define INSTRUMENT_FILENAME "instrument.json"

#pragma region Serialize Helpers
static bool WriteJsonFile(const std::filesystem::path& path, const json& j)
{
	std::error_code err;
	std::filesystem::create_directories(path.parent_path(), err);
	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	if(!out.is_open())
	{
		std::cout << "Unable to write json file " << path.filename() << "\n";
		return false;
	}

	out << j.dump(4);
	return true;
}

static bool ReadJsonFile(const std::filesystem::path& path, json& j)
{
	std::ifstream in(path, std::ios::binary);
	if(!in.is_open())
	{
		std::cout << "Failed to read json file " << path.filename() << "\n";
		return false;
	}

	in >> j;
	return true;
}

static std::string FormatFolderName(const char* folderName, int trackIndex)
{
	char buf[64];
	std::snprintf(buf, sizeof(buf), "%s_%04d", folderName, trackIndex);
	return std::string(buf);
}
#pragma endregion

#pragma region Serialize Functions
bool ProjectSerializer::SaveProject(const ProjectState& state, const std::filesystem::path& projectPath)
{
	std::filesystem::create_directories(projectPath / "Patterns");
	std::filesystem::create_directories(projectPath / "Samples");
	std::filesystem::create_directories(projectPath / "Tracks");

	bool saved = true;
	saved &= SaveSettings(state, projectPath); 
	saved &= SavePatterns(state, projectPath / "Patterns");
	saved &= SaveSamples(state, projectPath / "Samples");
	saved &= SaveTracks(state, projectPath / "Tracks");
	return saved;
}

bool ProjectSerializer::SaveSettings(const ProjectState& state, const std::filesystem::path& settingsDir)
{
	if (std::filesystem::exists(settingsDir) && !std::filesystem::is_directory(settingsDir))
		return false;

	json settings;
	settings["version"] = "1.0";
	settings["projectName"] = state.settings.projectName;
	settings["bpm"] = state.beatsPerMinute;
	settings["masterVolume"] = state.masterVolume;
	settings["timelinePositionBeats"] = state.timelinePositionBeats;
	settings["currentMidiPatternIndex"] = state.currentMidiPatternIndex;
	return WriteJsonFile(settingsDir / PROJECT_FILENAME, settings);
}

bool ProjectSerializer::SavePatterns(const ProjectState& state, const std::filesystem::path& patternDir)
{
	if (std::filesystem::exists(patternDir) && !std::filesystem::is_directory(patternDir))
		return false;

	std::filesystem::create_directories(patternDir);
	json patterns = json::array();
	for (size_t i = 0; i < state.patterns.size(); ++i)
	{
		const MidiPattern& pattern = state.patterns[i];
		json patternJson;
		patternJson["name"] = pattern.name;
		patternJson["color"] = { pattern.color.x, pattern.color.y, pattern.color.z, pattern.color.w };
		patternJson["notes"] = json::array();
		for (const MidiNote& note : pattern.notes)
		{
			json noteJson;
			noteJson["startBeat"] = note.startBeat;
			noteJson["lengthBeats"] = note.lengthBeats;
			noteJson["keyNumber"] = note.keyNumber;
			noteJson["velocity"] = note.velocity;
			patternJson["notes"].push_back(noteJson);
		}
		
		patterns.push_back(patternJson);
	}

	return WriteJsonFile(patternDir / PATTERN_FILENAME, patterns);
}

bool ProjectSerializer::SaveSamples(const ProjectState& state, const std::filesystem::path& samplesDir)
{
	if (std::filesystem::exists(samplesDir) && !std::filesystem::is_directory(samplesDir))
		return false;

	std::filesystem::create_directories(samplesDir);
	json samples = json::array();
	for(const AudioSample& sample : state.samples)
	{
		json sampleJson;
		sampleJson["name"] = sample.name;
		sampleJson["audioFileName"] = sample.filename;
		sampleJson["channelType"] = static_cast<int>(sample.channelType);
		sampleJson["sampleRate"] = sample.sampleRate;
		sampleJson["frameCount"] = sample.frameCount;
		sampleJson["color"] = { sample.color.x, sample.color.y, sample.color.z, sample.color.w };
		samples.push_back(sampleJson);
	}

	return WriteJsonFile(samplesDir / SAMPLE_FILENAME, samples);
}

bool ProjectSerializer::SaveAutomation(const ProjectState& state, const std::filesystem::path& projectPath)
{
	// if (std::filesystem::exists(projectPath) && !std::filesystem::is_directory(projectPath))
	// 	return false;
	
	// (Chimpchi): Add automation in the future

	return true;
}
#pragma endregion

#pragma region Track Serialization
bool ProjectSerializer::SaveTracks(const ProjectState& state, const std::filesystem::path& trackDir)
{
	if (!std::filesystem::exists(trackDir) || !std::filesystem::is_directory(trackDir))
		return false;

	bool success = true;
	for(size_t i = 0; i < state.tracks.size(); i++)
	{
		const AudioTrack& track = state.tracks[i];
		std::filesystem::path currentTrackDir = trackDir / ("Track_" + std::to_string(i));
		std::filesystem::create_directories(currentTrackDir);
		if(!SaveTrack(track, (int) i, currentTrackDir))
			success = false;
	}
	
	return success;
}

bool ProjectSerializer::SaveTrack(const AudioTrack& track, int trackIndex, const std::filesystem::path& trackDir)
{
	if(!std::filesystem::exists(trackDir) || !std::filesystem::is_directory(trackDir))
	{
		std::cout << "Unable to save to " << trackDir << "\n";
		return false;
	}

	bool saved = true;
	saved &= SaveTrackMetadata(track, trackIndex, trackDir);
	saved &= SaveTrackBlocks(track, trackIndex, trackDir);
	saved &= SaveTrackAutomation(track, trackIndex, trackDir);
	return saved;
}

bool ProjectSerializer::SaveTrackMetadata(const AudioTrack& track, int trackIndex, const std::filesystem::path& trackDir)
{
	if (std::filesystem::exists(trackDir) && !std::filesystem::is_directory(trackDir))
		return false;

	std::filesystem::create_directories(trackDir.parent_path());
	json metaData;
	metaData["name"] = track.name;
	metaData["type"] = static_cast<int>(track.type);
	metaData["volume"] = track.volume;
	metaData["pan"] = track.pan;
	metaData["muted"] = track.muted;
	metaData["solo"] = track.solo;
	metaData["color"] = { track.color.x, track.color.y, track.color.z, track.color.w };

	json instrument;
	instrument["hasPlugin"] = track.instrument.plugin != nullptr;
	if(track.instrument.plugin)
	{
		//instrument["id"] = track.instrument.pluginID;
		//instrument["vendor"] = track.instrument.vendorName;
		//instrument["name"] = track.instrument.pluginName;
		//instrument["version"] = track.instrument.pluginVersion;

		instrument["path"] = track.instrument.pluginPath;
		try
		{
			track.instrument.plugin->Serialize((trackDir / INSTRUMENT_FILENAME).string().c_str());
			instrument["dataPath"] = INSTRUMENT_FILENAME;
		}
		catch(const std::exception& e)
		{
			instrument["dataPath"] = "";
			std::cerr << e.what() << '\n';
		}
	}

	metaData["instrument"] = instrument;
	return WriteJsonFile(trackDir / TRACK_META_FILENAME, metaData);
}

bool ProjectSerializer::SaveTrackBlocks(const AudioTrack& track, int trackIndex, const std::filesystem::path& trackDir)
{
	if (std::filesystem::exists(trackDir) && !std::filesystem::is_directory(trackDir))
		return false;

	std::filesystem::create_directories(trackDir.parent_path());
	json blockJson = json::array();
	if (track.type == TrackType::Midi)
	{
		for(const TimelineBlock& block : track.blocks)
		{
			json note;
			note["startBeat"] = block.midiBlock.startBeat;
			note["startOffsetBeats"] = block.midiBlock.startOffsetBeats;
			note["lengthBeats"] = block.midiBlock.lengthBeats;
			note["patternIndex"] = block.midiBlock.patternIndex;
			blockJson.push_back(note);
		}
	}
	else if (track.type == TrackType::Audio)
	{
		for(const TimelineBlock& block : track.blocks)
		{
			json note;
			note["startBeat"] = block.sampleBlock.startBeat;
			note["startOffsetBeats"] = block.sampleBlock.startOffsetBeats;
			note["lengthBeats"] = block.sampleBlock.lengthBeats;
			note["stretchScale"] = block.sampleBlock.stretchScale;
			note["sampleIndex"] = block.sampleBlock.sampleIndex;
			blockJson.push_back(note);
		}
	}

	return WriteJsonFile(trackDir / TRACK_BLOCK_FILENAME, blockJson);
}

bool ProjectSerializer::SaveTrackAutomation(const AudioTrack& track, int trackIndex, const std::filesystem::path& trackDir)
{
	return true;
}
#pragma endregion

#pragma region Deserialize Functions

bool ProjectSerializer::LoadProject(ProjectState& state, const std::filesystem::path& projectPath)
{
	bool loaded = true;
	loaded &= LoadSettings(state, projectPath);
	loaded &= LoadPatterns(state, projectPath / "Patterns");
	loaded &= LoadSamples(state, projectPath / "Samples");
	loaded &= LoadTracks(state, projectPath / "Tracks");
	return loaded;
}

bool ProjectSerializer::LoadSettings(ProjectState& state, const std::filesystem::path& projectPath)
{
	std::filesystem::path file = std::filesystem::weakly_canonical(projectPath / PROJECT_FILENAME);
	if(!std::filesystem::exists(file))
	{
		std::cout << "Unable to load settings: " << file << "\n";
		return false;
	}

	json settings;
	if(!ReadJsonFile(file, settings))
		return false;

	state.settings.projectName = settings["projectName"];
	state.beatsPerMinute = settings["bpm"];		
	state.masterVolume = settings["masterVolume"];
	state.timelinePositionBeats = settings["timelinePositionBeats"];
	state.currentMidiPatternIndex = settings["currentMidiPatternIndex"];
	return true;
}

bool ProjectSerializer::LoadPatterns(ProjectState& state, const std::filesystem::path& patternDir)
{
	std::filesystem::path file = std::filesystem::weakly_canonical(patternDir / PATTERN_FILENAME);
	if(!std::filesystem::exists(file))
	{
		std::cout << "Unable to load pattern: " << file << "\n";
		return false;
	}

	json patterns;
	if(!ReadJsonFile(file, patterns))
		return false;

	state.patterns.clear();
	for (const auto& patternJson : patterns)
	{
		MidiPattern pattern;
		pattern.name = patternJson.value("name", "");
		auto c = patternJson.value("color", std::vector<float>{1,1,1,1});
		if (c.size() == 4)
			pattern.color = { c[0], c[1], c[2], c[3] };

		for (const auto& noteJson : patternJson["notes"])
		{
			MidiNote note;
			note.startBeat = noteJson.value("startBeat", 0.0f);
			note.lengthBeats = noteJson.value("lengthBeats", 0.0f);
			note.keyNumber = noteJson.value("keyNumber", 60);
			note.velocity = noteJson.value("velocity", 100);
			pattern.notes.push_back(note);
		}
		
		state.patterns.push_back(pattern);
	}

	return true;
}

bool ProjectSerializer::LoadSamples(ProjectState& state, const std::filesystem::path& sampleDir)
{
	std::filesystem::path file = std::filesystem::weakly_canonical(sampleDir / SAMPLE_FILENAME);
	if(!std::filesystem::exists(file))
	{
		std::cout << "Unable to load samples: " << file << "\n";
		return false;
	}

	json samples;
	if(!ReadJsonFile(file, samples))
		return false;

	state.samples.clear();
	for(const auto& sampleJson : samples)
	{
		const std::filesystem::path sampleFilename = sampleDir / sampleJson.value("audioFileName", "");
		if(!std::filesystem::exists(sampleFilename))
			continue;

		AudioSample& sample = AudioEngine::AddSample(sampleFilename.string().c_str(), state);
		sample.name = sampleJson.value("name", sampleFilename.filename().string());
		sample.channelType = static_cast<SampleChannelType>(sampleJson.value("channelType", 0));
		sample.sampleRate = sampleJson.value("sampleRate", 44100);	// (Chimpchi): Change this to grab the file and get the sample rate from that.
		sample.frameCount = sampleJson.value("frameCount", 0);
		sample.color.x = sampleJson["color"][0];
		sample.color.y = sampleJson["color"][1];
		sample.color.z = sampleJson["color"][2];
		sample.color.w = sampleJson["color"][3];
	}

	return true;
}

bool ProjectSerializer::LoadTracks(ProjectState& state, const std::filesystem::path& trackDir)
{
	if(!std::filesystem::exists(trackDir))
	{
		std::cout << "Unable to load track: " << trackDir << "\n";
		return false;
	}
	
	if(!std::filesystem::is_directory(trackDir))
	{
		std::cout << "Path is not a directory: " << trackDir << "\n";
		return false;
	}

	std::vector<std::filesystem::path> dirs;
	for(const auto& entry : std::filesystem::directory_iterator(trackDir))
	{
		if(entry.is_directory())
		{

			dirs.push_back(entry.path());
		}
	}

	std::sort(dirs.begin(), dirs.end());
	state.tracks.clear();
	for(const std::filesystem::path& path : dirs)
	{
		AudioTrack track;
		if(LoadTrack(track, path))
			state.tracks.push_back(track);
		else
			return false;
	}

	return true;
}

bool ProjectSerializer::LoadTrack(AudioTrack& track, const std::filesystem::path& trackDir)
{
	bool loaded = true;
	loaded &= LoadTrackMetadata(track, trackDir / TRACK_META_FILENAME);
	loaded &= LoadTrackBlocks(track, trackDir / TRACK_BLOCK_FILENAME);
	loaded &= LoadTrackAutomation(track, trackDir / TRACK_AUTOMATION_FILENAME);
	return loaded;
}

static void OldInstrumentLoading(AudioTrack& track, json& metaJson, const std::filesystem::path& trackFolder)
{
	if (!metaJson.contains("instrument"))
		return;

	const auto& inst = metaJson["instrument"];
	if (!inst.value("hasPlugin", false))
		return;

	std::string pluginPath = inst.value("path", "");
	if (pluginPath.empty() || !std::filesystem::exists(pluginPath))
		return;

	PluginManager::LoadEffect(track.instrument, pluginPath);
	std::string dataPath = inst.value("dataPath", "");
	if (dataPath.empty() || !track.instrument.plugin)
		return;

	std::filesystem::path binPath = trackFolder / dataPath;
	if (std::filesystem::exists(binPath))
	{
		try
		{
			track.instrument.plugin->Deserialize(binPath.string().c_str());
		}
		catch (const std::exception& e)
		{
			std::cerr << "Failed to load instrument state: " << e.what() << "\n";
		}
	}
}

bool ProjectSerializer::LoadTrackMetadata(AudioTrack& track, const std::filesystem::path& metaDir)
{
	if(!std::filesystem::exists(metaDir) || std::filesystem::is_directory(metaDir))
		return false;

	json metaJson;
	if(!ReadJsonFile(metaDir, metaJson))
		return false;

	track.name = metaJson.value("name", "");
	track.type = static_cast<TrackType>(metaJson.value("type", 0));
	track.volume = metaJson.value("volume", 1.0f);
	track.pan = metaJson.value("pan", 0.0f);
	track.muted = metaJson.value("muted", false);
	track.solo = metaJson.value("solo", false);
	track.color.x = metaJson["color"][0];
	track.color.y = metaJson["color"][1];
	track.color.z = metaJson["color"][2];
	track.color.w = metaJson["color"][3];
	OldInstrumentLoading(track, metaJson, metaDir.parent_path());
	// if(metaJson.contains("instrument"))
	// {
	// 	const auto& instrument = metaJson["instrument"];
	// 	bool hasPlugin = instrument.value("hasPlugin", false);
	// 	if(hasPlugin)
	// 	{
	// 		//track.instrument.pluginID  = inst.value("id", "");
	// 		//track.instrument.vendorName= inst.value("vendor", "");
	// 		//track.instrument.pluginName= inst.value("name", "");
	// 		//track.instrument.pluginVersion = inst.value("version", "");
	// 		//track.instrument.pluginPath= inst.value("path", "");
	// 	}
	// }

	return true;
}

// Be sure to call LoadTrackMetadata before this function
bool ProjectSerializer::LoadTrackBlocks(AudioTrack& track, const std::filesystem::path& blockDir)
{
	if(!std::filesystem::exists(blockDir) || std::filesystem::is_directory(blockDir))
		return false;

	json blockArrJson;
	if(!ReadJsonFile(blockDir, blockArrJson))
		return false;

	for(const auto& blockJson : blockArrJson)
	{
		TimelineBlock block;
		if(track.type == TrackType::Midi)
		{
			block.midiBlock.startBeat = blockJson.value("startBeat", 0.0f);
			block.midiBlock.startOffsetBeats = blockJson.value("startOffsetBeats", 0.0f);
			block.midiBlock.lengthBeats = blockJson.value("lengthBeats", 0.0f);
			block.midiBlock.patternIndex = blockJson.value("patternIndex", 0);
		}
		else
		{
			block.sampleBlock.startBeat = blockJson.value("startBeat", 0.0f);
			block.sampleBlock.startOffsetBeats = blockJson.value("startOffsetBeats", 0.0f);
			block.sampleBlock.lengthBeats = blockJson.value("lengthBeats", 0.0f);
			block.sampleBlock.stretchScale = blockJson.value("stretchScale", 1.0f);
			block.sampleBlock.sampleIndex = blockJson.value("sampleIndex", 0);
		}

		track.blocks.push_back(block);
	}

	return true;
}

bool ProjectSerializer::LoadTrackAutomation(AudioTrack& state, const std::filesystem::path& automationDir)
{
	return true;
}
#pragma endregion