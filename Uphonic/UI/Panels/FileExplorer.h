#pragma once

#include "Naui.h"
#include "Core/ProjectState.h"
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>

namespace fs = std::filesystem;

class FileExplorer : public Naui::Panel
{
public:
	FileExplorer();

protected:
	void OnRender() override;

private:
	struct FileItem
	{
		std::string name;
		std::string extension;
		fs::path fullPath;
		bool isDirectory;
		uintmax_t size;
	};

	void RenderToolbar();
	void RenderFileList();
	void RenderInfoPanel();
	void RefreshDirectory();
	bool PassesFilter(const FileItem& item);
	bool IsAudioFile(const std::string& ext);
	std::string FormatFileSize(uintmax_t size);
	void NavigateToPath(const std::string& path);
	void NavigateUp();
	void NavigateBack();
	void NavigateForward();
	void OnFileSelected(const fs::path& filePath);
	
	fs::path m_currentPath;
	std::vector<FileItem> m_files;
	char m_pathInput[512];
	std::vector<fs::path> m_history;
	size_t m_historyIndex;
	char m_searchFilter[256];
	int m_selectedIndex;
};

