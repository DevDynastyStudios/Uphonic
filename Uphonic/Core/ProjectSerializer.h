#pragma once

#include "ProjectState.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>

class ProjectSerializer
{
public:
    static bool Save(const std::filesystem::path& projectPath);
    static bool Load(const std::filesystem::path& projectPath);

private:
    static void SerializeToJson(const ProjectState& state, nlohmann::json& json, const std::filesystem::path& projectDir);
    static void DeserializeFromJson(ProjectState& state, const nlohmann::json& json, const std::filesystem::path& projectDir);
};
