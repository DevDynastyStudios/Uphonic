#include "ProjectSerializer.h"
#include "../Plugin/PluginManager.h"
#include "../Audio/AudioEngine.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;

bool ProjectSerializer::Save(const std::filesystem::path& projectPath)
{
    try
    {
        if (std::filesystem::exists(projectPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(projectPath))
            {
                try
                {
                    if (entry.path().filename() == "Samples")
                        continue;
                    
                    if (entry.is_directory())
                    {
                        std::filesystem::remove_all(entry.path());
                    }
                    else
                    {
                        std::filesystem::remove(entry.path());
                    }
                }
                catch (...)
                {
                }
            }
        }
        
        std::filesystem::create_directories(projectPath);
        
        std::filesystem::path samplesDir = projectPath / "Samples";
        std::filesystem::path pluginsDir = projectPath / "PluginData";
        std::filesystem::create_directories(samplesDir);
        std::filesystem::create_directories(pluginsDir);
        
        ProjectState& state = ProjectState::GetInstance();
        json projectJson;
        
        SerializeToJson(state, projectJson, projectPath);
        
        std::filesystem::path jsonPath = projectPath / "project.json";
        std::ofstream jsonFile(jsonPath);
        if (!jsonFile.is_open())
        {
            return false;
        }
        jsonFile << projectJson.dump(4);
        jsonFile.close();
        
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool ProjectSerializer::Load(const std::filesystem::path& projectPath)
{
    try
    {
        std::filesystem::path jsonPath = projectPath / "project.json";
        if (!std::filesystem::exists(jsonPath))
        {
            return false;
        }
        
        std::ifstream jsonFile(jsonPath);
        if (!jsonFile.is_open())
        {
            return false;
        }
        
        json projectJson;
        jsonFile >> projectJson;
        jsonFile.close();
        
        ProjectState& state = ProjectState::GetInstance();
        
        DeserializeFromJson(state, projectJson, projectPath);
        
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

void ProjectSerializer::SerializeToJson(const ProjectState& state, nlohmann::json& j, const std::filesystem::path& projectDir)
{
    j["version"] = "1.0";
    j["bpm"] = state.beatsPerMinute;
    j["masterVolume"] = state.masterVolume;
    j["timelinePositionBeats"] = state.timelinePositionBeats;
    j["currentMidiPatternIndex"] = state.currentMidiPatternIndex;
    
    j["patterns"] = json::array();
    for (size_t i = 0; i < state.patterns.size(); ++i)
    {
        const MidiPattern& pattern = state.patterns[i];
        json patternJson;
        patternJson["name"] = pattern.name;
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
        
        j["patterns"].push_back(patternJson);
    }
    
    j["samples"] = json::array();
    std::filesystem::path samplesDir = projectDir / "Samples";
    for (size_t i = 0; i < state.samples.size(); ++i)
    {
        const AudioSample& sample = state.samples[i];
        json sampleJson;
        sampleJson["name"] = sample.name;
        sampleJson["channelType"] = static_cast<int>(sample.channelType);
        sampleJson["sampleRate"] = sample.sampleRate;
        sampleJson["frameCount"] = sample.frameCount;
        
        std::string fileName;
        if (!sample.filePath.empty())
        {
            std::filesystem::path sourcePath(sample.filePath);
            
            if (!sourcePath.is_absolute())
            {
                sourcePath = std::filesystem::absolute(sourcePath);
            }
            
            fileName = sourcePath.filename().string();
            
            if (std::filesystem::exists(sourcePath))
            {
                std::filesystem::path destPath = samplesDir / fileName;
                bool needsCopy = true;
                
                try
                {
                    if (std::filesystem::exists(destPath))
                    {
                        std::filesystem::path canonicalSource = std::filesystem::canonical(sourcePath);
                        std::filesystem::path canonicalDest = std::filesystem::canonical(destPath);
                        if (canonicalSource == canonicalDest)
                        {
                            needsCopy = false;
                        }
                    }
                    
                    if (needsCopy)
                    {
                        std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing);
                    }
                }
                catch (...)
                {

                }
            }
        }
        else
        {
            fileName = sample.name;
        }
        
        if (!fileName.empty())
        {
            sampleJson["fileName"] = fileName;
        }
        
        j["samples"].push_back(sampleJson);
    }
    
    j["tracks"] = json::array();
    for (size_t trackIdx = 0; trackIdx < state.tracks.size(); ++trackIdx)
    {
        const AudioTrack& track = state.tracks[trackIdx];
        json trackJson;
        trackJson["name"] = track.name;
        trackJson["type"] = static_cast<int>(track.type);
        trackJson["volume"] = track.volume;
        trackJson["pan"] = track.pan;
        trackJson["muted"] = track.muted;
        trackJson["solo"] = track.solo;
        trackJson["armed"] = track.armed;
        trackJson["color"] = { track.color.x, track.color.y, track.color.z, track.color.w };
        
        if (track.instrument.plugin && !track.instrument.pluginPath.empty())
        {
            trackJson["instrument"]["hasPlugin"] = true;
            trackJson["instrument"]["path"] = track.instrument.pluginPath;
            
            std::filesystem::path pluginDataPath = projectDir / "PluginData" / ("instrument_" + std::to_string(trackIdx) + ".bin");
            try
            {
                track.instrument.plugin->Serialize(pluginDataPath.string().c_str());
                trackJson["instrument"]["dataPath"] = std::filesystem::relative(pluginDataPath, projectDir).string();
            }
            catch (...)
            {

            }
        }
        else
        {
            trackJson["instrument"]["hasPlugin"] = false;
        }
        
        trackJson["effects"] = json::array();
        for (size_t i = 0; i < track.effects.size(); ++i)
        {
            const PluginEffect& effect = track.effects[i];
            json effectJson;
            effectJson["index"] = i;
            if (effect.plugin && !effect.pluginPath.empty())
            {
                effectJson["hasPlugin"] = true;
                effectJson["path"] = effect.pluginPath;
                
                std::filesystem::path pluginDataPath = projectDir / "PluginData" / ("track_" + std::to_string(trackIdx) + "_effect_" + std::to_string(i) + ".bin");
                try
                {
                    effect.plugin->Serialize(pluginDataPath.string().c_str());
                    effectJson["dataPath"] = std::filesystem::relative(pluginDataPath, projectDir).string();
                }
                catch (...)
                {

                }
            }
            else
            {
                effectJson["hasPlugin"] = false;
            }
            trackJson["effects"].push_back(effectJson);
        }
        
        trackJson["blocks"] = json::array();
        for (const TimelineBlock& block : track.blocks)
        {
            json blockJson;
            if (track.type == TrackType::Midi)
            {
                blockJson["type"] = "midi";
                blockJson["startBeat"] = block.midiBlock.startBeat;
                blockJson["startOffsetBeats"] = block.midiBlock.startOffsetBeats;
                blockJson["lengthBeats"] = block.midiBlock.lengthBeats;
                blockJson["patternIndex"] = block.midiBlock.patternIndex;
            }
            else
            {
                blockJson["type"] = "sample";
                blockJson["startBeat"] = block.sampleBlock.startBeat;
                blockJson["startOffsetBeats"] = block.sampleBlock.startOffsetBeats;
                blockJson["lengthBeats"] = block.sampleBlock.lengthBeats;
                blockJson["stretchScale"] = block.sampleBlock.stretchScale;
                blockJson["sampleIndex"] = block.sampleBlock.sampleIndex;
            }
            trackJson["blocks"].push_back(blockJson);
        }
        
        j["tracks"].push_back(trackJson);
    }
    
    json masterJson;
    masterJson["volume"] = state.masterTrack.volume;
    masterJson["pan"] = state.masterTrack.pan;
    masterJson["muted"] = state.masterTrack.muted;
    masterJson["solo"] = state.masterTrack.solo;
    masterJson["armed"] = state.masterTrack.armed;
    
    masterJson["effects"] = json::array();
    std::filesystem::path pluginsDir = projectDir / "PluginData";
    for (size_t i = 0; i < state.masterTrack.effects.size(); ++i)
    {
        const PluginEffect& effect = state.masterTrack.effects[i];
        json effectJson;
        effectJson["index"] = i;
        if (effect.plugin && !effect.pluginPath.empty())
        {
            effectJson["hasPlugin"] = true;
            effectJson["path"] = effect.pluginPath;
            
            std::filesystem::path pluginDataPath = pluginsDir / ("master_effect_" + std::to_string(i) + ".bin");
            try
            {
                effect.plugin->Serialize(pluginDataPath.string().c_str());
                effectJson["dataPath"] = std::filesystem::relative(pluginDataPath, projectDir).string();
            }
            catch (...)
            {

            }
        }
        else
        {
            effectJson["hasPlugin"] = false;
        }
        masterJson["effects"].push_back(effectJson);
    }
    j["masterTrack"] = masterJson;
}

void ProjectSerializer::DeserializeFromJson(ProjectState& state, const nlohmann::json& j, const std::filesystem::path& projectDir)
{
    if (j.contains("bpm")) state.beatsPerMinute = j["bpm"];
    if (j.contains("masterVolume")) state.masterVolume = j["masterVolume"];
    if (j.contains("timelinePositionBeats")) state.timelinePositionBeats = j["timelinePositionBeats"];
    if (j.contains("currentMidiPatternIndex")) state.currentMidiPatternIndex = j["currentMidiPatternIndex"];
    
    if (j.contains("patterns"))
    {
        state.patterns.clear();
        for (const json& patternJson : j["patterns"])
        {
            MidiPattern pattern;
            if (patternJson.contains("name")) pattern.name = patternJson["name"];
            
            if (patternJson.contains("notes"))
            {
                for (const json& noteJson : patternJson["notes"])
                {
                    MidiNote note;
                    if (noteJson.contains("startBeat")) note.startBeat = noteJson["startBeat"];
                    if (noteJson.contains("lengthBeats")) note.lengthBeats = noteJson["lengthBeats"];
                    if (noteJson.contains("keyNumber")) note.keyNumber = noteJson["keyNumber"];
                    if (noteJson.contains("velocity")) note.velocity = noteJson["velocity"];
                    pattern.notes.push_back(note);
                }
            }
            state.patterns.push_back(pattern);
        }
    }
    
    if (j.contains("samples"))
    {
        state.samples.clear();
        std::filesystem::path samplesDir = projectDir / "Samples";
        for (const json& sampleJson : j["samples"])
        {
            std::string fileName;
            
            if (sampleJson.contains("fileName"))
            {
                fileName = sampleJson["fileName"];
            }
            else if (sampleJson.contains("filePath"))
            {
                std::filesystem::path oldPath = sampleJson["filePath"];
                fileName = oldPath.filename().string();
            }
            
            if (!fileName.empty())
            {
                std::filesystem::path filePath = samplesDir / fileName;
                
                if (std::filesystem::exists(filePath))
                {
                    AudioSample sample = AudioEngine::LoadSample(filePath.string().c_str());
                    state.samples.push_back(sample);
                }
            }
        }
    }
    
    if (j.contains("tracks"))
    {
        state.tracks.clear();
        std::filesystem::path pluginsDir = projectDir / "PluginData";
        for (size_t trackIdx = 0; trackIdx < j["tracks"].size(); ++trackIdx)
        {
            const json& trackJson = j["tracks"][trackIdx];
            AudioTrack track;
            if (trackJson.contains("name")) track.name = trackJson["name"];
            if (trackJson.contains("type")) track.type = static_cast<TrackType>(trackJson["type"]);
            if (trackJson.contains("volume")) track.volume = trackJson["volume"];
            if (trackJson.contains("pan")) track.pan = trackJson["pan"];
            if (trackJson.contains("muted")) track.muted = trackJson["muted"];
            if (trackJson.contains("solo")) track.solo = trackJson["solo"];
            if (trackJson.contains("armed")) track.armed = trackJson["armed"];
            if (trackJson.contains("color"))
            {
                track.color.x = trackJson["color"][0];
                track.color.y = trackJson["color"][1];
                track.color.z = trackJson["color"][2];
                track.color.w = trackJson["color"][3];
            }
            
            if (trackJson.contains("instrument") && trackJson["instrument"].contains("hasPlugin") && trackJson["instrument"]["hasPlugin"])
            {
                if (trackJson["instrument"].contains("path"))
                {
                    std::string pluginPath = trackJson["instrument"]["path"];
                    if (std::filesystem::exists(pluginPath))
                    {
                        PluginManager::LoadEffect(track.instrument, pluginPath);
                        
                        if (trackJson["instrument"].contains("dataPath"))
                        {
                            std::string dataPath = trackJson["instrument"]["dataPath"];
                            if (!std::filesystem::path(dataPath).is_absolute())
                            {
                                dataPath = (projectDir / dataPath).string();
                            }
                            if (std::filesystem::exists(dataPath) && track.instrument.plugin)
                            {
                                track.instrument.plugin->Deserialize(dataPath.c_str());
                            }
                        }
                    }
                }
            }
            
            if (trackJson.contains("effects"))
            {
                for (const json& effectJson : trackJson["effects"])
                {
                    if (effectJson.contains("hasPlugin") && effectJson["hasPlugin"])
                    {
                        if (effectJson.contains("path"))
                        {
                            std::string pluginPath = effectJson["path"];
                            if (std::filesystem::exists(pluginPath))
                            {
                                PluginEffect effect;
                                PluginManager::LoadEffect(effect, pluginPath);
                                
                                if (effectJson.contains("dataPath"))
                                {
                                    std::string dataPath = effectJson["dataPath"];
                                    if (!std::filesystem::path(dataPath).is_absolute())
                                    {
                                        dataPath = (projectDir / dataPath).string();
                                    }
                                    if (std::filesystem::exists(dataPath) && effect.plugin)
                                    {
                                        effect.plugin->Deserialize(dataPath.c_str());
                                    }
                                }
                                
                                track.effects.push_back(effect);
                            }
                        }
                    }
                }
            }
            
            if (trackJson.contains("blocks"))
            {
                for (const json& blockJson : trackJson["blocks"])
                {
                    TimelineBlock block;
                    std::string blockType = blockJson.contains("type") ? blockJson["type"] : "midi";
                    if (blockType == "midi")
                    {
                        if (blockJson.contains("startBeat")) block.midiBlock.startBeat = blockJson["startBeat"];
                        if (blockJson.contains("startOffsetBeats")) block.midiBlock.startOffsetBeats = blockJson["startOffsetBeats"];
                        if (blockJson.contains("lengthBeats")) block.midiBlock.lengthBeats = blockJson["lengthBeats"];
                        if (blockJson.contains("patternIndex")) block.midiBlock.patternIndex = blockJson["patternIndex"];
                    }
                    else
                    {
                        if (blockJson.contains("startBeat")) block.sampleBlock.startBeat = blockJson["startBeat"];
                        if (blockJson.contains("startOffsetBeats")) block.sampleBlock.startOffsetBeats = blockJson["startOffsetBeats"];
                        if (blockJson.contains("lengthBeats")) block.sampleBlock.lengthBeats = blockJson["lengthBeats"];
                        if (blockJson.contains("stretchScale")) block.sampleBlock.stretchScale = blockJson["stretchScale"];
                        if (blockJson.contains("sampleIndex")) block.sampleBlock.sampleIndex = blockJson["sampleIndex"];
                    }
                    track.blocks.push_back(block);
                }
            }
            
            state.tracks.push_back(track);
        }
    }
    
    if (j.contains("masterTrack"))
    {
        const json& masterJson = j["masterTrack"];
        if (masterJson.contains("volume")) state.masterTrack.volume = masterJson["volume"];
        if (masterJson.contains("pan")) state.masterTrack.pan = masterJson["pan"];
        if (masterJson.contains("muted")) state.masterTrack.muted = masterJson["muted"];
        if (masterJson.contains("solo")) state.masterTrack.solo = masterJson["solo"];
        if (masterJson.contains("armed")) state.masterTrack.armed = masterJson["armed"];
        
        if (masterJson.contains("effects"))
        {
            std::filesystem::path pluginsDir = projectDir / "PluginData";
            for (const json& effectJson : masterJson["effects"])
            {
                if (effectJson.contains("hasPlugin") && effectJson["hasPlugin"])
                {
                    if (effectJson.contains("path"))
                    {
                        std::string pluginPath = effectJson["path"];
                        if (std::filesystem::exists(pluginPath))
                        {
                            PluginEffect effect;
                            PluginManager::LoadEffect(effect, pluginPath);
                            
                            if (effectJson.contains("dataPath"))
                            {
                                std::string dataPath = effectJson["dataPath"];
                                if (!std::filesystem::path(dataPath).is_absolute())
                                {
                                    dataPath = (projectDir / dataPath).string();
                                }
                                if (std::filesystem::exists(dataPath) && effect.plugin)
                                {
                                    effect.plugin->Deserialize(dataPath.c_str());
                                }
                            }
                            
                            state.masterTrack.effects.push_back(effect);
                        }
                    }
                }
            }
        }
    }
}