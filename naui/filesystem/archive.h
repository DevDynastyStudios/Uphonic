#pragma once

#define NAUI_ARCHIVE_MAGIC "NauiPak"
#define NAUI_ARCHIVE_MAGIC_SIZE (sizeof(NAUI_ARCHIVE_MAGIC) - 1)
#define NAUI_ARCHIVE_VERSION 1

#define NAUI_ARCHIVE_INIT { {0}, 0, false }

typedef uint8_t Naui_ArchiveMode;
enum
{
	NAUI_ARCHIVE_MODE_READ,
	NAUI_ARCHIVE_MODE_WRITE
};

typedef struct Naui_ArchiveEntry
{
	Naui_Path path;
	uint64_t size;
	bool is_directory;
} Naui_ArchiveEntry;

typedef struct Naui_Archive
{
	mz_zip_archive zip;
	Naui_ArchiveMode mode;
	bool is_valid;
} Naui_Archive;

bool naui_archive_open(Naui_Archive* archive, const Naui_Path path, Naui_ArchiveMode mode);
void naui_archive_move(Naui_Archive* dst, Naui_Archive* src);
void naui_archive_close(Naui_Archive* archive);
bool naui_archive_is_valid(const Naui_Archive* archive);

/* Add all files under folder recursively, prefixed with root_in_archive. */
bool naui_archive_add_folder(Naui_Archive* archive, const Naui_Path folder, const Naui_Path root_in_archive);

/* Add a single file into the archive at dest_in_archive. */
bool naui_archive_add_file(Naui_Archive* archive, const Naui_Path source, const Naui_Path dest_in_archive);

/* Extract all entries to output_folder. */
bool naui_archive_extract_to(Naui_Archive* archive, const Naui_Path output_folder);

/* Extract a single named entry to dest. */
bool naui_archive_extract_file(Naui_Archive* archive, const Naui_Path entry, const Naui_Path dest);

/* List all entries.
 * Call naui_archive_list_free() when done. */
Naui_List(Naui_ArchiveEntry) naui_archive_list_entries(Naui_Archive* archive);
void naui_archive_list_free(Naui_List(Naui_ArchiveEntry) list);

bool naui_archive_create_custom(const Naui_Path folder, const Naui_Path archive_path);
bool naui_archive_extract_custom(const Naui_Path archive_path, const Naui_Path output_folder);