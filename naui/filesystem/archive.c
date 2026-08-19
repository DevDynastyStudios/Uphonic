typedef struct
{
	char name[NAUI_PATH_MAX];
	uint64_t size;
	bool is_directory;
} Zip_EntryStat;

static bool zip_open_read(mz_zip_archive* zip, const char* path)
{
	return mz_zip_reader_init_file(zip, path, 0) != 0;
}

static bool zip_open_write(mz_zip_archive* zip, const char* path)
{
	return mz_zip_writer_init_file(zip, path, 0) != 0;
}

static void zip_close_read(mz_zip_archive* zip)
{
	mz_zip_reader_end(zip);
}

static void zip_close_write(mz_zip_archive* zip)
{
	mz_zip_writer_finalize_archive(zip);
	mz_zip_writer_end(zip);
}

static bool zip_add_file(mz_zip_archive* zip, const char* dest, const char* source)
{
	return mz_zip_writer_add_file(zip, dest, source, NULL, 0, MZ_BEST_SPEED) != 0;
}

static int zip_entry_count(mz_zip_archive* zip)
{
	return (int)mz_zip_reader_get_num_files(zip);
}

static bool zip_entry_stat(mz_zip_archive* zip, int index, Zip_EntryStat* out)
{
	mz_zip_archive_file_stat st;
	if (!mz_zip_reader_file_stat(zip, index, &st))
		return false;

	snprintf(out->name, NAUI_PATH_MAX, "%s", st.m_filename);
	out->size = (uint64_t)st.m_uncomp_size;
	out->is_directory = mz_zip_reader_is_file_a_directory(zip, index) != 0;
	return true;
}

static bool zip_extract_to_file(mz_zip_archive* zip, int index, const char* dest)
{
	return mz_zip_reader_extract_to_file(zip, index, dest, 0) != 0;
}

static bool zip_extract_entry_to_file(mz_zip_archive* zip, const char* entry, const char* dest)
{
	return mz_zip_reader_extract_file_to_file(zip, entry, dest, 0) != 0;
}

bool naui_archive_open(Naui_Archive* archive, const Naui_Path path, Naui_ArchiveMode mode)
{
	memset(&archive->zip, 0, sizeof(archive->zip));
	archive->mode = mode;
	archive->is_valid = false;

	if (mode == NAUI_ARCHIVE_MODE_READ)
		archive->is_valid = zip_open_read(&archive->zip, path.data);
	else
		archive->is_valid = zip_open_write(&archive->zip, path.data);

	if (!archive->is_valid)
		fprintf(stderr, "[Naui] Failed to open archive: %s\n", path.data);

	return archive->is_valid;
}

void naui_archive_move(Naui_Archive* dst, Naui_Archive* src)
{
	naui_archive_close(dst);
	memcpy(dst, src, sizeof(Naui_Archive));
	memset(&src->zip, 0, sizeof(src->zip));
	src->is_valid = false;
}

void naui_archive_close(Naui_Archive* archive)
{
	if (!archive->is_valid)
		return;

	if (archive->mode == NAUI_ARCHIVE_MODE_READ)
		zip_close_read(&archive->zip);
	else
		zip_close_write(&archive->zip);

	archive->is_valid = false;
}

bool naui_archive_is_valid(const Naui_Archive* archive)
{
	return archive->is_valid;
}

bool naui_archive_add_file(Naui_Archive* archive, const Naui_Path source, const Naui_Path dest_in_archive)
{
	if (!archive->is_valid || archive->mode != NAUI_ARCHIVE_MODE_WRITE)
		return false;

	return zip_add_file(&archive->zip, dest_in_archive.data, source.data);
}

