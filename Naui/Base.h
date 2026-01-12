#pragma once

namespace Naui
{

#if defined(_WIN32) || defined(_WIN64)
    #define NAUI_PLATFORM_WINDOWS 1
#elif defined(__APPLE__) || defined(__MACH__)
    #define NAUI_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define NAUI_PLATFORM_LINUX 1
#endif

#if defined(NAUI_PLATFORM_WINDOWS)
#if defined(NAUI_EXPORT)
    #define NAUI_API __declspec(dllexport)
#else
    #define NAUI_API __declspec(dllimport)
#endif
#else
    #define NAUI_API
#endif

}