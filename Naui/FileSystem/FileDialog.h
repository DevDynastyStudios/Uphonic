#pragma once

#include "Base.h"
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

class NAUI_API FileDialog
{
public:
	static void SaveFile(const std::string& key, const std::string& title, const std::string& filters, const std::string& confirmLabel = "Save As");
	static void OpenFile(const std::string& key, const std::string& title, const std::string& filters, const std::string& confirmLabel = "Open");
	static void OpenFolder(const std::string& key, const std::string& filters, const std::string& confirmLabel = "Select Folder");

	static void Display(const std::string& key, const std::function<void(const std::filesystem::path& path)>& callback);

private:
	static void ConfirmSaveFile(const char* typedName);
	static void DrawBreadcrumb();
	static void DrawFileTable();
	static void PrintTime(const std::filesystem::file_time_type& ft);
	static std::string FormatBytes(uint64_t bytes);

	static inline DialogState state;
};
