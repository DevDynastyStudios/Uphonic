#pragma once

#include "../Core/ProjectState.h"
#include "../UVI/UVILoader.h"
#include <filesystem>

class PluginManager
{
public:
    static void LoadEffect(PluginEffect& effect, const std::filesystem::path& path);
    static void OpenEffect(PluginEffect& effect);
    static void UnloadEffect(PluginEffect& effect);
	static void UnloadAllEffects();
};

