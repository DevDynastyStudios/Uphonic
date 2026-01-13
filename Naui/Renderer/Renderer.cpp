#include "Renderer.h"

#if NAUI_PLATFORM_WINDOWS
    #include "Direct3D11.h"
#elif NAUI_PLATFORM_LINUX
    #include "OpenGL.h"
#endif

namespace Naui
{

Renderer *CreateRenderer(const PlatformWindow &window)
{
#if NAUI_PLATFORM_WINDOWS
    return new Direct3D11Renderer(window);
#elif NAUI_PLATFORM_LINUX
    return new OpenGLRenderer(window);
#else
    return nullptr;
#endif
}

}
