#include "../types.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

static json serialize_note(const UphNote& n) 
{
    return 
	{
        {"start", n.start},
        {"length", n.length},
        {"key", n.key},
        {"velocity", n.velocity}
    };
}

static json serialize_pattern(const UphMidiPattern& p) 
{
    json j;
    j["name"] = p.name;

    for (auto& note : p.notes)
        j["notes"].push_back(serialize_note(note));

    return j;
}

static json serialize_timeline_block(const UphTimelineBlock& b) 
{
    json j;
    j["track_type"] = (b.track_type == UphTrackType_Midi ? "midi" : "sample");
    j["start_time"] = b.start_time;
    j["start_offset"] = b.start_offset;
    j["length"] = b.length;

    if (b.track_type == UphTrackType_Midi)
        j["pattern_index"] = b.pattern_index;
    else 
	{
        j["sample_index"] = b.sample_index;
        j["stretch_scale"] = b.stretch_scale;
    }

    return j;
}

static json serialize_track(const UphTrack& t) {
    json j;
    j["name"]   = t.name;
    j["volume"] = t.volume;
    j["pan"]    = t.pan;
    j["pitch"]  = t.pitch;
    j["muted"]  = t.muted;
    j["color"]  = t.color;
    j["track_type"] = (t.track_type == UphTrackType_Midi ? "midi" : "sample");
    j["instrument_path"] = t.instrument.path;

    for (auto& block : t.timeline_blocks)
        j["timeline_blocks"].push_back(serialize_timeline_block(block));

    return j;
}

static json serialize_project(const UphProject& p) 
{
    json j;
    j["volume"] = p.volume;
    j["bpm"]    = p.bpm;

    for (auto& pat : p.patterns)
        j["patterns"].push_back(serialize_pattern(pat));

    for (auto& s : p.samples) 
	{
        json js;
        js["name"] = s.name;
        js["type"] = (s.type == UphSampleType_Mono ? "mono" : "stereo");
        js["sample_rate"] = s.sample_rate;
        js["path"] = std::string("samples/") + s.name; // relative path
        j["samples"].push_back(js);
    }

    for (auto& t : p.tracks)
        j["tracks"].push_back(serialize_track(t));

    return j;
}

static UphNote deserialize_note(const json& jn) 
{
    UphNote n{};
    n.start    = jn.value("start", 0.0f);
    n.length   = jn.value("length", 0.0f);
    n.key      = jn.value("key", 60);
    n.velocity = jn.value("velocity", 100);
    return n;
}

static UphMidiPattern deserialize_pattern(const json& jp) 
{
    UphMidiPattern p{};
    strncpy(p.name, jp.value("name", "").c_str(), sizeof(p.name)-1);
    if (jp.contains("notes")) 
	{
        for (auto& jn : jp["notes"])
            p.notes.push_back(deserialize_note(jn));
    }

    return p;
}

static UphTimelineBlock deserialize_timeline_block(const json& jb) 
{
    UphTimelineBlock b{};
    std::string type = jb.value("track_type", "midi");
    if (type == "midi") 
	{
        b.track_type    = UphTrackType_Midi;
        b.pattern_index = jb.value("pattern_index", 0);

    } else 
	{
        b.track_type    = UphTrackType_Sample;
        b.sample_index  = jb.value("sample_index", 0);
        b.stretch_scale = jb.value("stretch_scale", 1.0f);
    }

    b.start_time   = jb.value("start_time", 0.0);
    b.start_offset = jb.value("start_offset", 0.0f);
    b.length       = jb.value("length", 1.0f);
    return b;
}

static UphTrack deserialize_track(const json& jt) {
    UphTrack t{};

    strncpy(t.name, jt.value("name", "").c_str(), sizeof(t.name)-1);
    t.volume     = jt.value("volume", 1.0f);
    t.pan        = jt.value("pan", 0.0f);
    t.pitch      = jt.value("pitch", 0.0f);
    t.muted      = jt.value("muted", false);
    t.color      = jt.value("color", 0xFFFFFFFF);
    t.track_type = (jt.value("track_type", "midi") == "midi") ? UphTrackType_Midi : UphTrackType_Sample;

    if (jt.contains("instrument_path")) 
	{
        auto pathStr = jt["instrument_path"].get<std::string>();
        strncpy(t.instrument.path, pathStr.c_str(), sizeof(t.instrument.path) - 1);
        t.instrument.path[sizeof(t.instrument.path) - 1] = '\0';
    }

    if (jt.contains("timeline_blocks")) {
        for (auto& jb : jt["timeline_blocks"])
            t.timeline_blocks.push_back(deserialize_timeline_block(jb));
    }

    return t;
}

static UphProject deserialize_project(const json& j) 
{
    UphProject p{};
    p.volume = j.value("volume", 0.5f);
    p.bpm    = j.value("bpm", 120.0f);

    if (j.contains("patterns")) 
	{
        for (auto& jp : j["patterns"])
            p.patterns.push_back(deserialize_pattern(jp));
    }

    if (j.contains("samples")) {
        for (auto& js : j["samples"]) {
            UphSample s{};
            strncpy(s.name, js.value("name", "").c_str(), sizeof(s.name)-1);
            s.type        = (js.value("type", "mono") == "stereo") ? UphSampleType_Stereo : UphSampleType_Mono;
            s.sample_rate = js.value("sample_rate", 44100.0f);
            // path is stored in JSON, but you’ll need to load frames separately
            p.samples.push_back(std::move(s));
        }
    }

    if (j.contains("tracks")) 
	{
        for (auto& jt : j["tracks"])
            p.tracks.push_back(deserialize_track(jt));
    }

    return p;
}

void uph_project_serializer_save_json(const std::filesystem::path& path, const char* file_name) 
{ 
	std::filesystem::path manifest = path / file_name;
    json j = serialize_project(app->project);
   	std::ofstream out(manifest, std::ios::trunc);

    if (!out.is_open()) 
	{
        std::cerr << "Failed to open " << manifest << " for writing\n";
        return;
    }

    out << j.dump(4);
    out.flush();
	out.close();
}

void uph_project_serializer_load_json(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        std::cerr << "No file found at " << path << "\n";
        return;
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Failed to open " << path << " for reading\n";
        return;
    }

    json j;
    in >> j;

    app->project = deserialize_project(j);
}
