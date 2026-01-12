#pragma once

#include "Base.h"
#include <imgui.h>

namespace Naui
{

class NAUI_API Theme
{
public:
    static void Load(const char* path);
    static void LoadDefault(void);
    static ImColor GetColor(const char* name);
};

};