#include "File.h"
#include "Platform/Platform.h"

#include <fstream>
#include <iostream>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <cerrno>
#include <filesystem>

#if NAUI_PLATFORM_WINDOWS
#include <Windows.h>
#include <fileapi.h>
#include <shlobj.h>
#else
#include <sys/file.h>	// (Chimpchi): Smoke I don't know what I'm doing. plz help ;-;
#include <fcntl.h>
#include <unistd.h>
#endif

namespace Naui {

#if NAUI_PLATFORM_WINDOWS
#	define NAUI_PATH_MAX 260
#	define NAUI_ENV_HOME "HOMEPATH"
#	define NAUI_ENV_USER "USERPROFILE"
#else
#	define NAUI_PATH_MAX 4096
#	define NAUI_ENV_HOME "HOME"
#	define NAUI_ENV_USER "HOME"
#endif

std::filesystem::path Directory::workspaceDirectory{};
std::unordered_map<std::filesystem::path, LockHandle> Directory::g_lockTable;

#pragma region File
File::File(const std::filesystem::path& path, FileMode mode) : handle_(nullptr)
{
	const char* modeStr = nullptr;
	switch (mode) {
		case FileMode::Read:  modeStr = "rb"; break;
		case FileMode::Write: modeStr = "wb"; break;
		case FileMode::Append: modeStr = "ab"; break;
		default:
			throw std::invalid_argument("Invalid FileMode");
	}

#if defined(NAUI_COMPILER_MSVC)
	if (fopen_s(&handle_, path.string().c_str(), modeStr) != 0) {
		std::cerr << "[Naui] Failed to open file " << path.string() << "!\n";
		handle_ = nullptr;
	}
#else
	handle_ = fopen(path.string().c_str(), modeStr);
	if (!handle_) {
		std::cerr << "[Naui] Failed to open file " << path.string() << "!\n";
	}
#endif
}

File::~File() {
	Close();
}

File::File(File&& other) noexcept : handle_(other.handle_) {
	other.handle_ = nullptr;
}

File& File::operator=(File&& other) noexcept {
	if (this != &other) {
		Close();
		handle_ = other.handle_;
		other.handle_ = nullptr;
	}

	return *this;
}

bool File::IsValid() const {
	return handle_ != nullptr;
}

size_t File::Read(void* buffer, size_t size) {
	return handle_ ? fread(buffer, 1, size, handle_) : 0;
}

size_t File::Write(const void* buffer, size_t size) {
	return handle_ ? fwrite(buffer, 1, size, handle_) : 0;
}

void File::Seek(long offset, int origin) {
	if(!handle_)
		return;

	fseek(handle_, offset, origin);
}

void File::Close() {
	if(!handle_)
		return;
		
	fclose(handle_);
	handle_ = nullptr;
}

#pragma endregion

#pragma region Directory
std::filesystem::path Directory::HomeDirectory() {
	return GetEnv(NAUI_ENV_USER);
}

std::filesystem::path Directory::BinDirectory() {
	std::filesystem::path exePath = Platform::GetExecutablePath();
	return exePath.parent_path();
}

std::filesystem::path Directory::WorkingDirectory() {
	return std::filesystem::current_path();
}

std::filesystem::path Directory::WorkspaceDirectory() {
	return workspaceDirectory;
}

std::filesystem::path Directory::AppDataDirectory()
{
#if NAUI_PLATFORM_WINDOWS
	return std::filesystem::path(GetEnv("LOCALAPPDATA"));
#elif NAUI_PLATFORM_MACOS
	return HomeDirectory() / "Library/Application Support/";
#else
	return HomeDirectory() / ".local/share/";
#endif
}

std::filesystem::path Directory::DownloadsDirectory()
{
	return Directory::HomeDirectory() / "Downloads";		// (Chimpchi): This may be wrong, correct this later.
}

void Directory::SetWorkspaceDirectory(const std::filesystem::path& path, bool hidden) {
	if (path.empty() || path.has_extension())
		return;

	std::filesystem::path finalPath = path;
	std::filesystem::create_directories(finalPath);
	finalPath = Directory::HideDirectory(finalPath, hidden);
	workspaceDirectory = finalPath;
}

std::filesystem::path Directory::HideDirectory(const std::filesystem::path& path, bool hidden)
{
	if(path.empty())
	{
		std::cout << "Path does not exist.\nUnable to hide directory: " << path;
		return path;
	}

	if(IsHidden(path) == hidden)
		return path;

	std::filesystem::path finalPath = path;
#if NAUI_PLATFORM_WINDOWS
	DWORD attrs = GetFileAttributesA(finalPath.string().c_str());
	if(attrs == INVALID_FILE_ATTRIBUTES)
		return finalPath;

	attrs = hidden ? attrs | FILE_ATTRIBUTE_HIDDEN : attrs & ~FILE_ATTRIBUTE_HIDDEN;
	SetFileAttributesA(finalPath.string().c_str(), attrs);
	return finalPath;
#else
	std::string name = finalPath.filename().string();
	std::filesystem::path parent = finalPath.parent_path();

	if(name.empty())
		return finalPath;

	std::filesystem::path newPath = finalPath;
	if(hidden)
	{
		if(name[0] == '.')
			return finalPath;

		newPath = parent / ('.' + name);
	}
	else
	{
		if(name[0] != '.')
			return finalPath;
		
		newPath = parent / name.substr(1);
	}

	std::filesystem::rename(finalPath, newPath);
	return newPath;
#endif
}

std::string Directory::GetEnv(const char* name) {
#if defined(NAUI_COMPILER_MSVC)
	char* buffer = nullptr;
	size_t len = 0;
	if (_dupenv_s(&buffer, &len, name) == 0 && buffer) {
		std::string result(buffer);
		free(buffer);
		return result;
	}

	return {};
#else
	const char* env = std::getenv(name);
	return env ? std::string(env) : std::string{};
#endif
}

std::vector<DirEntry> Directory::Filter(const std::filesystem::path& path, std::string_view nameFilter, const std::vector<std::string_view>& allowedExtensions)
{
	std::vector<DirEntry> entries;
	if (!std::filesystem::exists(path)) 
		return entries;

	std::string nameFilterLower;
	nameFilterLower.reserve(nameFilter.size());
	for (char c : nameFilter)
		nameFilterLower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

	for (const std::filesystem::directory_entry& p : std::filesystem::directory_iterator(path)) {
		const std::string filename = p.path().filename().string();
		std::string filenameLower;
		filenameLower.reserve(filename.size());
		for (char c : filename)
			filenameLower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

		if (!nameFilterLower.empty() && filenameLower.find(nameFilterLower) == std::string::npos)
			continue;

		// Extension check
		bool extOk = allowedExtensions.empty();
		std::string extLower = p.path().extension().string();
		std::transform(extLower.begin(), extLower.end(), extLower.begin(), [](unsigned char c){ return std::tolower(c); });

		for (const std::string_view& allowed : allowedExtensions) {
			std::string allowedLower;
			allowedLower.reserve(allowed.size());
			for (char c : allowed)
				allowedLower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

			if (extLower == allowedLower) {
				extOk = true;
				break;
			}
		}

		if (!extOk)
			continue;

		DirEntry e;
		e.path = p.path();
		e.isDirectory = p.is_directory();
		e.size = e.isDirectory ? 0 : std::filesystem::file_size(p.path());
		entries.push_back(std::move(e));
	}

	return entries;
}

std::string Directory::ToUTF8(const std::u8string& s)
{
	return std::string(reinterpret_cast<const char*>(s.c_str()));
}

std::u8string Directory::ToU8(const std::string& s)
{
	return std::u8string(reinterpret_cast<const char8_t*>(s.c_str()));
}

std::wstring Directory::UTF8ToUTF16(const std::string& utf8)
{
	if (utf8.empty())
		return std::wstring();

#ifdef _WIN32
    if (utf8.empty()) return std::wstring();
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &result[0], sizeNeeded);
    return result;
#else
    std::wstring result(utf8.size(), L'\0');
    std::mbstate_t state{};
    const char* src = utf8.c_str();
    size_t len = std::mbsrtowcs(result.data(), &src, result.size(), &state);
    if (len == static_cast<size_t>(-1)) return L"";
    result.resize(len);
    return result;
#endif
}

