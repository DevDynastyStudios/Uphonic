#pragma once

#include <functional>
#include <filesystem>

class FileDialog
{
public:
    static void OpenFile(const char* title, const char* filters);
    static void OpenFolder(const char* title, const char* filters);

    static void Display(const char* title, const std::function<void(const std::filesystem::path& path)>& callback);
};