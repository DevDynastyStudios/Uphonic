#if !defined(_WIN32) && !defined(_WIN64)

#define NAUI_LOCK_MAX 32
typedef struct
{
	char path[NAUI_PATH_MAX];
	int fd;
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

static bool is_separator(char c)
{
	return c == '/';
}

typedef struct { FILE* fp; } Naui_FileInternal;

static Naui_FileInternal* _naui_file_internal(const Naui_FileHandle* handle)
{
	return (Naui_FileInternal*)(void*)handle->_opaque;
}

static bool match_filter(const char* name, const char* filter)
{
	if (!filter || filter[0] == '\0')
		return true;

	const char* star = strchr(filter, '*');
	if (!star)
		return strcmp(name, filter) == 0;

	size_t prefix_len = (size_t)(star - filter);
	if (strncmp(name, filter, prefix_len) != 0)
		return false;

	const char* suffix = star + 1;
	size_t suffix_len = strlen(suffix);
	size_t name_len = strlen(name);
	if (suffix_len > name_len - prefix_len)
		return false;

	return strcmp(name + name_len - suffix_len, suffix) == 0;
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
		if (exts[i] && strcmp(dot, exts[i]) == 0)
			return true;
	}

	return false;
}

static void _naui_filter_recursive_impl(const char* path, const char* filter, const char** extensions, int ext_count, Naui_List(Naui_DirEntry)* list)
{
	DIR* dir = opendir(path);
	if (!dir)
		return;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		char child[NAUI_PATH_MAX];
		snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

		struct stat st;
		if (stat(child, &st) != 0)
			continue;

		if (S_ISDIR(st.st_mode))
		{
			Naui_DirEntry de;
			de.path = path_from(child);
			de.is_directory = true;
			de.size = 0;
			naui_list_push(*list, de);
			_naui_filter_recursive_impl(child, filter, extensions, ext_count, list);
		}
		else
		{
			if (!match_filter(entry->d_name, filter))
				continue;

			if (!match_extensions(entry->d_name, extensions, ext_count))
				continue;

			Naui_DirEntry de;
			de.path = path_from(child);
			de.is_directory = false;
			de.size = (size_t)st.st_size;
			naui_list_push(*list, de);
		}
	}

	closedir(dir);
}

static void _naui_resolve_lock_target(const char* path, char* out)
{
	struct stat st;
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
		snprintf(out, NAUI_PATH_MAX, "%s/.lock", path);
	else
		snprintf(out, NAUI_PATH_MAX, "%s", path);
}

bool naui_file_open(Naui_FileHandle* handle, const Naui_Path path, Naui_FileMode mode)
{
	if (!handle)
		return false;

	const char* m;
	switch (mode)
	{
		case NAUI_FILE_READ:
			m = "rb";
			break;

		case NAUI_FILE_WRITE:
			m = "wb";
			break;

		case NAUI_FILE_APPEND:
			m = "ab";
			break;

		default:
			return false;
	}

	Naui_FileInternal* fi = _naui_file_internal(handle);
	fi->fp = fopen(path.data, m);
	return fi->fp != NULL;
}

size_t naui_file_read(const Naui_FileHandle* handle, void* restrict buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	Naui_FileInternal* fi = _naui_file_internal(handle);
	if (!fi->fp)
		return 0;

	return fread(buffer, 1, size, fi->fp);
}

size_t naui_file_write(const Naui_FileHandle* handle, const void* restrict buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	Naui_FileInternal* fi = _naui_file_internal(handle);
	if (!fi->fp)
		return 0;

	return fwrite(buffer, 1, size, fi->fp);
}

bool naui_file_seek(Naui_FileHandle* handle, long offset, int origin)
{
	if (!handle)
		return false;

	Naui_FileInternal* fi = _naui_file_internal(handle);
	if (!fi->fp)
		return false;

	return fseek(fi->fp, offset, origin) == 0;
}

bool naui_file_is_valid(const Naui_FileHandle* handle)
{
	if (!handle)
		return false;

	return _naui_file_internal(handle)->fp != NULL;
}

void naui_file_close(Naui_FileHandle* handle)
{
	if (!handle)
		return;

	Naui_FileInternal* fi = _naui_file_internal(handle);
	if (fi->fp)
	{
		fclose(fi->fp);
		fi->fp = NULL;
	}
}

size_t naui_file_size(const Naui_Path path)
{
	struct stat st;
	if (stat(path.data, &st) != 0)
		return 0;

	return (size_t)st.st_size;
}

