#pragma once

#include <cstdint>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define UPH_PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(__gnu_linux__)
#define UPH_PLATFORM_LINUX 1
#endif