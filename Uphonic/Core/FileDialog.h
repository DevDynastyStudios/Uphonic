#pragma once

#include <functional>
#include <filesystem>

enum class DialogMode
{
	OpenFile,
	OpenFolder,
	SaveFile
};

struct DialogState {
	bool open = false;
	bool editingPath = false;
	bool editingJustActivated = false;
	bool hasPendingDir = false;
	bool canConfirm = false;
	DialogMode mode;
	std::filesystem::path pendingDir;
	std::string key;
	std::string title;
	std::string confirmLabel = "";
	std::string filters;
	std::filesystem::path currentDir;
	std::filesystem::path selected;
	std::string searchText;
	std::function<void(const std::filesystem::path&)> callback;
};

class FileDialog
{

public:
	static void SaveFile(const char* key, const char* title, const char* filters, const char* confirmLabel = "Save As");
	static void OpenFile(const char* key, const char* title, const char* filters, const char* confirmLabel = "Open");
	static void OpenFolder(const char* key, const char* filters, const char* confirmLabel = "Select Folder");

	static void Display(const char* key, const std::function<void(const std::filesystem::path& path)>& callback);

private:
	static void DrawBreadcrumb();
	static void DrawFileTable();
	static void PrintTime(const std::filesystem::file_time_type& ft);
	static std::string FormatBytes(uint64_t bytes);

	static inline DialogState state;
};
