#define _CRT_SECURE_NO_WARNINGS
#if defined(_WIN32) || defined(_WIN64)

typedef struct
{
	HANDLE h;
} Naui_FileInternal;

#define NAUI_LOCK_MAX 32
typedef struct
{
	char path[NAUI_PATH_MAX];
	HANDLE handle;
} Naui_LockEntry;

static Naui_LockEntry s_locks[NAUI_LOCK_MAX];
static int s_lock_count = 0;

static Naui_Path path_empty(void)
{
	Naui_Path p;
	p.data[0] = '\0';
	return p;
}

static Naui_Path path_from(const char* s)
{
	Naui_Path p;
	if (s)
		snprintf(p.data, NAUI_PATH_MAX, "%s", s);
	else
		p.data[0] = '\0';

	return p;
}

static bool to_wide(const char* src, wchar_t* dst)
{
	return MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, NAUI_PATH_MAX) > 0;
}

static bool to_utf8(const wchar_t* src, char* dst)
{
	return WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, NAUI_PATH_MAX, NULL, NULL) > 0;
}

static bool is_separator(char c)
{
	return c == '/' || c == '\\';
}

static Naui_FileInternal* file_internal(const Naui_FileHandle* handle)
{
	return (Naui_FileInternal*)(void*)handle->_opaque;
}

static bool match_filter(const char* name, const char* filter)
{
	if (!filter || filter[0] == '\0')
		return true;

	const char* star = strchr(filter, '*');
	if (!star)
		return _stricmp(name, filter) == 0;

	size_t prefix_len = (size_t)(star - filter);
	if (_strnicmp(name, filter, prefix_len) != 0)
		return false;

	const char* suffix = star + 1;
	size_t suffix_len = strlen(suffix);
	size_t name_len = strlen(name);
	if (suffix_len > name_len - prefix_len)
		return false;

	return _stricmp(name + name_len - suffix_len, suffix) == 0;
}

static bool match_extensions(const char* name, const char** exts, int ext_count)
{
	if (ext_count <= 0 || !exts)
		return true;

	const char* dot = strrchr(name, '.');
	if (!dot)
		return false;

	for (int i = 0; i < ext_count; ++i)
	{
		if (exts[i] && _stricmp(dot, exts[i]) == 0)
			return true;
	}

	return false;
}

static void filter_recursive_impl_w(const char* path, const char* filter, const char** extensions, int ext_count, Naui_List(Naui_DirEntry)* list)
{
	wchar_t wsearch[NAUI_PATH_MAX];
	{
		wchar_t wpath[NAUI_PATH_MAX];
		if (!to_wide(path, wpath))
			return;

		_snwprintf(wsearch, NAUI_PATH_MAX, L"%s\\*", wpath);
	}

	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(wsearch, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		char name_u8[NAUI_PATH_MAX];
		if (!to_utf8(fd.cFileName, name_u8))
			continue;

		char child[NAUI_PATH_MAX];
		snprintf(child, sizeof(child), "%s\\%s", path, name_u8);
		bool is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		if (is_dir)
		{
			Naui_DirEntry de;
			de.path = path_from(child);
			de.is_directory = true;
			de.size = 0;
			naui_list_push(*list, de);
			filter_recursive_impl_w(child, filter, extensions, ext_count, list);
		}
		else
		{
			if (!match_filter(name_u8, filter))
				continue;

			if (!match_extensions(name_u8, extensions, ext_count))
				continue;

			ULARGE_INTEGER size;
			size.HighPart = fd.nFileSizeHigh;
			size.LowPart = fd.nFileSizeLow;

			Naui_DirEntry de;
			de.path = path_from(child);
			de.is_directory = false;
			de.size = (size_t)size.QuadPart;
			naui_list_push(*list, de);
		}

	} while (FindNextFileW(h, &fd));

	FindClose(h);
}

