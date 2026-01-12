#include "Renderer.h"

#if NAUI_PLATFORM_WINDOWS
    #include "Direct3D11.h"
#endif

namespace Naui
{

Renderer *CreateRenderer(const PlatformWindow &window)
{
#if NAUI_PLATFORM_WINDOWS
    return new Direct3D11Renderer(window);
#else
    return nullptr;
#endif
}

}