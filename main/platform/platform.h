#pragma once

#include "base.h"

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