static void resolve_lock_target(const char* path, char* out)
{
	wchar_t wpath[NAUI_PATH_MAX];
	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, NAUI_PATH_MAX) > 0)
	{
		DWORD attrs = GetFileAttributesW(wpath);
		if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			snprintf(out, NAUI_PATH_MAX, "%s\\.lock", path);
			return;
		}
	}

	snprintf(out, NAUI_PATH_MAX, "%s", path);
}

bool naui_file_open(Naui_FileHandle* handle, const Naui_Path path, Naui_FileMode mode)
{
	if (!handle)
		return false;

	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return false;

	DWORD access, creation;
	switch (mode)
	{
		case NAUI_FILE_READ:
			access = GENERIC_READ;
			creation = OPEN_EXISTING;
			break;

		case NAUI_FILE_WRITE:
			access = GENERIC_WRITE;
			creation = CREATE_ALWAYS;
			break;

		case NAUI_FILE_APPEND:
			access = FILE_APPEND_DATA;
			creation = OPEN_ALWAYS;
			break;

		default:
			return false;
	}

	HANDLE h = CreateFileW(wpath, access, FILE_SHARE_READ, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	if (mode == NAUI_FILE_APPEND)
		SetFilePointer(h, 0, NULL, FILE_END);

	file_internal(handle)->h = h;
	return true;
}

size_t naui_file_read(const Naui_FileHandle* handle, void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	HANDLE h = file_internal(handle)->h;
	if (!h || h == INVALID_HANDLE_VALUE)
		return 0;

	DWORD got = 0;
	ReadFile(h, buffer, (DWORD)size, &got, NULL);
	return (size_t)got;
}

size_t naui_file_write(const Naui_FileHandle* handle, const void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	HANDLE h = file_internal(handle)->h;
	if (!h || h == INVALID_HANDLE_VALUE)
		return 0;

	DWORD written = 0;
	WriteFile(h, buffer, (DWORD)size, &written, NULL);
	return (size_t)written;
}

bool naui_file_seek(Naui_FileHandle* handle, long offset, int origin)
{
	if (!handle)
		return false;

	HANDLE h = file_internal(handle)->h;
	if (!h || h == INVALID_HANDLE_VALUE)
		return false;

	DWORD method;
	switch (origin)
	{
		case SEEK_SET:
			method = FILE_BEGIN;
			break;

		case SEEK_CUR:
			method = FILE_CURRENT;
			break;

		case SEEK_END:
			method = FILE_END;
			break;

		default:
			return false;
	}

	return SetFilePointer(h, offset, NULL, method) != INVALID_SET_FILE_POINTER;
}

bool naui_file_is_valid(const Naui_FileHandle* handle)
{
	if (!handle)
		return false;

	HANDLE h = file_internal(handle)->h;
	return h && h != INVALID_HANDLE_VALUE;
}

void naui_file_close(Naui_FileHandle* handle)
{
	if (!handle)
		return;

	Naui_FileInternal* fi = file_internal(handle);
	if (fi->h && fi->h != INVALID_HANDLE_VALUE)
	{
		CloseHandle(fi->h);
		fi->h = NULL;
	}
}

size_t naui_file_size(const Naui_Path path)
{
	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return 0;

	WIN32_FILE_ATTRIBUTE_DATA info;
	if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &info))
		return 0;

	ULARGE_INTEGER size;
	size.HighPart = info.nFileSizeHigh;
	size.LowPart = info.nFileSizeLow;
	return (size_t)size.QuadPart;
}

char* naui_file_read_all(const Naui_Path path, size_t* out_size)
{
	Naui_FileHandle handle = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&handle, path, NAUI_FILE_READ))
		return NULL;

	HANDLE h = file_internal(&handle)->h;
	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(h, &file_size))
	{
		naui_file_close(&handle);
		return NULL;
	}

	size_t size = (size_t)file_size.QuadPart;
	char* buf = (char*)malloc(size + 1);
	if (!buf)
	{
		naui_file_close(&handle);
		return NULL;
	}

	size_t total = 0;
	DWORD chunk;
	while (total < size)
	{
		DWORD to_read = (DWORD)((size - total) > 0xFFFFFFFFu ? 0xFFFFFFFFu : size - total);
		if (!ReadFile(h, buf + total, to_read, &chunk, NULL) || chunk == 0)
			break;

		total += chunk;
	}

	naui_file_close(&handle);
	buf[total] = '\0';
	if (out_size)
		*out_size = total;

	return buf;
}