bool naui_archive_add_folder(Naui_Archive* archive, const Naui_Path folder, const Naui_Path root_in_archive)
{
	if (!archive->is_valid || archive->mode != NAUI_ARCHIVE_MODE_WRITE)
		return false;

	Naui_List(Naui_DirEntry) entries = naui_directory_filter_recursive(folder, NULL, NULL);
	if (!entries)
		return true;

	bool ok = true;
	for (ptrdiff_t i = 0; i < naui_list_len(entries); ++i)
	{
		if (entries[i].is_directory)
			continue;

		const char* full = entries[i].path.data;
		const char* rel = full + strlen(folder.data);
		if (*rel == '/' || *rel == '\\')
			++rel;

		Naui_Path dest;
		if (root_in_archive.data[0] != '\0')
			snprintf(dest.data, NAUI_PATH_MAX, "%s/%s", root_in_archive.data, rel);
		else
			snprintf(dest.data, NAUI_PATH_MAX, "%s", rel);

		if (!naui_archive_add_file(archive, entries[i].path, dest))
		{
			ok = false;
			break;
		}
	}

	naui_directory_filter_free(entries);
	return ok;
}

bool naui_archive_extract_to(Naui_Archive* archive, const Naui_Path output_folder)
{
	if (!archive->is_valid || archive->mode != NAUI_ARCHIVE_MODE_READ)
		return false;

	int count = zip_entry_count(&archive->zip);
	for (int i = 0; i < count; ++i)
	{
		Zip_EntryStat st;
		if (!zip_entry_stat(&archive->zip, i, &st))
			continue;

		Naui_Path out;
		snprintf(out.data, NAUI_PATH_MAX, "%s/%s", output_folder.data, st.name);
		if (st.is_directory)
		{
			naui_directory_create(out);
			continue;
		}

		Naui_Path parent = naui_path_parent(out);
		naui_directory_create(parent);
		if (!zip_extract_to_file(&archive->zip, i, out.data))
			return false;
	}

	return true;
}

bool naui_archive_extract_file(Naui_Archive* archive, const Naui_Path entry, const Naui_Path dest)
{
	if (!archive->is_valid || archive->mode != NAUI_ARCHIVE_MODE_READ)
		return false;

	return zip_extract_entry_to_file(&archive->zip, entry.data, dest.data);
}

Naui_List(Naui_ArchiveEntry) naui_archive_list_entries(Naui_Archive* archive)
{
	Naui_List(Naui_ArchiveEntry) list = NULL;

	if (!archive->is_valid || archive->mode != NAUI_ARCHIVE_MODE_READ)
		return list;

	int count = zip_entry_count(&archive->zip);
	for (int i = 0; i < count; ++i)
	{
		Zip_EntryStat st;
		if (!zip_entry_stat(&archive->zip, i, &st))
			continue;

		Naui_ArchiveEntry entry;
		snprintf(entry.path.data, NAUI_PATH_MAX, "%s", st.name);
		entry.size = st.size;
		entry.is_directory = st.is_directory;
		naui_list_push(list, entry);
	}

	return list;
}

void naui_archive_list_free(Naui_List(Naui_ArchiveEntry) list)
{
	if (list)
		naui_list_free(list);
}

