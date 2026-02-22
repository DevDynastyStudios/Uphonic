#pragma once

#include "Base.h"
#include <filesystem>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>

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

struct LockHandle
{
	#if NAUI_PLATFORM_WINDOWS
		void* handle = nullptr;
	#else
		int fd = -1;
	#endif
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
	static std::filesystem::path AppDataDirectory();
	static std::filesystem::path DownloadsDirectory();
	static void SetWorkspaceDirectory(const std::filesystem::path& path, bool hidden = false);
	static std::filesystem::path HideDirectory(const std::filesystem::path& path, bool hidden = false);
	static std::string GetEnv(const char* name);
	static std::vector<DirEntry> Filter(const std::filesystem::path& path, std::string_view nameFilter, const std::vector<std::string_view>& allowedExtensions);

	static std::string ToUTF8(const std::u8string& s);
	static std::u8string ToU8(const std::string& s);
	static std::wstring UTF8ToUTF16(const std::string& utf8);
	static std::wstring UTF8ToUTF16(const std::u8string& u8);
	static std::wstring PathToUTF16(const std::filesystem::path& p);

	static std::string PathToUTF8(const std::filesystem::path& path);
	static std::filesystem::path UTF8ToPath(const std::string& utf8);
	static std::string GetFilename(const std::filesystem::path& path);

	static bool LockPath(const std::filesystem::path& path);
	static void UnlockPath(const std::filesystem::path& path);
	static bool IsLocked(const std::filesystem::path& path);

	static bool IsHidden(const std::filesystem::path& path);

private:
	static std::filesystem::path ResolveLockTarget(const std::filesystem::path& path);

	static std::filesystem::path workspaceDirectory;
	static std::unordered_map<std::filesystem::path, LockHandle> g_lockTable;
};

class NAUI_API PathUtils
{
public:
	static void RemoveExtension(std::string& filename);
	static std::string GetParent(const std::string& path);
};

}