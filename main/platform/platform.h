#pragma once

#include "base.h"
#include <ctime>
#include <filesystem>

struct UphPlatformCreateInfo
{
    uint32_t width, height;
    const char* title;
};

void uph_platform_initialize(const UphPlatformCreateInfo *create_info);
void uph_platform_shutdown(void);

void uph_platform_begin(void);
void uph_platform_end(void);

double uph_get_time(void);
std::tm uph_localtime(std::time_t t);
std::tm uph_localtime(const std::filesystem::file_time_type& ftime);