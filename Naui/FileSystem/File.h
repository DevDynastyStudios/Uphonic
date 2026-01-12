#pragma once

#include "Base.h"
#include <filesystem>
#include <cstdio>
#include <string>
#include <vector>

namespace Naui {

enum class FileMode { Read, Write, Append };

class NAUI_API File
{
public:
	File(const std::filesystem::path& path, FileMode mode);
	~File();

	File(const File&) = delete;
	File& operator=(const File&) = delete;
	File(File&& other) noexcept;
	File& operator=(File&& other) noexcept;

	bool IsValid() const;
	size_t Read(void* buffer, size_t size);
	size_t Write(const void* buffer, size_t size);
	void Seek(long offset, int origin);
	void Close();

private:
	FILE* handle_;
};

struct NAUI_API DirEntry
{
	std::filesystem::path path;
	bool isDirectory;
	size_t size;
};

class NAUI_API Directory
{
public:
	static std::filesystem::path HomeDirectory();
	static std::filesystem::path BinDirectory();
	static std::filesystem::path WorkingDirectory();
	static std::filesystem::path WorkspaceDirectory();
	static void SetWorkspaceDirectory(const std::filesystem::path& path);
	static std::string GetEnv(const char* name);
	static std::vector<DirEntry> Filter(const std::filesystem::path& path, std::string_view nameFilter, const std::vector<std::string_view>& allowedExtensions);

private:
	static std::filesystem::path workspaceDirectory;
};

class NAUI_API PathUtils
{
public:
	static void RemoveExtension(std::string& filename);
	static std::string GetParent(const std::string& path);
};

}