#include "Platform.h"

namespace Naui
{

#if NAUI_PLATFORM_WINDOWS

#include <windows.h>

std::filesystem::path Platform::GetExecutablePath(void)
{
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (length == 0)
        return {};
    return std::filesystem::path(path);
}

#else

#endif

}