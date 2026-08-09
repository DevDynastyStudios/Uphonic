typedef struct {
    char *data;
    size_t length;
} Naui_StringView;

typedef Naui_List(char) Naui_String;

#define naui_string(cstr) (Naui_StringView){ (char*)(cstr), sizeof(cstr) - 1 } // Pretty damn convenient
#define naui_string_spread(s) (int)(s).length, (s).data
#define naui_string_fmt "%.*s"
NAUI_API bool naui_string_is_valid(Naui_StringView string_view);
NAUI_API Naui_StringView naui_string_from_cstring(char *s);
NAUI_API bool naui_string_eq(Naui_StringView a, Naui_StringView b, bool case_sensitive);
NAUI_API bool naui_string_contains(Naui_StringView string_view, Naui_StringView substring);
NAUI_API bool naui_string_starts_with(Naui_StringView string_view, Naui_StringView substring);
NAUI_API bool naui_string_ends_with(Naui_StringView string_view, Naui_StringView substring);
NAUI_API Naui_StringView naui_string_substring(Naui_StringView string_view, size_t start, size_t count);
NAUI_API Naui_StringView naui_string_find(Naui_StringView haystack, Naui_StringView needle);
NAUI_API char *naui_string_find_char(Naui_StringView haystack, char needle);
NAUI_API Naui_StringView naui_string_trim_left(Naui_StringView string_view);
NAUI_API Naui_StringView naui_string_trim_right(Naui_StringView string_view);
NAUI_API Naui_StringView naui_string_trim(Naui_StringView string_view);
// String functions that involve allocations
NAUI_API Naui_StringView naui_string_clone(Naui_Arena *arena, Naui_StringView string_view);
NAUI_API Naui_StringView naui_string_clone_from_cstring(Naui_Arena *arena, char *s);
NAUI_API Naui_StringView naui_string_clone_from_bytes(Naui_Arena *arena, char *s, size_t len);
NAUI_API char *naui_string_clone_to_cstring(Naui_Arena *arena, Naui_StringView string_view);
NAUI_API Naui_StringView naui_string_to_lower(Naui_Arena *arena, Naui_StringView string_view);
NAUI_API Naui_StringView naui_string_to_upper(Naui_Arena *arena, Naui_StringView string_view);
NAUI_API Naui_StringView naui_string_concat(Naui_Arena *arena, Naui_StringView a, Naui_StringView b);
NAUI_API Naui_StringView naui_string_replace(Naui_Arena *arena, Naui_StringView string_view, Naui_StringView find, Naui_StringView replace);
/* TODO(doomguy)
NAUI_API naui_string_slice naui_string_split(Naui_Arena *arena, Naui_StringView string_view, Naui_StringView seperator);
NAUI_API naui_string_slice naui_string_split_lines(Naui_Arena *arena, Naui_StringView string_view);
NAUI_API naui_string_slice naui_string_split_char(Naui_Arena *arena, Naui_StringView string_view, char seperator);
*/

NAUI_API Naui_String naui_sb_create(void);
NAUI_API void naui_sb_destroy(Naui_String string);
NAUI_API Naui_StringView naui_sb_to_string(Naui_String string);

#define naui_sb_append_string(string, ...) naui_sb_append_string_null(string, __VA_ARGS__, (Naui_StringView){0});
NAUI_API void naui_sb_append_string_null(Naui_String string, ...);

// functions needed by iterator_win32 and iterator_unix
int naui_cstr_strcmp(const char *str1, const char *str2, bool case_sensitive);
int naui_cstr_strncmp(const char *str1, const char *str2, size_t len, bool case_sensitive);
