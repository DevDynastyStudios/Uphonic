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

#if NAUI_PLATFORM_LINUX

#include <unistd.h>

std::filesystem::path Platform::GetExecutablePath(void)
{
    const size_t pathSize = 1024;
    char path[pathSize] = {};
    ssize_t ok = readlink("/proc/self/exe", path, pathSize);
    if (ok == -1)
    {
        fprintf(stderr, "failed to get executable path\n");
        return {};
    }
    return std::filesystem::path(path);
}

#endif

}