char* naui_file_read_all(const Naui_Path path, size_t* out_size)
{
	FILE* fp = fopen(path.data, "rb");
	if (!fp)
		return NULL;

	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	rewind(fp);

	if (len < 0)
	{
		fclose(fp);
		return NULL;
	}

	char* buf = (char*)malloc((size_t)len + 1);
	if (!buf)
	{
		fclose(fp);
		return NULL;
	}

	size_t got = fread(buf, 1, (size_t)len, fp);
	fclose(fp);
	buf[got] = '\0';

	if (out_size)
		*out_size = got;

	return buf;
}

bool naui_file_write_all(const Naui_Path path, const void* data, size_t size)
{
	if (!data)
		return false;

	FILE* fp = fopen(path.data, "wb");
	if (!fp)
		return false;

	size_t written = fwrite(data, 1, size, fp);
	fclose(fp);
	return written == size;
}

bool naui_file_delete(const Naui_Path path)
{
	return remove(path.data) == 0;
}

bool naui_file_rename(const Naui_Path old_path, const Naui_Path new_path)
{
	return rename(old_path.data, new_path.data) == 0;
}

Naui_Path naui_file_hide(const Naui_Path* path, bool hidden)
{
	Naui_Path result = *path;
	Naui_StringView filename = naui_file_filename(path);
	bool currently_hidden = (filename.data[0] == '.');
	if (currently_hidden == hidden)
		return result;

	Naui_Path parent = naui_path_parent(result);
	Naui_Path renamed;
	if (hidden)
		snprintf(renamed.data, NAUI_PATH_MAX, "%s/.%s", parent.data, filename.data);
	else
		snprintf(renamed.data, NAUI_PATH_MAX, "%s/%s", parent.data, filename.data + 1);

	if (rename(result.data, renamed.data) != 0)
		return result;

	return renamed;
}

bool naui_file_is_hidden(const Naui_Path* path)
{
	Naui_StringView filename = naui_file_filename(path);
	return filename.data[0] == '.';
}

bool naui_directory_create(const Naui_Path path)
{
	return mkdir(path.data, 0755) == 0 || errno == EEXIST;
}

bool naui_directory_remove(const Naui_Path path)
{
	return rmdir(path.data) == 0;
}

static bool _naui_remove_all_recursive(const char* path)
{
	DIR* dir = opendir(path);
	if (!dir)
		return remove(path) == 0;

	struct dirent* entry;
	bool ok = true;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		char child[NAUI_PATH_MAX];
		snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
		struct stat st;
		ok &= (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) ? _naui_remove_all_recursive(child) : (remove(child) == 0);
	}

	closedir(dir);
	ok &= (rmdir(path) == 0);
	return ok;
}

bool naui_directory_remove_all(const Naui_Path path)
{
	if (path.data[0] == '\0')
		return false;

	return _naui_remove_all_recursive(path.data);
}

Naui_List(Naui_DirEntry) naui_directory_filter_recursive(const Naui_Path path, const char* filter, const char** extensions)
{
	Naui_List(Naui_DirEntry) list = NULL;

	if (path.data[0] == '\0')
		return list;

	uint32_t ext_count = 0;
	while(extensions[ext_count] != NULL)
	{
		ext_count++;
	}

	_naui_filter_recursive_impl(path.data, filter, extensions, ext_count, &list);
	return list;
}

bool naui_directory_rename(const Naui_Path old_path, const Naui_Path new_path)
{
	return naui_file_rename(old_path, new_path);
}

static char* s_dir_cache[NAUI_DIR_COUNT] = {0};

static char* heap_dup(const char* s)
{
	if (!s)
		return NULL;

	size_t len = strlen(s) + 1;
	char* copy = (char*)malloc(len);
	if (copy)
		memcpy(copy, s, len);

	return copy;
}

static void _naui_dir_cache_set(Naui_Dir directory, const char* value)
{
	if (directory >= NAUI_DIR_COUNT || !value || !value[0])
		return;

	free(s_dir_cache[directory]);
	s_dir_cache[directory] = heap_dup(value);
}