bool naui_file_write_all(const Naui_Path path, const void* data, size_t size)
{
	if (!data)
		return false;

	Naui_FileHandle h = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&h, path, NAUI_FILE_WRITE))
		return false;

	size_t written = naui_file_write(&h, data, size);
	naui_file_close(&h);
	return written == size;
}

bool naui_file_delete(const Naui_Path path)
{
	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return false;

	return DeleteFileW(wpath) != 0;
}

bool naui_file_rename(const Naui_Path old_path, const Naui_Path new_path)
{
	wchar_t wold[NAUI_PATH_MAX];
	wchar_t wnew[NAUI_PATH_MAX];
	if (!to_wide(old_path.data, wold))
		return false;

	if (!to_wide(new_path.data, wnew))
		return false;

	return MoveFileExW(wold, wnew, MOVEFILE_REPLACE_EXISTING) != 0;
}

Naui_Path naui_file_hide(const Naui_Path path, bool hidden)
{
	Naui_Path result = path;
	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return result;

	DWORD attrs = GetFileAttributesW(wpath);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return result;

	attrs = hidden ? attrs | FILE_ATTRIBUTE_HIDDEN : attrs & ~FILE_ATTRIBUTE_HIDDEN;
	SetFileAttributesW(wpath, attrs);
	return result;
}

bool naui_file_is_hidden(const Naui_Path path)
{
	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return false;

	DWORD attrs = GetFileAttributesW(wpath);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
}

const char* naui_file_filename(const Naui_Path path)
{
	const char* data = path.data;
	const char* last = NULL;
	for (const char* p = data; *p; ++p)
	{
		if (is_separator(*p))
			last = p;
	}

	return last ? last + 1 : data;
}

Naui_Path naui_file_stem(const Naui_Path path)
{
	const char* filename = naui_file_filename(path);
	const char* dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return path_from(filename);

	Naui_Path result;
	size_t len = (size_t)(dot - filename);
	if (len >= NAUI_PATH_MAX)
		len = NAUI_PATH_MAX - 1;

	memcpy(result.data, filename, len);
	result.data[len] = '\0';
	return result;
}

Naui_Path naui_file_extension(const Naui_Path path)
{
	const char* filename = naui_file_filename(path);
	const char* dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return path_empty();

	return path_from(dot);
}

