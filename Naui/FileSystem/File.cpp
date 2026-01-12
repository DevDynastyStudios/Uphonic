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

void Directory::SetWorkspaceDirectory(const std::filesystem::path& path) {
	if (path.empty())
		return;

	workspaceDirectory = path;
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