Naui_Path naui_directory_get(const Naui_Dir directory)
{
	if (directory < NAUI_DIR_COUNT && s_dir_cache[directory])
		return path_from(s_dir_cache[directory]);

	char resolved[NAUI_PATH_MAX] = {0};

	switch (directory)
	{
		case NAUI_DIR_HOME:
		{
			const char* h = getenv("HOME");
			if (h)
				snprintf(resolved, sizeof(resolved), "%s", h);

			break;
		}
		case NAUI_DIR_BIN:
		{
#ifdef __APPLE__
			uint32_t size = sizeof(resolved);
			if (_NSGetExecutablePath(resolved, &size) == 0)
			{
				char* last = strrchr(resolved, '/');
				if (last)
					*last = '\0';
			}
#else
			ssize_t n = readlink("/proc/self/exe", resolved, sizeof(resolved) - 1);
			if (n > 0)
			{
				resolved[n] = '\0';
				char* last = strrchr(resolved, '/');
				if (last)
					*last = '\0';
			}
#endif
			break;
		}
		case NAUI_DIR_WORKING:
		{
			getcwd(resolved, sizeof(resolved));
			break;
		}
		case NAUI_DIR_APPDATA:
		{
		#ifdef __APPLE__
			const char* h = getenv("HOME");
			if (h)
				snprintf(resolved, sizeof(resolved), "%s/Library/Caches", h);
		#else
			const char* xdg = getenv("XDG_CACHE_HOME");
			if (xdg)
				snprintf(resolved, sizeof(resolved), "%s", xdg);
			else
			{
				const char* h = getenv("HOME");
				if (h)
					snprintf(resolved, sizeof(resolved), "%s/.cache", h);
			}
		#endif
			break;
		}
		case NAUI_DIR_ROAMING:
		{
#ifdef __APPLE__
			const char* h = getenv("HOME");
			if (h)
				snprintf(resolved, sizeof(resolved), "%s/Library/Application Support", h);
#else
			const char* xdg = getenv("XDG_DATA_HOME");
			if (xdg)
				snprintf(resolved, sizeof(resolved), "%s", xdg);
			else
			{
				const char* h = getenv("HOME");
				if (h)
					snprintf(resolved, sizeof(resolved), "%s/.local/share", h);
			}
#endif
			break;
		}
		case NAUI_DIR_DOWNLOADS:
		{
			const char* h = getenv("HOME");
			if (h)
				snprintf(resolved, sizeof(resolved), "%s/Downloads", h);

			break;
		}
		case NAUI_DIR_TEMP:
		{
			const char* tmp = getenv("TMPDIR");
			if(!tmp || !tmp[0])
				tmp = "/tmp";

			snprintf(resolved, sizeof(resolved), "%s", tmp);
			break;
		}
		case NAUI_DIR_ASSETS:
		{
			Naui_Path bin_path = naui_directory_get(NAUI_DIR_WORKING);
			if (bin_path.data[0])
			{
				Naui_Path joined = naui_path_join(bin_path, naui_path_from_cstr("Assets"));
				snprintf(resolved, sizeof(resolved), "%s", joined.data);
			}

			break;
		}
		default:
			return path_empty();
	}

	_naui_dir_cache_set(directory, resolved);
	return path_from(resolved);
}

Naui_List(Naui_DirEntry) naui_directory_filter(const Naui_Path path, const char* filter, const char** extensions)
{
	Naui_List(Naui_DirEntry) list = NULL;

	if (path.data[0] == '\0')
		return list;

	DIR* dir = opendir(path.data);
	if (!dir)
		return list;

	uint32_t ext_count = 0;
	while (extensions[ext_count] != NULL)
	{
		ext_count++;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (!match_filter(entry->d_name, filter))
			continue;

		if (!match_extensions(entry->d_name, extensions, ext_count))
			continue;

		char child[NAUI_PATH_MAX];
		snprintf(child, sizeof(child), "%s/%s", path.data, entry->d_name);
		struct stat st;
		if (stat(child, &st) != 0)
			continue;

		Naui_DirEntry de;
		de.path = path_from(child);
		de.is_directory = S_ISDIR(st.st_mode);
		de.size = (size_t)st.st_size;
		naui_list_push(list, de);
	}

	closedir(dir);
	return list;
}

bool naui_path_set_current(const Naui_Path path)
{
	if (path.data[0] == '\0')
		return false;

	if (chdir(path.data) != 0)
		return false;

	_naui_dir_cache_set(NAUI_DIR_WORKING, path.data);
	return true;
}

bool naui_path_exists(const Naui_Path path)
{
	struct stat st;
	return stat(path.data, &st) == 0;
}

bool naui_path_is_directory(const Naui_Path path)
{
	struct stat st;
	if (stat(path.data, &st) != 0)
		return false;

	return S_ISDIR(st.st_mode);
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
	Naui_Path result;
	memcpy(result.data, path.data, parent_len);
	result.data[parent_len] = '\0';
	return result;
}

