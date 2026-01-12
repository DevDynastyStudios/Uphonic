#pragma once

#include "Base.h"
#include <filesystem>
#include <string>
#include <vector>
#include <miniz.h>

namespace Naui {

enum class ArchiveMode { Read, Write };

struct NAUI_API ArchiveEntry
{
	std::string path;
	uint64_t size;
	bool isDirectory;
};

class NAUI_API Archive
{
public:
	Archive(const std::filesystem::path& file, ArchiveMode mode);
	~Archive();

	Archive(const Archive&) = delete;
	Archive& operator=(const Archive&) = delete;
	Archive(Archive&& other) noexcept;
	Archive& operator=(Archive&& other) noexcept;

	bool IsValid() const;
	bool AddFile(const std::filesystem::path& source, const std::filesystem::path& destInArchive);
	bool ExtractFile(const std::filesystem::path& entry, const std::filesystem::path& dest);

	std::vector<ArchiveEntry> ListEntries();
	static bool CreateCustom(const std::filesystem::path& folder, const std::filesystem::path& archivePath);
	static bool ExtractCustom(const std::filesystem::path& archivePath, const std::filesystem::path& outputFolder);

private:
	mz_zip_archive zip_;
	ArchiveMode mode_;
	bool isValid_;
};

}