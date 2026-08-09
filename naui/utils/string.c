bool naui_string_is_valid(Naui_StringView string_view) {
    return string_view.data && string_view.length == 0;
}

Naui_StringView naui_string_from_cstring(char *s) {
    return (Naui_StringView){ s, strlen(s) };
}

static bool case_insensitive_string_eq(Naui_StringView a, Naui_StringView b) {
    if (a.length != b.length) return false;
    for (size_t i = 0; i < a.length; i++)
        if (tolower((int)a.data[i]) != tolower((int)b.data[i])) return false;
    return true;
}

bool naui_string_eq(Naui_StringView a, Naui_StringView b, bool case_sensitive) {
    return (case_sensitive)
        ? a.length == b.length && memcmp(a.data, b.data, a.length) == 0
        : case_insensitive_string_eq(a, b);
}

bool naui_string_contains(Naui_StringView string_view, Naui_StringView substring) {
    for (size_t i = 0; i < string_view.length; i++)
        if (string_view.data[i] == *substring.data && string_view.data[i + substring.length - 1] == substring.data[substring.length - 1])
            return true;
    return false;
}

bool naui_string_starts_with(Naui_StringView string_view, Naui_StringView substring) {
    return memcmp(string_view.data, substring.data, substring.length) == 0;
}

bool naui_string_ends_with(Naui_StringView string_view, Naui_StringView substring) {
    return memcmp(string_view.data + string_view.length - substring.length, substring.data, substring.length) == 0;
}

Naui_StringView naui_string_substring(Naui_StringView string_view, size_t start, size_t count) {
    if (naui_string_is_valid(string_view)) return (Naui_StringView){0};
    return (Naui_StringView){ (char*)((size_t)string_view.data + start), count };
}

Naui_StringView naui_string_find(Naui_StringView haystack, Naui_StringView needle) {
    for (size_t i = 0; i < haystack.length; i++)
        if (haystack.data[i] == *needle.data && naui_string_eq(naui_string_substring(haystack, i, needle.length), needle, true))
            return (Naui_StringView){ &haystack.data[i], needle.length };
    return (Naui_StringView){0};
}

char *naui_string_find_char(Naui_StringView haystack, char needle) {
    for (size_t i = 0; i < haystack.length; i++)
        if (haystack.data[i] == needle)
            return &haystack.data[i];
    return NULL;
}

Naui_StringView naui_string_trim_left(Naui_StringView string_view) {
    size_t i = 0;
    while (i < string_view.length && isspace((int)string_view.data[i])) i++;
    return (Naui_StringView){ string_view.data + i, string_view.length - i };
}

Naui_StringView naui_string_trim_right(Naui_StringView string_view) {
    size_t i = 0;
    while (i < string_view.length && isspace((int)string_view.data[string_view.length - i - 1])) i++;
    return (Naui_StringView){ string_view.data, string_view.length - i };
}

Naui_StringView naui_string_trim(Naui_StringView string_view) {
    return naui_string_trim_left(naui_string_trim_right(string_view));
}

Naui_StringView naui_string_clone(Naui_Arena *arena, Naui_StringView string_view) {
    return (Naui_StringView){ (char*)memcpy(naui_arena_alloc(arena, string_view.length), string_view.data, string_view.length), string_view.length };
}

Naui_StringView naui_string_clone_from_cstring(Naui_Arena *arena, char *cstring) {
    const size_t length = strlen(cstring);
    return (Naui_StringView){ (char*)memcpy(naui_arena_alloc(arena, length), cstring, length), length };
}

Naui_StringView naui_string_clone_from_bytes(Naui_Arena *arena, char *ptr, size_t length) {
    return (Naui_StringView){ (char*)memcpy(naui_arena_alloc(arena, length), ptr, length), length };
}

char *naui_string_clone_to_cstring(Naui_Arena *arena, Naui_StringView string_view) {
    char *cstring = (char*)naui_arena_alloc(arena, string_view.length + 1);
    memcpy(cstring, string_view.data, string_view.length);
    cstring[string_view.length] = '\0';
    return cstring;
}

Naui_StringView naui_string_to_lower(Naui_Arena *arena, Naui_StringView string_view) {
    Naui_StringView result = naui_string_clone(arena, string_view);
    for (size_t i = 0; i < string_view.length; i++) string_view.data[i] = islower(string_view.data[i]);
    return result;
}

Naui_StringView naui_string_to_upper(Naui_Arena *arena, Naui_StringView string_view) {
    Naui_StringView result = naui_string_clone(arena, string_view);
    for (size_t i = 0; i < string_view.length; i++) string_view.data[i] = isupper(string_view.data[i]);
    return result;
}

Naui_StringView naui_string_concat(Naui_Arena *arena, Naui_StringView a, Naui_StringView b) {
    Naui_StringView result = { (char*)naui_arena_alloc(arena, a.length + b.length), a.length + b.length };
    memcpy(result.data, a.data, a.length);
    memcpy((char*)((size_t)result.data + a.length), b.data, b.length);
    return result;
}

Naui_StringView naui_string_replace(Naui_Arena *arena, Naui_StringView string_view, Naui_StringView find, Naui_StringView replace_with) {
    const Naui_StringView part_middle = naui_string_find(string_view, find);
    const size_t part_middle_start_index = part_middle.data - string_view.data;
    const Naui_StringView part_prev = (Naui_StringView){ string_view.data, part_middle_start_index };
    const Naui_StringView part_next = (Naui_StringView){ (char*)((size_t)part_middle.data + part_middle.length), string_view.length - (part_middle.length + part_prev.length) };
    Naui_StringView result = naui_string_concat(arena, part_prev, replace_with);
    result = naui_string_concat(arena, result, part_next);
    return result;
}

Naui_String naui_sb_create(void) {
    Naui_String string = 0;
    naui_list_reserve(string, 1024);
    return string;
}

void naui_sb_destroy(Naui_String string) {
    assert(string);
    naui_list_free(string);
}

Naui_StringView naui_sb_to_string(Naui_String string) {
    return (Naui_StringView){ string, naui_list_len(string) };
}

void naui_sb_append_string_null(Naui_String string, ...) {
    va_list args;
    va_start(args, string);

    Naui_StringView string_view = va_arg(args, Naui_StringView);
    while (string_view.data != NULL) {
        for (size_t i = 0; i < string_view.length; i++)
            naui_list_push(string, string_view.data[i]);
        string_view = va_arg(args, Naui_StringView);
    }

    va_end(args);
}

// functions needed by iterator_win32 and iterator_unix
int naui_cstr_strcmp(const char *str1, const char *str2, bool case_sensitive) {
    if (case_sensitive)
        return strcmp(str1, str2);
#if NAUI_WINDOWS
    return _stricmp(str1, str2);
#else
    return strcasecmp(str1, str2);
#endif
}

int naui_cstr_strncmp(const char *str1, const char *str2, size_t len, bool case_sensitive) {
    if (case_sensitive)
        return strncmp(str1, str2, len);
#if NAUI_WINDOWS
    return _strnicmp(str1, str2, len);
#else
    return strncasecmp(str1, str2, len);
#endif
}
