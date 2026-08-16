#if !defined(_WIN32) && !defined(_WIN64)

typedef struct
{
	DIR* dir;
	char root[NAUI_PATH_MAX];
} Naui_DirIterInternal;

static Naui_DirIterInternal* iterator_internal(Naui_DirIterator* it)
{
	return (Naui_DirIterInternal*)(void*)it->_handle;
}

// functions needed by iterator_win32 and iterator_unix
int naui_cstr_strcmp(const char *str1, const char *str2, bool case_sensitive) {
    if (case_sensitive)
        return strcmp(str1, str2);

    return strcasecmp(str1, str2);
}

int naui_cstr_strncmp(const char *str1, const char *str2, size_t len, bool case_sensitive) {
    if (case_sensitive)
        return strncmp(str1, str2, len);

    return strncasecmp(str1, str2, len);
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

static void iterator_advance(Naui_DirIterator* it)
{
	Naui_DirIterInternal* internal = iterator_internal(it);
	it->is_valid = false;

	struct dirent* entry;
	while ((entry = readdir(internal->dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		const char* filter = it->_filter[0] ? it->_filter : NULL;
		if (!iterator_match_filter(entry->d_name, filter, it->case_sensitive))
			continue;

		if (!iterator_match_extensions(entry->d_name, it->_extensions, it->case_sensitive))
			continue;

		char child[NAUI_PATH_MAX];
		snprintf(child, sizeof(child), "%s/%s", internal->root, entry->d_name);

		struct stat st;
		if (stat(child, &st) != 0)
			continue;

		snprintf(it->entry.path.data, NAUI_PATH_MAX, "%s", child);
		it->entry.is_directory = S_ISDIR(st.st_mode);
		it->entry.size = (size_t)st.st_size;
		it->is_valid = true;
		return;
	}
}

Naui_DirIterator naui_dir_iterator_open(const Naui_Path path, const char* filter, const char** extensions, bool case_sensitive)
{
	Naui_DirIterator it;
	memset(&it, 0, sizeof(it));

	Naui_DirIterInternal* internal = iterator_internal(&it);
	internal->dir = opendir(path.data);
	if (!internal->dir)
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
	Naui_DirIterInternal* internal = iterator_internal(it);
	if (internal->dir)
	{
		closedir(internal->dir);
		internal->dir = NULL;
	}

	it->is_valid = false;
}

#endif