Naui_Path naui_path_normalize(const Naui_Path path)
{
	char buf[NAUI_PATH_MAX];
	snprintf(buf, sizeof(buf), "%s", path.data);
	bool is_absolute = (buf[0] == '/');
	const char* parts[NAUI_PATH_MAX >> 1];
	int n = 0;
	char* tok = strtok(buf, "/");
	
	while (tok)
	{
		if (strcmp(tok, ".") == 0)
			;
		else if (strcmp(tok, "..") == 0)
		{
			if (n > 0)
				--n;
		}
		else
			parts[n++] = tok;

		tok = strtok(NULL, "/");
	}

	if (n == 0)
		return path_from(is_absolute ? "/" : ".");

	Naui_Path result;
	result.data[0] = '\0';
	const char* expr = is_absolute ? "/%s" : "%s";
	snprintf(result.data, NAUI_PATH_MAX, expr, parts[0]);

	for (int i = 1; i < n; ++i)
	{
		strncat(result.data, "/", NAUI_PATH_MAX - strlen(result.data) - 1), strncat(result.data, parts[i], NAUI_PATH_MAX - strlen(result.data) - 1);
	}

	return result;
}

Naui_Path naui_path_absolute(const Naui_Path path)
{
	if (path.data[0] == '/')
		return path;

	Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
	return naui_path_join(cwd, path);
}

Naui_Path naui_path_canonical(const Naui_Path path)
{
	char resolved[NAUI_PATH_MAX];
	if (!realpath(path.data, resolved))
		return path_empty();

	return path_from(resolved);
}

Naui_Path naui_path_weakly_canonical(const Naui_Path path)
{
	Naui_Path existing = path;
	Naui_Path tail = path_empty();
	while (existing.data[0] != '\0' && !naui_path_exists(existing))
	{
		Naui_Path parent = naui_path_parent(existing);
		Naui_Path segment = path_from(naui_file_filename(&existing).data);

		if (tail.data[0] != '\0')
		{
			Naui_Path new_tail;
			snprintf(new_tail.data, NAUI_PATH_MAX, "%s/%s", segment.data, tail.data);
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
	_naui_resolve_lock_target(path.data, target);

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (strcmp(s_locks[i].path, target) == 0)
			return false;
	}

	const char* filename = target + strlen(target);
	while (filename > target && *filename != '/')
	{
		--filename;
	}

	if (strcmp(filename, "/.lock") == 0)
	{
		char parent[NAUI_PATH_MAX];
		size_t parent_len = (size_t)(filename - target);
		memcpy(parent, target, parent_len);
		parent[parent_len] = '\0';
		mkdir(parent, 0755);
	}

	int fd = open(target, O_RDWR | O_CREAT | O_CLOEXEC, 0666);
	if (fd == -1)
		return false;

	if (flock(fd, LOCK_EX | LOCK_NB) != 0)
	{
		close(fd);
		return false;
	}

	snprintf(s_locks[s_lock_count].path, NAUI_PATH_MAX, "%s", target);
	s_locks[s_lock_count].fd = fd;
	++s_lock_count;
	return true;
}

bool naui_path_is_locked(const Naui_Path path)
{
	if (path.data[0] == '\0')
		return false;

	char target[NAUI_PATH_MAX];
	_naui_resolve_lock_target(path.data, target);
	for (int i = 0; i < s_lock_count; ++i)
	{
		if (strcmp(s_locks[i].path, target) == 0)
			return true;
	}

	struct stat st;
	if (stat(target, &st) != 0)
		return false;

	int fd = open(target, O_RDWR | O_CLOEXEC);
	if (fd == -1)
		return false;

	bool locked = (flock(fd, LOCK_EX | LOCK_NB) != 0);
	if (!locked)
		flock(fd, LOCK_UN);

	close(fd);
	return locked;
}

void naui_path_unlock(const Naui_Path path)
{
	if (path.data[0] == '\0')
		return;

	char target[NAUI_PATH_MAX];
	_naui_resolve_lock_target(path.data, target);
	for (int i = 0; i < s_lock_count; ++i)
	{
		if (strcmp(s_locks[i].path, target) != 0)
			continue;

		flock(s_locks[i].fd, LOCK_UN);
		close(s_locks[i].fd);
		s_locks[i] = s_locks[--s_lock_count];
		return;
	}
}

#endif