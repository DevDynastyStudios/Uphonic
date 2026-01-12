#pragma once

#include "Core.h"
#include <filesystem>

class PluginLoader
{
public:
    static void LoadEffect(Effect &effect, const std::filesystem::path &path);
    static void OpenEffect(Effect &effect);
    static void UnloadEffect(Effect &effect);
};