#pragma once

#include "base.h"

typedef void *UphLibrary;
typedef void *UphProcAddress;

struct UphPlatformCreateInfo
{
    uint32_t width, height;
    const char *title;
};

struct UphChildWindowCreateInfo
{
    uint32_t width, height;
    const char *title;
};

struct UphChildWindow
{
    void *handle;
};

void uph_platform_initialize(const UphPlatformCreateInfo *create_info);
void uph_platform_shutdown(void);

void uph_platform_begin(void);
void uph_platform_end(void);

UphChildWindow uph_create_child_window(const UphChildWindowCreateInfo *create_info);
void uph_destroy_child_window(const UphChildWindow *window);

double uph_get_time(void);

UphLibrary uph_load_library(const char *path);
UphProcAddress uph_get_proc_address(UphLibrary library, const char *name);