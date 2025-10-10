#pragma once

#include <filesystem>
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

UphLibrary uph_load_library(const char *path);
void uph_unload_library(UphLibrary library);
UphProcAddress uph_get_proc_address(UphLibrary library, const char *name);
std::filesystem::path uph_open_file_dialog(const wchar_t* filter, const wchar_t* title);
std::filesystem::path uph_save_file_dialog(const wchar_t* filter, const wchar_t* title, const wchar_t* default_name = nullptr);
std::filesystem::path uph_select_folder_dialog(const wchar_t* title);