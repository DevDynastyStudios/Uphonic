#if defined(_WIN32) || defined(_WIN64)

typedef struct
{
	HANDLE find_handle;
	WIN32_FIND_DATAW find_data;
	char root[NAUI_PATH_MAX];
	bool first;
} Naui_DirIteratorInternal;

static Naui_DirIteratorInternal* iterator_internal(Naui_DirIterator* it)
{
	return (Naui_DirIteratorInternal*)(void*)it->_handle;
}

static bool iterator_match_filter(const char* name, const char* filter, bool case_sensitive)
{
	if (!filter || filter[0] == '\0')
		return true;

	size_t filter_len = strlen(filter);
	return naui_cstr_strncmp(name, filter, filter_len, case_sensitive) == 0;
}

static bool iterator_match_extensions(const char* name, const char** exts, bool case_sensitive)
{
	if (!exts)
		return true;

	const char* dot = strrchr(name, '.');
	if (!dot)
		return false;

	size_t index = 0;
	while (exts[index] != NULL)
	{
		if (exts[index] && naui_cstr_strcmp(dot, exts[index], case_sensitive) == 0)
			return true;

		index++;
	}

	return false;	
}

static bool iterator_fill_entry(Naui_DirIterator* it)
{
	Naui_DirIteratorInternal* internal = iterator_internal(it);
	char name_u8[NAUI_PATH_MAX];
	if (!to_utf8(internal->find_data.cFileName, name_u8))
		return false;

	if (strcmp(name_u8, ".") == 0 || strcmp(name_u8, "..") == 0)
		return false;

	const char* filter = it->_filter[0] ? it->_filter : NULL;
	if (!iterator_match_filter(name_u8, filter, it->case_sensitive))
		return false;

	if (!iterator_match_extensions(name_u8, it->_extensions, it->case_sensitive))
		return false;

	snprintf(it->entry.path.data, NAUI_PATH_MAX, "%s\\%s", internal->root, name_u8);
	it->entry.is_directory = (internal->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

	ULARGE_INTEGER sz;
	sz.HighPart = internal->find_data.nFileSizeHigh;
	sz.LowPart = internal->find_data.nFileSizeLow;
	it->entry.size = (size_t)sz.QuadPart;
	return true;
}

static void iterator_advance(Naui_DirIterator* it)
{
	Naui_DirIteratorInternal* internal = iterator_internal(it);
	it->is_valid = false;

	if (internal->find_handle == INVALID_HANDLE_VALUE)
		return;

	if (internal->first)
	{
		internal->first = false;
		if (iterator_fill_entry(it))
		{
			it->is_valid = true;
			return;
		}
	}

	while (FindNextFileW(internal->find_handle, &internal->find_data))
	{
		if (iterator_fill_entry(it))
		{
			it->is_valid = true;
			return;
		}
	}
}

Naui_DirIterator naui_dir_iterator_open(const Naui_Path path, const char* filter, const char** extensions, bool case_sensitive)
{
	Naui_DirIterator it;
	memset(&it, 0, sizeof(it));

	Naui_DirIteratorInternal* internal = iterator_internal(&it);
	internal->find_handle = INVALID_HANDLE_VALUE;
	internal->first = true;
	wchar_t wsearch[NAUI_PATH_MAX];
	{
		wchar_t wpath[NAUI_PATH_MAX];
		if (!to_wide(path.data, wpath))
			return it;

		_snwprintf(wsearch, NAUI_PATH_MAX, L"%s\\*", wpath);
	}

	internal->find_handle = FindFirstFileW(wsearch, &internal->find_data);
	if (internal->find_handle == INVALID_HANDLE_VALUE)
		return it;

	snprintf(internal->root, NAUI_PATH_MAX, "%s", path.data);
	if (filter)
		snprintf(it._filter, sizeof(it._filter), "%s", filter);

	it._extensions = extensions;
	it.case_sensitive = case_sensitive;
	iterator_advance(&it);
	return it;
}

void naui_dir_iterator_next(Naui_DirIterator* it)
{
	iterator_advance(it);
}

bool naui_dir_iterator_valid(const Naui_DirIterator* it)
{
	return it->is_valid;
}

void naui_dir_iterator_close(Naui_DirIterator* it)
{
	Naui_DirIteratorInternal* internal = iterator_internal(it);
	if (internal->find_handle != INVALID_HANDLE_VALUE)
	{
		FindClose(internal->find_handle);
		internal->find_handle = INVALID_HANDLE_VALUE;
	}

	it->is_valid = false;
}

#endif