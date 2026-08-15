typedef struct
{
    char *data;
    size_t length;
}
Naui_StringView;

#define NAUI_STRING_MAX_SIZE 128
typedef struct
{
    char data[NAUI_STRING_MAX_SIZE];
    size_t len;
}
Naui_String;

NAUI_API Naui_String        naui_string_from_cstr               (const char *cstr);
NAUI_API Naui_StringView    naui_string_to_view                 (const Naui_String *str);
NAUI_API Naui_String        naui_view_to_string                 (Naui_StringView view);

NAUI_API Naui_StringView    naui_sub_string                     (const Naui_String *str, size_t start, size_t len);
NAUI_API Naui_StringView    naui_sub_string_view                (Naui_StringView view, size_t start, size_t len);

NAUI_API void               naui_string_copy                    (Naui_String *dest, const Naui_String src);
NAUI_API void               naui_string_append                  (Naui_String *dest, Naui_String str);
NAUI_API void               naui_string_append_char             (Naui_String *dest, char ch);
NAUI_API void               naui_string_append_cstr             (Naui_String *dest, const char *cstr);
NAUI_API void               naui_string_append_view             (Naui_String *dest, Naui_StringView view);

NAUI_API bool               naui_string_view_contains           (Naui_StringView haystack, Naui_StringView needle, bool case_sensitive);
NAUI_API bool               naui_string_contains                (Naui_String haystack, Naui_String needle, bool case_sensitive);

NAUI_API bool               naui_strings_equal                  (Naui_String str1, Naui_String str2, bool case_sensitive);
NAUI_API bool               naui_strings_with_len_equal         (Naui_String str1, Naui_String str2, size_t len, bool case_sensitive);
NAUI_API bool               naui_string_views_equal             (Naui_StringView view1, Naui_StringView view2, bool case_sensitive);

NAUI_API Naui_StringView    naui_string_view_trim               (Naui_StringView view);
NAUI_API Naui_StringView    naui_string_view_trim_left          (Naui_StringView view);
NAUI_API Naui_StringView    naui_string_view_trim_right         (Naui_StringView view);
NAUI_API Naui_String        naui_string_trim                    (Naui_String str);

NAUI_API Naui_String        naui_string_to_lower                (Naui_String str);
NAUI_API Naui_String        naui_string_to_upper                (Naui_String str);
NAUI_API void               naui_string_to_lower_inplace        (Naui_String *str);
NAUI_API void               naui_string_to_upper_inplace        (Naui_String *str);

NAUI_API size_t              naui_string_split                  (const Naui_String *str, char delim, Naui_StringView *out_parts, size_t max_parts);
NAUI_API size_t              naui_string_view_split             (Naui_StringView view, char delim, Naui_StringView *out_parts, size_t max_parts);

NAUI_API Naui_String        naui_string_replace                 (Naui_String str, Naui_String find, Naui_String replace, bool case_sensitive);

NAUI_API size_t              naui_string_view_find              (Naui_StringView haystack, Naui_StringView needle, bool case_sensitive);
NAUI_API size_t              naui_string_find                   (Naui_String haystack, Naui_String needle, bool case_sensitive);

NAUI_API bool                naui_string_view_starts_with       (Naui_StringView view, Naui_StringView prefix, bool case_sensitive);
NAUI_API bool                naui_string_view_ends_with         (Naui_StringView view, Naui_StringView suffix, bool case_sensitive);
NAUI_API bool                naui_string_starts_with            (Naui_String str, Naui_String prefix, bool case_sensitive);
NAUI_API bool                naui_string_ends_with              (Naui_String str, Naui_String suffix, bool case_sensitive);

NAUI_API bool                naui_string_is_empty               (Naui_String str);
NAUI_API bool                naui_string_view_is_empty          (Naui_StringView view);

// typedef Naui_List(char) Naui_StringBuilder;
/*static inline void        naui_string_builder_reserve         (Naui_StringBuilder builder, size_t capacity) { naui_list_reserve(builder, capacity); }
static inline void          naui_string_builder_clear           (Naui_StringBuilder builder) { naui_list_clear(builder); }
static inline void          naui_string_builder_free            (Naui_StringBuilder builder) { naui_list_free(builder); }
static inline size_t        naui_string_builder_len             (Naui_StringBuilder builder) { return naui_list_len(builder); }

NAUI_API void               naui_string_builder_append          (Naui_StringBuilder builder, Naui_String str);
NAUI_API void               naui_string_builder_append_builder  (Naui_StringBuilder dest_builder, Naui_StringBuilder src_builder);
NAUI_API void               naui_string_builder_append_view     (Naui_StringBuilder builder, Naui_StringView view);
NAUI_API void               naui_string_builder_append_char     (Naui_StringBuilder builder, char c);
NAUI_API Naui_StringView    naui_string_builder_to_view         (Naui_StringBuilder builder);*/