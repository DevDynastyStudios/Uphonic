#pragma once
#include <imgui.h>

namespace Naui
{
    class Style
    {
    public:
        static void Load(const char* path);
        static void LoadDefault();
    };
}
