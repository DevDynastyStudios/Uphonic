#pragma once

#include "Renderer/Renderer.h"

namespace Naui
{

class NAUI_API AssetManager
{
public:
    static void Initialize(Renderer &renderer, const char *assets_directory = "Assets");
    static void Shutdown(Renderer &renderer);

    static Image &GetImage(const char *name);
};

}