bool naui_directory_create(const Naui_Path path)
{
	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return false;

	return CreateDirectoryW(wpath, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool naui_directory_remove(const Naui_Path path)
{
	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return false;

	return RemoveDirectoryW(wpath) != 0;
}

static bool remove_all_recursive(wchar_t* wpath)
{
	wchar_t search[NAUI_PATH_MAX];
	_snwprintf(search, NAUI_PATH_MAX, L"%s\\*", wpath);
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(search, &fd);

	if (h == INVALID_HANDLE_VALUE)
		return RemoveDirectoryW(wpath) != 0;

	bool ok = true;
	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		wchar_t child[NAUI_PATH_MAX];
		_snwprintf(child, NAUI_PATH_MAX, L"%s\\%s", wpath, fd.cFileName);
		ok &= (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? remove_all_recursive(child) : (DeleteFileW(child) != 0);
	} while (FindNextFileW(h, &fd));

	FindClose(h);
	return ok & (RemoveDirectoryW(wpath) != 0);
}

bool naui_directory_remove_all(const Naui_Path path)
{
	if (path.data[0] == '\0')
		return false;

	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return false;

	return remove_all_recursive(wpath);
}

bool naui_directory_rename(const Naui_Path old_path, const Naui_Path new_path)
{
	return naui_file_rename(old_path, new_path);
}

static char s_working[NAUI_PATH_MAX] = {0}; // Used in `naui_directory_get` and `naui_path_set_current`
Naui_Path naui_directory_get(Naui_Dir directory)
{
	static char s_home[NAUI_PATH_MAX] = {0};
	static char s_bin[NAUI_PATH_MAX] = {0};
	static char s_appdata[NAUI_PATH_MAX] = {0};
	static char s_downloads[NAUI_PATH_MAX] = {0};
	static char s_temp[NAUI_PATH_MAX] = {0};

	switch (directory)
	{
		case NAUI_DIR_HOME:
		{
			if (s_home[0])
				return path_from(s_home);

			wchar_t w[NAUI_PATH_MAX];
			if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, w)))
				to_utf8(w, s_home);

			return path_from(s_home);
		}
		case NAUI_DIR_BIN:
		{
			if (s_bin[0])
				return path_from(s_bin);

			wchar_t w[NAUI_PATH_MAX];
			if (GetModuleFileNameW(NULL, w, NAUI_PATH_MAX))
			{
				wchar_t* last = wcsrchr(w, L'\\');
				if (last)
					*last = L'\0';

				to_utf8(w, s_bin);
			}

			return path_from(s_bin);
		}
		case NAUI_DIR_WORKING:
		{
			if (s_working[0])
				return path_from(s_working);

			wchar_t w[NAUI_PATH_MAX];
			if (GetCurrentDirectoryW(NAUI_PATH_MAX, w))
				to_utf8(w, s_working);

			return path_from(s_working);
		}
		case NAUI_DIR_APPDATA:
		{
			if (s_appdata[0])
				return path_from(s_appdata);

			wchar_t w[NAUI_PATH_MAX];
			if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, w)))
				to_utf8(w, s_appdata);

			return path_from(s_appdata);
		}
		case NAUI_DIR_DOWNLOADS:
		{
			if (s_downloads[0])
				return path_from(s_downloads);

			const char* home = naui_directory_get(NAUI_DIR_HOME).data;
			if (home[0])
				snprintf(s_downloads, NAUI_PATH_MAX, "%s\\Downloads", home);

			return path_from(s_downloads);
		}
		case NAUI_DIR_TEMP:
		{
			if(s_temp[0])
				return path_from(s_temp);

			wchar_t w[NAUI_PATH_MAX];
			DWORD len = GetTempPathW(NAUI_PATH_MAX, w);
			if(len > 0 && len < NAUI_PATH_MAX)
			{
				if (len > 1 && (w[len - 1] == L'\\' || w[len - 1] == L'/'))
					w[len - 1] = L'\0';

				to_utf8(w, s_temp);
			}

			return path_from(s_temp);
		}
	}

	return path_empty();
}

Naui_List(Naui_DirEntry) naui_directory_filter(const Naui_Path path, const char* filter, const char** extensions, int ext_count)
{
	Naui_List(Naui_DirEntry) list = NULL;

	if (path.data[0] == '\0')
		return list;

	wchar_t wsearch[NAUI_PATH_MAX];
	{
		wchar_t wpath[NAUI_PATH_MAX];
		if (!to_wide(path.data, wpath))
			return list;

		_snwprintf(wsearch, NAUI_PATH_MAX, L"%s\\*", wpath);
	}

	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(wsearch, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return list;

	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		char name_u8[NAUI_PATH_MAX];
		if (!to_utf8(fd.cFileName, name_u8))
			continue;

		if (!match_filter(name_u8, filter))
			continue;

		if (!match_extensions(name_u8, extensions, ext_count))
			continue;

		char full[NAUI_PATH_MAX];
		snprintf(full, sizeof(full), "%s\\%s", path.data, name_u8);

		ULARGE_INTEGER size;
		size.HighPart = fd.nFileSizeHigh;
		size.LowPart = fd.nFileSizeLow;

		Naui_DirEntry de;
		de.path = path_from(full);
		de.is_directory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		de.size = (size_t)size.QuadPart;
		naui_list_push(list, de);
	} while (FindNextFileW(h, &fd));

	FindClose(h);
	return list;
}

