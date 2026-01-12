#pragma once

#include "Base.h"
#include <imgui.h>

#include <cstdint>

namespace Naui
{

class NAUI_API Image
{
public:
    virtual ~Image(void) = default;

    virtual uint32_t GetWidth(void) const = 0;
    virtual uint32_t GetHeight(void) const = 0;
    virtual void *GetNativeHandle(void) const = 0;
    
    operator ImTextureRef(void) const { return ImTextureRef((ImTextureID)GetNativeHandle()); }
};

}