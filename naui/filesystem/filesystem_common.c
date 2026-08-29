Naui_StringView naui_file_filename(const Naui_Path* path)
{
	const char* start = path->data;
	const char* last_sep = NULL;
	const char* p = start;
	for (; *p; ++p)
	{
		if (is_separator(*p))
			last_sep = p;
	}

	const char* filename_start = last_sep ? last_sep + 1 : start; 
	return (Naui_StringView){
		.data = (char*)filename_start,
		.length = (size_t)(p - filename_start)
	};
}

Naui_StringView naui_file_stem(const Naui_Path* path)
{
	Naui_StringView filename = naui_file_filename(path);
	const char* dot = find_last_char(filename, '.');

	if (!dot || dot == filename.data)
		return filename;

	return naui_sub_string_view(filename, 0, (size_t)(dot - filename.data));
}

Naui_StringView naui_file_extension(const Naui_Path* path)
{
	Naui_StringView filename = naui_file_filename(path);
	const char* dot = find_last_char(filename, '.');

	if (!dot || dot == filename.data)
		return (Naui_StringView){ 0 };

	size_t offset = (size_t)(dot - filename.data);
	return naui_sub_string_view(filename, offset, filename.length - offset);
}

bool naui_file_create(const Naui_Path path)
{
	Naui_FileHandle handle = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&handle, path, NAUI_FILE_WRITE))
		return false;

	naui_file_write(&handle, "\0", 0);
	naui_file_close(&handle);
	return true;
}

bool naui_file_copy(const Naui_Path src_path, const Naui_Path dest_path)
{
	Naui_FileHandle src_handle = NAUI_FILE_HANDLE_INIT;
	Naui_FileHandle dest_handle = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&src_handle, src_path, NAUI_FILE_READ))
		return false;

	if (!naui_file_open(&dest_handle, dest_path, NAUI_FILE_WRITE))
	{
		naui_file_close(&src_handle);
		return false;
	}

	bool success = true;
	char buffer[4096];
	size_t bytes;
	while((bytes = naui_file_read(&src_handle, buffer, sizeof(buffer))) > 0)
	{
		if (naui_file_write(&dest_handle, buffer, bytes) != bytes)
		{
			success = false;
			break;
		}
	}

	naui_file_close(&src_handle);
	naui_file_close(&dest_handle);
	return success;
}

void naui_directory_filter_free(Naui_List(Naui_DirEntry) list)
{
	if (list)
		naui_list_free(list);
}

bool naui_directories_create(const Naui_Path path)
{
	if (naui_path_is_empty(path))
		return false;

	if (naui_path_exists(path))
		return naui_path_is_directory(path);

	Naui_Path parent = naui_path_parent(path);
	if (!naui_path_is_empty(parent) && strcmp(parent.data, path.data) != 0)
	{
		if(!naui_directories_create(parent))
			return false;
	}

	return naui_directory_create(path);
}

bool naui_path_is_empty(const Naui_Path path)
{
	return path.data[0] == '\0';
}

Naui_Path naui_path_from_cstr(const char* str)
{
	Naui_Path p;
	if (!str)
	{
		p.data[0] = '\0';
		return p;
	}

	snprintf(p.data, NAUI_PATH_MAX, "%s", str);
	return p;
}

Naui_Path naui_path_join(const Naui_Path a, const Naui_Path b)
{
	const char* parts[] = { a.data, b.data, NULL };
	return naui_path_join_parts(parts);
}
 
Naui_Path naui_path_join_parts(const char** parts)
{
	Naui_Path result;
	result.data[0] = '\0';
	if (!parts)
		return result;
 
	size_t out_len = 0;
	for (int i = 0; parts[i] != NULL; ++i)
	{
		const char* part = parts[i];
		if (!part)
			continue;
 
		size_t part_len = strlen(part);
		if (part_len == 0)
			continue;
 
		size_t start = 0;
		if (out_len > 0)
		{
			while (start < part_len && is_separator(part[start]))
			{
				++start;
			}
		}
 
		size_t end = part_len;
		while (end > start && is_separator(part[end - 1]))
		{
			--end;
		}
 
		if (start >= end)
			continue;
 
		if (out_len > 0 && !is_separator(result.data[out_len - 1]))
		{
			if (out_len + 1 >= NAUI_PATH_MAX)
				break;
 
			result.data[out_len++] = NAUI_PATH_SEP;
		}
 
		size_t copy_len = end - start;
		if (out_len + copy_len >= NAUI_PATH_MAX)
			copy_len = NAUI_PATH_MAX - 1 - out_len;
 
		memcpy(result.data + out_len, part + start, copy_len);
		out_len += copy_len;
 
		if (out_len >= NAUI_PATH_MAX - 1)
			break;
	}
 
	result.data[out_len] = '\0';
	return result;
}