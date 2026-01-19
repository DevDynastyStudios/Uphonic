#pragma once

#include <functional>
#include <filesystem>

struct DialogState {
    bool open = false;
    bool folderMode = false;
    bool editingPath = false;
    bool editingJustActivated = false;
    bool hasPendingDir = false;
    std::filesystem::path pendingDir;
    std::string key;
    std::string title;
    std::string filters;
    std::filesystem::path currentDir;
    std::filesystem::path selected;
    std::string searchText;
    std::function<void(const std::filesystem::path&)> callback;
};

class FileDialog
{

public:
	static void OpenFile(const char* key, const char* title, const char* filters);
	static void OpenFolder(const char* key, const char* filters);

	static void Display(const char* key, const std::function<void(const std::filesystem::path& path)>& callback);

private:
	static void DrawBreadcrumb();
	static void DrawFileTable();
	static void PrintTime(const std::filesystem::file_time_type& ft);
	static std::string FormatBytes(uint64_t bytes);

	static inline DialogState state;
};
