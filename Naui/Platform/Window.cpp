#include "Window.h"
#include "Base.h"

#if NAUI_PLATFORM_WINDOWS
    #include "Win32.h"
#endif

namespace Naui
{

PlatformWindow *CreatePlatformWindow(int width, int height, const char *title, PlatformWindow *parent)
{
#if NAUI_PLATFORM_WINDOWS
    return new PlatformWin32Window(width, height, title, parent);
#else
    return nullptr;
#endif
}

}