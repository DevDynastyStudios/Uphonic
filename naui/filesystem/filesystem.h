#ifndef NAUI_PATH_MAX
#if defined(_WIN32) || defined(_WIN64)
#	define NAUI_PATH_MAX 1024
#	define NAUI_PATH_SEP '\\'
#else
#	define NAUI_PATH_MAX 4096
#	define NAUI_PATH_SEP '/'
#endif
#endif

typedef struct Naui_Path
{
	char data[NAUI_PATH_MAX];
} Naui_Path;

#define NAUI_PATH(...) naui_path_join_parts((const char*[]){ __VA_ARGS__, NULL })

typedef uint8_t Naui_Dir;
enum
{
	NAUI_DIR_HOME,
	NAUI_DIR_BIN,
	NAUI_DIR_WORKING,
	NAUI_DIR_APPDATA,
	NAUI_DIR_ROAMING,
	NAUI_DIR_DOWNLOADS,
	NAUI_DIR_TEMP,
	NAUI_DIR_CACHE,
	NAUI_DIR_CONFIG
};

typedef uint8_t Naui_FileMode;
enum
{
	NAUI_FILE_READ,
	NAUI_FILE_WRITE,
	NAUI_FILE_APPEND
};

#define NAUI_FILE_HANDLE_SIZE 64
typedef struct Naui_FileHandle
{
	unsigned char _opaque[NAUI_FILE_HANDLE_SIZE];
} Naui_FileHandle;

#define NAUI_FILE_HANDLE_INIT { {0} }

typedef struct Naui_DirEntry
{
	Naui_Path path;
	bool is_directory;
	size_t size;
} Naui_DirEntry;

/* Open a file. Returns true on success. */
bool naui_file_open(Naui_FileHandle* handle, const Naui_Path path, Naui_FileMode mode);

/* Read/write up to `size` bytes.
 * Returns bytes actually transferred. */
size_t naui_file_read(const Naui_FileHandle* handle, void* restrict buffer, size_t size);
size_t naui_file_write(const Naui_FileHandle* handle, const void* restrict buffer, size_t size);

/* Seek within a file. `origin`: SEEK_SET / SEEK_CUR / SEEK_END.
 * Returns false on invalid handle or seek failure. */
bool naui_file_seek(Naui_FileHandle* handle, long offset, int origin);

/* Returns false if the handle was never opened or has been closed. */
bool naui_file_is_valid(const Naui_FileHandle* handle);

void naui_file_close(Naui_FileHandle* handle);

bool naui_file_copy(const Naui_Path file_path, const Naui_Path dest_path);

/* Returns file size in bytes, or 0 on error. */
size_t naui_file_size(const Naui_Path path);

/* Read entire file into a heap buffer.
 * Sets *out_size to bytes read (excludes null terminator). NULL on failure. */
char* naui_file_read_all(const Naui_Path path, size_t* out_size);

/* Write `size` bytes from `data` to path, creating or truncating the file. */
bool naui_file_write_all(const Naui_Path path, const void* data, size_t size);

/* Delete or rename a file. */
bool naui_file_delete(const Naui_Path path);
bool naui_file_rename(const Naui_Path old_path, const Naui_Path new_path);

/* Hide or unhide a file. */
Naui_Path naui_file_hide(const Naui_Path* path, bool hidden);
bool naui_file_is_hidden(const Naui_Path *path);

/* Returns a Naui_StringView of the filename. */
Naui_StringView naui_file_filename(const Naui_Path* path);

/* Returns the filename without its extension, as a Naui_StringView.
 * Returns an empty Naui_StringView if there is no stem. */
Naui_StringView naui_file_stem(const Naui_Path* path);

/* Returns the extension including the dot, as a Naui_StringView.
 * Returns an empty Naui_StringView if there is no extension. */
Naui_StringView naui_file_extension(const Naui_Path* path);

bool naui_directory_create(const Naui_Path path);
bool naui_directory_remove(const Naui_Path path);
bool naui_directory_remove_all(const Naui_Path path);
bool naui_directory_rename(const Naui_Path old_path, const Naui_Path new_path);
bool naui_directories_create(const Naui_Path path);

/* Returns a Naui_Path for a well-known directory.
 * Returns an empty Naui_Path on failure. */
Naui_Path naui_directory_get(const Naui_Dir directory);

/* List directory entries.
 * Returns a Naui_List(Naui_DirEntry). Call naui_directory_filter_free() when done. */
Naui_List(Naui_DirEntry) naui_directory_filter(const Naui_Path path, const char* filter, const char** extensions);

/* List directory and sub-directory entries.
 * Returns a Naui_List(Naui_DirEntry). Call naui_directory_filter_free() when done. */
Naui_List(Naui_DirEntry) naui_directory_filter_recursive(const Naui_Path path, const char* filter, const char** extensions);

/* Frees the list returned by naui_directory_filter. */
void naui_directory_filter_free(Naui_List(Naui_DirEntry) list);

bool naui_directories_create(const Naui_Path path);

bool naui_path_set_current(const Naui_Path current_directory);

/* Returns true if path exists on the filesystem. */
bool naui_path_exists(const Naui_Path path);

/* Returns true if path is a directory on the filesystem. */
bool naui_path_is_directory(const Naui_Path path);

/* Returns true if path is empty. */
bool naui_path_is_empty(const Naui_Path path);

Naui_Path naui_path_from_cstr(const char* str);

/* Returns the parent directory. */
Naui_Path naui_path_parent(const Naui_Path path);

/* Join two path components, inserting a separator as needed. */
Naui_Path naui_path_join(const Naui_Path a, const Naui_Path b);

/* Join an arbitrary number of C-string path parts into one Naui_Path.
 * `parts` must be NULL-terminated.
 * Prefer the NAUI_PATH macro over calling this directly. */
Naui_Path naui_path_join_parts(const char** parts);

/* Resolve `.` and `..` components without hitting the filesystem. */
Naui_Path naui_path_normalize(const Naui_Path path);

/* Returns the absolute path by prepending the current working directory */
Naui_Path naui_path_absolute(const Naui_Path path);

/* The path must exist. Returns an empty Naui_Path on failure. */
Naui_Path naui_path_canonical(const Naui_Path path);

/* Resolves as much of the path as possible, then normalizes the rest. */
Naui_Path naui_path_weakly_canonical(const Naui_Path path);

bool naui_path_lock(const Naui_Path path);
bool naui_path_is_locked(const Naui_Path path);
void naui_path_unlock(const Naui_Path path);