std::wstring Directory::UTF8ToUTF16(const std::u8string& u8)
{
	return UTF8ToUTF16(ToUTF8(u8));
}

std::wstring Directory::PathToUTF16(const std::filesystem::path& p)
{
#ifdef _WIN32
    return p.native();
#else
    return UTF8ToUTF16(p.native());
#endif
}

// Convert filesystem path to UTF-8 string (safe on all platforms)
std::string Directory::PathToUTF8(const std::filesystem::path& path)
{
#ifdef NAUI_PLATFORM_WINDOWS
    std::wstring wstr = path.wstring();
    if (wstr.empty()) 
        return {};
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) 
        return {};
    
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
#else
    return path.string();
#endif
}
    
// Convert UTF-8 string to filesystem path (safe on all platforms)
std::filesystem::path Directory::UTF8ToPath(const std::string& utf8)
{
#ifdef NAUI_PLATFORM_WINDOWS
    if (utf8.empty()) 
        return {};
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 0) 
        return {};
    
    std::wstring wstr(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], size);
    return std::filesystem::path(wstr);
#else
    return std::filesystem::path(utf8);
#endif
}   

std::string Directory::GetFilename(const std::filesystem::path& path)
{
#ifdef NAUI_PLATFORM_WINDOWS
	return Directory::PathToUTF8(path.filename());
#else
    return path.filename().string();
#endif
}