Naui_List(Naui_DirEntry) naui_directory_filter_recursive(const Naui_Path path, const char* filter, const char** extensions, int ext_count)
{
	Naui_List(Naui_DirEntry) list = NULL;
	if (path.data[0] == '\0')
		return list;

	filter_recursive_impl_w(path.data, filter, extensions, ext_count, &list);
	return list;
}

void naui_directory_filter_free(Naui_List(Naui_DirEntry) list)
{
	if (list)
		naui_list_free(list);
}

bool naui_path_set_current(const Naui_Path path)
{
	if (path.data[0] == '\0')
		return false;

	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return false;

	if (!SetCurrentDirectoryW(wpath))
		return false;

	memcpy(s_working, path.data, NAUI_PATH_MAX);
	return true;
}

bool naui_path_exists(const Naui_Path path)
{
	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return false;

	return GetFileAttributesW(wpath) != INVALID_FILE_ATTRIBUTES;
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

Naui_Path naui_path_parent(const Naui_Path path)
{
	size_t len = strlen(path.data);
	while (len > 1 && is_separator(path.data[len - 1]))
	{
		--len;
	}

	const char* last = NULL;
	for (size_t i = len; i > 0; --i)
	{
		if (!is_separator(path.data[i - 1]))
			continue;

		last = path.data + i - 1;
		break;
	}

	if (!last)
		return path_from(".");

	if (last == path.data)
		return path_from("/");

	size_t parent_len = (size_t)(last - path.data);
	if (parent_len == 2 && path.data[1] == ':')
		parent_len = 3;

	Naui_Path result;
	memcpy(result.data, path.data, parent_len);
	result.data[parent_len] = '\0';
	return result;
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
 
			result.data[out_len++] = '\\';
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

Naui_Path naui_path_normalize(const Naui_Path path)
{
	char buf[NAUI_PATH_MAX];
	snprintf(buf, sizeof(buf), "%s", path.data);
	for (char* p = buf; *p; ++p)
	{
		if (*p == '/')
			*p = '\\';
	}

	bool is_unix_abs = (buf[0] == '\\');
	bool is_drive_abs = (buf[1] == ':' && buf[2] == '\\');
	const char* parts[NAUI_PATH_MAX / 2];
	int n = 0;
	char* tok = strtok(buf, "\\");

	if (is_drive_abs && tok)
	{
		parts[n++] = tok;
		tok = strtok(NULL, "\\");
	}

	while (tok)
	{
		if (strcmp(tok, ".") == 0)
			;
		else if (strcmp(tok, "..") == 0)
		{
			int root_depth = is_drive_abs ? 1 : 0;
			if (n > root_depth)
				--n;
		}
		else
			parts[n++] = tok;

		tok = strtok(NULL, "\\");
	}

	if (n == 0)
		return path_from(is_unix_abs ? "\\" : ".");

	Naui_Path result;
	result.data[0] = '\0';
	const char* expr = is_unix_abs ? "\\%s" : "%s";
	snprintf(result.data, NAUI_PATH_MAX, expr, parts[0]);

	for (int i = 1; i < n; ++i)
	{
		strncat(result.data, "\\", NAUI_PATH_MAX - strlen(result.data) - 1), strncat(result.data, parts[i], NAUI_PATH_MAX - strlen(result.data) - 1);
	}

	size_t len = strlen(result.data);
	if (len == 2 && result.data[1] == ':')
	{
		result.data[2] = '\\';
		result.data[3] = '\0';
	}

	return result;
}

Naui_Path naui_path_absolute(const Naui_Path path)
{
	if ((path.data[0] && path.data[1] == ':') || path.data[0] == '/' || path.data[0] == '\\')
		return path;

	Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
	return naui_path_join(cwd, path);
}

Naui_Path naui_path_canonical(const Naui_Path path)
{
	wchar_t wpath[NAUI_PATH_MAX];
	if (!to_wide(path.data, wpath))
		return path_empty();

	wchar_t wresolved[NAUI_PATH_MAX];
	if (!GetFullPathNameW(wpath, NAUI_PATH_MAX, wresolved, NULL))
		return path_empty();

	if (GetFileAttributesW(wresolved) == INVALID_FILE_ATTRIBUTES)
		return path_empty();

	Naui_Path result;
	to_utf8(wresolved, result.data);
	return result;
}

Naui_Path naui_path_weakly_canonical(const Naui_Path path)
{
	Naui_Path existing = path;
	Naui_Path tail = path_empty();
	while (existing.data[0] != '\0' && !naui_path_exists(existing))
	{
		Naui_Path parent = naui_path_parent(existing);
		Naui_Path segment = path_from(naui_file_filename(existing));

		if (tail.data[0] != '\0')
		{
			Naui_Path new_tail;
			snprintf(new_tail.data, NAUI_PATH_MAX, "%s\\%s", segment.data, tail.data);
			tail = new_tail;
		}
		else
			tail = segment;

		existing = parent;
	}

	Naui_Path base = (existing.data[0] != '\0') ? naui_path_canonical(existing) : path_from(".");
	if (tail.data[0] == '\0')
		return base;

	Naui_Path joined = naui_path_join(base, tail);
	return naui_path_normalize(joined);
}

bool naui_path_lock(const Naui_Path path)
{
	if (path.data[0] == '\0' || s_lock_count >= NAUI_LOCK_MAX)
		return false;

	char target[NAUI_PATH_MAX];
	resolve_lock_target(path.data, target);
	for (int i = 0; i < s_lock_count; ++i)
	{
		if (_stricmp(s_locks[i].path, target) == 0)
			return false;
	}

	const char* dot_lock = strstr(target, "\\.lock");
	if (dot_lock && *(dot_lock + 6) == '\0')
	{
		char parent[NAUI_PATH_MAX];
		size_t parent_len = (size_t)(dot_lock - target);
		memcpy(parent, target, parent_len);
		parent[parent_len] = '\0';
		wchar_t wparent[NAUI_PATH_MAX];
		if (to_wide(parent, wparent))
			CreateDirectoryW(wparent, NULL);
	}

	wchar_t wtarget[NAUI_PATH_MAX];
	if (!to_wide(target, wtarget))
		return false;

	HANDLE h = CreateFileW(
		wtarget,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_HIDDEN,
		NULL
	);

	if (h == INVALID_HANDLE_VALUE)
		return false;

	OVERLAPPED overlap = {0};
	if (!LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXWORD, MAXWORD, &overlap))
	{
		CloseHandle(h);
		return false;
	}

	snprintf(s_locks[s_lock_count].path, NAUI_PATH_MAX, "%s", target);
	s_locks[s_lock_count].handle = h;
	++s_lock_count;
	return true;
}

bool naui_path_is_locked(const Naui_Path path)
{
	if (path.data[0] == '\0')
		return false;

	char target[NAUI_PATH_MAX];
	resolve_lock_target(path.data, target);
	for (int i = 0; i < s_lock_count; ++i)
	{
		if (_stricmp(s_locks[i].path, target) == 0)
			return true;
	}

	wchar_t wtarget[NAUI_PATH_MAX];
	if (!to_wide(target, wtarget))
		return false;

	if (GetFileAttributesW(wtarget) == INVALID_FILE_ATTRIBUTES)
		return false;

	HANDLE h = CreateFileW(
		wtarget,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_HIDDEN,
		NULL
	);

	if (h == INVALID_HANDLE_VALUE)
		return true;

	CloseHandle(h);
	return false;
}

void naui_path_unlock(const Naui_Path path)
{
	if (path.data[0] == '\0')
		return;

	char target[NAUI_PATH_MAX];
	resolve_lock_target(path.data, target);

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (_stricmp(s_locks[i].path, target) != 0)
			continue;

		UnlockFile(s_locks[i].handle, 0, 0, MAXWORD, MAXWORD);
		CloseHandle(s_locks[i].handle);
		s_locks[i] = s_locks[--s_lock_count];
		return;
	}
}

#endif