#pragma once

#define NAUI_DIR_ITERATOR_HANDLE_SIZE (NAUI_PATH_MAX << 1)
#define NAUI_EXTENSIONS(...) (const char*[]){ __VA_ARGS__, NULL }

typedef struct Naui_DirIterator
{
	Naui_DirEntry entry;
	bool is_valid;
	bool case_sensitive;
	unsigned char _handle[NAUI_DIR_ITERATOR_HANDLE_SIZE];
	char _filter[NAUI_PATH_MAX >> 2];
	const char** _extensions;
} Naui_DirIterator;

Naui_DirIterator naui_dir_iterator_open(const Naui_Path path, const char* filter, const char** extensions, bool case_sensitive);

void naui_dir_iterator_next(Naui_DirIterator* it);
void naui_dir_iterator_close(Naui_DirIterator* it);
bool naui_dir_iterator_valid(const Naui_DirIterator* it);