bool naui_archive_create_custom(const Naui_Path folder, const Naui_Path archive_path)
{
	Naui_List(Naui_DirEntry) entries = naui_directory_filter_recursive(folder, NULL, NULL);

	size_t blob_cap = 1024 * 1024;
	size_t blob_len = 0;
	uint8_t* blob = (uint8_t*)malloc(blob_cap);
	if (!blob)
	{
		naui_directory_filter_free(entries);
		return false;
	}

	typedef struct
	{
		char name[NAUI_PATH_MAX];
		uint64_t offset;
		uint64_t size;
	} IndexEntry;

	size_t idx_cap = 64;
	size_t idx_len = 0;
	IndexEntry* index = (IndexEntry*)malloc(idx_cap * sizeof(IndexEntry));
	if (!index)
	{
		free(blob);
		naui_directory_filter_free(entries);
		return false;
	}

	bool ok = true;
	size_t n = entries ? naui_list_len(entries) : 0;
	for (size_t i = 0; i < n && ok; ++i)
	{
		if (entries[i].is_directory)
			continue;

		size_t file_size;
		char* file_data = naui_file_read_all(entries[i].path, &file_size);
		if (!file_data)
		{
			ok = false;
			break;
		}

		while (blob_len + file_size > blob_cap)
		{
			blob_cap *= 2;
			uint8_t* tmp = (uint8_t*)realloc(blob, blob_cap);
			if (!tmp)
			{
				ok = false;
				free(file_data);
				break;
			}

			blob = tmp;
		}
		if (!ok)
		{
			free(file_data);
			break;
		}

		memcpy(blob + blob_len, file_data, file_size);
		free(file_data);

		const char* full = entries[i].path.data;
		const char* rel = full + strlen(folder.data);
		if (*rel == '/' || *rel == '\\')
			++rel;

		if (idx_len >= idx_cap)
		{
			idx_cap *= 2;
			IndexEntry* tmp = (IndexEntry*)realloc(index, idx_cap * sizeof(IndexEntry));
			if (!tmp)
			{
				ok = false;
				break;
			}

			index = tmp;
		}

		snprintf(index[idx_len].name, NAUI_PATH_MAX, "%s", rel);
		index[idx_len].offset = (uint64_t)blob_len;
		index[idx_len].size = (uint64_t)file_size;
		++idx_len;
		blob_len += file_size;
	}

	naui_directory_filter_free(entries);

	if (!ok)
	{
		free(blob);
		free(index);
		return false;
	}

	Naui_FileHandle fh = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&fh, archive_path, NAUI_FILE_WRITE))
	{
		free(blob);
		free(index);
		return false;
	}

	uint32_t version = NAUI_ARCHIVE_VERSION;
	uint64_t count = (uint64_t)idx_len;

	naui_file_write(&fh, NAUI_ARCHIVE_MAGIC, NAUI_ARCHIVE_MAGIC_SIZE);
	naui_file_write(&fh, &version, sizeof(version));
	naui_file_write(&fh, &count, sizeof(count));

	for (size_t i = 0; i < idx_len; ++i)
	{
		uint16_t len = (uint16_t)strlen(index[i].name);
		naui_file_write(&fh, &len, sizeof(len));
		naui_file_write(&fh, index[i].name, len);
		naui_file_write(&fh, &index[i].offset, sizeof(index[i].offset));
		naui_file_write(&fh, &index[i].size, sizeof(index[i].size));
	}

	naui_file_write(&fh, blob, blob_len);
	naui_file_close(&fh);
	free(blob);
	free(index);
	return true;
}

bool naui_archive_extract_custom(const Naui_Path archive_path, const Naui_Path output_folder)
{
	size_t total_size;
	char* blob = naui_file_read_all(archive_path, &total_size);
	if (!blob)
		return false;

	if (total_size < NAUI_ARCHIVE_MAGIC_SIZE || memcmp(blob, NAUI_ARCHIVE_MAGIC, NAUI_ARCHIVE_MAGIC_SIZE) != 0)
	{
		free(blob);
		return false;
	}

	const char* cursor = blob + NAUI_ARCHIVE_MAGIC_SIZE;

	uint32_t version;
	uint64_t count;
	memcpy(&version, cursor, sizeof(version));
	cursor += sizeof(version);

	memcpy(&count, cursor, sizeof(count));
	cursor += sizeof(count);

	typedef struct
	{
		char name[NAUI_PATH_MAX];
		uint64_t offset;
		uint64_t size;
	} IndexEntry;

	IndexEntry* index = (IndexEntry*)malloc((size_t)count * sizeof(IndexEntry));
	if (!index)
	{
		free(blob);
		return false;
	}

	for (uint64_t i = 0; i < count; ++i)
	{
		uint16_t len;
		memcpy(&len, cursor, sizeof(len));
		cursor += sizeof(len);

		memcpy(index[i].name, cursor, len);
		cursor += len;

		index[i].name[len] = '\0';

		memcpy(&index[i].offset, cursor, sizeof(index[i].offset));
		cursor += sizeof(index[i].offset);

		memcpy(&index[i].size, cursor, sizeof(index[i].size));
		cursor += sizeof(index[i].size);
	}

	const char* data_start = cursor;
	bool ok = true;

	for (uint64_t i = 0; i < count && ok; ++i)
	{
		Naui_Path out;
		snprintf(out.data, NAUI_PATH_MAX, "%s/%s", output_folder.data, index[i].name);

		Naui_Path parent = naui_path_parent(out);
		naui_directory_create(parent);
		ok = naui_file_write_all(out, data_start + index[i].offset, (size_t)index[i].size);
	}

	free(index);
	free(blob);
	return ok;
}