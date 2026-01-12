#include "Archive.h"
#include <fstream>
#include <tuple>
#include <iostream>

#define MAGIC_STRING "NauiPak"
#define MAGIC_STRING_SIZE (sizeof(MAGIC_STRING) - 1)

namespace Naui {

Archive::Archive(const std::filesystem::path& file, ArchiveMode mode) : zip_{}, mode_(mode), isValid_(false)
{
	memset(&zip_, 0, sizeof(zip_));
	if (mode == ArchiveMode::Read)
		isValid_ = mz_zip_reader_init_file(&zip_, file.string().c_str(), 0) != 0;
	else
		isValid_ = mz_zip_writer_init_file(&zip_, file.string().c_str(), 0) != 0;

	if (!isValid_) {
		std::cerr << "[Naui] Failed to open archive " << file << "\n";
	}
}

Archive::~Archive() {
	if (!isValid_) 
		return;

	if (mode_ == ArchiveMode::Read) {
		mz_zip_reader_end(&zip_);
	} else {
		mz_zip_writer_finalize_archive(&zip_);
		mz_zip_writer_end(&zip_);
	}
}

Archive::Archive(Archive&& other) noexcept : zip_(other.zip_), mode_(other.mode_), isValid_(other.isValid_)
{
	memset(&other.zip_, 0, sizeof(other.zip_));
	other.isValid_ = false;
}

Archive& Archive::operator=(Archive&& other) noexcept {
	if (this != &other) {
		this->~Archive(); // clean up current
		zip_ = other.zip_;
		mode_ = other.mode_;
		isValid_ = other.isValid_;
		memset(&other.zip_, 0, sizeof(other.zip_));	//(Chimpchi): Don't hurt me Box ;-;
		other.isValid_ = false;
	}

	return *this;
}

bool Archive::IsValid() const 
{
	return isValid_; 
}

bool Archive::AddFile(const std::filesystem::path& source, const std::filesystem::path& destInArchive)
{
	if (!isValid_ || mode_ != ArchiveMode::Write)
		return false;

	return mz_zip_writer_add_file(&zip_, destInArchive.string().c_str(), source.string().c_str(), nullptr, 0, MZ_BEST_SPEED) != 0;
}

bool Archive::ExtractFile(const std::filesystem::path& entry, const std::filesystem::path& dest)
{
	if (!isValid_ || mode_ != ArchiveMode::Read)
		return false;

	return mz_zip_reader_extract_file_to_file(&zip_, entry.string().c_str(), dest.string().c_str(), 0) != 0;
}

std::vector<ArchiveEntry> Archive::ListEntries() {
	std::vector<ArchiveEntry> entries;
	if (!isValid_ || mode_ != ArchiveMode::Read) 
		return entries;

	int count = (int)mz_zip_reader_get_num_files(&zip_);
	for (int i = 0; i < count; ++i) {
		mz_zip_archive_file_stat fileStat;
		if(!mz_zip_reader_file_stat(&zip_, i, &fileStat))
			continue;

		ArchiveEntry entry;
		entry.path = fileStat.m_filename;
		entry.size = fileStat.m_uncomp_size;
		entry.isDirectory = mz_zip_reader_is_file_a_directory(&zip_, i) != 0;
		entries.push_back(std::move(entry));
	}

	return entries;
}

bool Archive::CreateCustom(const std::filesystem::path& folder, const std::filesystem::path& archivepath)
{
	std::vector<std::tuple<std::filesystem::path,uint64_t,uint64_t>> entries;
	std::vector<char> fileData;

	for (auto& entry : std::filesystem::recursive_directory_iterator(folder)) {
		if (entry.is_directory())
			continue;

		std::ifstream in(entry.path(), std::ios::binary);
		std::vector<char> buffer((std::istreambuf_iterator<char>(in)), {});
		uint64_t offset = fileData.size();
		uint64_t size = buffer.size();

		fileData.insert(fileData.end(), buffer.begin(), buffer.end());
		entries.emplace_back(std::filesystem::relative(entry.path(), folder), offset, size);
	}

	std::ofstream out(archivepath, std::ios::binary);
	out.write(MAGIC_STRING, MAGIC_STRING_SIZE);
	uint32_t version = 1;
	uint64_t count = entries.size();
	out.write((char*)&version, sizeof(version));
	out.write((char*)&count, sizeof(count));

	for (auto& [relpath, offset, size] : entries) {
		std::string rel = relpath.generic_string();
		uint16_t len = (uint16_t)rel.size();
		out.write((char*)&len, sizeof(len));
		out.write(rel.data(), len);
		out.write((char*)&offset, sizeof(offset));
		out.write((char*)&size, sizeof(size));
	}

	out.write(fileData.data(), fileData.size());
	return true;
}

bool Archive::ExtractCustom(const std::filesystem::path& archivepath, const std::filesystem::path& outputFolder)
{
	std::ifstream in(archivepath, std::ios::binary);
	char magic[MAGIC_STRING_SIZE];
	in.read(magic, MAGIC_STRING_SIZE);
	if (std::string(magic, MAGIC_STRING_SIZE) != MAGIC_STRING)
		return false;

	uint32_t version;
	uint64_t count;
	in.read((char*)&version, sizeof(version));
	in.read((char*)&count, sizeof(count));

	struct Entry { std::string path; uint64_t offset; uint64_t size; };
	std::vector<Entry> entries(count);

	for (uint64_t i=0; i<count; ++i) {
		uint16_t len;
		in.read((char*)&len, sizeof(len));
		std::string rel(len, '\0');
		in.read(rel.data(), len);
		uint64_t offset, size;
		in.read((char*)&offset, sizeof(offset));
		in.read((char*)&size, sizeof(size));
		entries[i] = {rel, offset, size};
	}

	std::streampos dataStart = in.tellg();

	for (auto& e : entries) {
		std::filesystem::path outpath = outputFolder / e.path;
		std::filesystem::create_directories(outpath.parent_path());

		std::vector<char> buffer(e.size);
		in.seekg(dataStart + (std::streamoff)e.offset);
		in.read(buffer.data(), e.size);

		std::ofstream out(outpath, std::ios::binary);
		out.write(buffer.data(), buffer.size());
	}

	return true;
}

}