bool Directory::IsHidden(const std::filesystem::path& path)
{
	if(path.empty())
		return false;

#if NAUI_PLATFORM_WINDOWS
	DWORD attrs = GetFileAttributesA(path.string().c_str());
	if(attrs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
#else
	std::string name = path.filename().string();
	return !name.empty() && name[0] == '.';
#endif
}

bool Directory::LockPath(const std::filesystem::path& path)
{
	std::filesystem::path target = Directory::ResolveLockTarget(path);
	if(target.filename() == ".lock")
		std::filesystem::create_directories(target.parent_path());

	LockHandle lock;
#if NAUI_PLATFORM_WINDOWS
	HANDLE winHandle = CreateFileW(
						target.wstring().c_str(),
						GENERIC_READ | GENERIC_WRITE,
						0,
						nullptr,
						OPEN_ALWAYS,
						FILE_ATTRIBUTE_HIDDEN,
						nullptr
					);

	if(winHandle == INVALID_HANDLE_VALUE)
		return false;

	OVERLAPPED overlap = {};
	if(!LockFileEx(winHandle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXWORD, MAXWORD, &overlap))
	{
		CloseHandle(winHandle);
		return false;
	}

	lock.handle = winHandle;
#else
	int fileDiscriptor = ::open(target.string().c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0666);
	if(fileDiscriptor == -1)
		return false;

	if(flock(fileDiscriptor, LOCK_EX | LOCK_NB) != 0)
	{
		::close(fileDiscriptor);
		return false;
	}

	lock.fd = fileDiscriptor;
#endif

	g_lockTable[target] = lock;
	return true;
}

void Directory::UnlockPath(const std::filesystem::path& path)
{
	std::filesystem::path target = Directory::ResolveLockTarget(path);
	auto it = g_lockTable.find(target);
	if(it == g_lockTable.end())
		return;

	LockHandle& lock = it->second;
#if NAUI_PLATFORM_WINDOWS
	if(lock.handle)
		CloseHandle(lock.handle);
#else
	if(lock.fd != -1)
	{
		flock(lock.fd, LOCK_UN);
		::close(lock.fd);
	}
#endif

	g_lockTable.erase(it);
}

bool Directory::IsLocked(const std::filesystem::path& path)
{
	std::filesystem::path target = Directory::ResolveLockTarget(path);
	if(!std::filesystem::exists(target))
		return false;

#if NAUI_PLATFORM_WINDOWS
	HANDLE winHandle = CreateFileW(
		target.wstring().c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_HIDDEN,
		nullptr
	);

	if(winHandle == INVALID_HANDLE_VALUE)
		return true;

	CloseHandle(winHandle);
	return false;
#else
	int fileDiscriptor = ::open(target.string().c_str(), O_RDWR | O_CLOEXEC);
	if(fileDiscriptor == -1)
		return false;

	bool locked = (flock(fileDiscriptor, LOCK_EX | LOCK_NB) != 0);
	if(!locked)
		flock(fileDiscriptor, LOCK_UN);

	::close(fileDiscriptor);
	return locked;
#endif
}

std::filesystem::path Directory::ResolveLockTarget(const std::filesystem::path& path)
{
	return std::filesystem::is_directory(path) ? path / ".lock" : path;
}

#pragma endregion

#pragma region PathUtils
void PathUtils::RemoveExtension(std::string& filename) {
	size_t dotPos = filename.find_last_of('.');
	if (dotPos != std::string::npos)
		filename = filename.substr(0, dotPos);
}

std::string PathUtils::GetParent(const std::string& path) {
	if (path.empty())
		return {};

	std::string parent = path;
	size_t pos = parent.find_last_of('/');

#if defined(NAUI_PLATFORM_WINDOWS)
	size_t back = parent.find_last_of('\\');
	if (back != std::string::npos && (pos == std::string::npos || back > pos))
		pos = back;
#endif

	if (pos != std::string::npos)
		parent.erase(pos);
	return parent;
}
#pragma endregion

}
