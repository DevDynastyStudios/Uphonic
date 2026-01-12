#pragma once

#include "Base.h"
#include <filesystem>

namespace Naui
{

class NAUI_API Platform
{
public:
    static std::filesystem::path GetExecutablePath(void);
};

}