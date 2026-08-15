typedef struct
{
    char *data;
    size_t length;
}
Naui_StringView;

typedef char *Naui_String;
typedef Naui_List(char) Naui_StringBuilder;

NAUI_API size_t             naui_string_len                     (Naui_String str);
NAUI_API Naui_StringView    naui_string_to_view                 (Naui_String str);

NAUI_API Naui_StringView    naui_sub_string                     (Naui_String str, size_t start, size_t length);
NAUI_API Naui_StringView    naui_sub_string_view                (Naui_StringView view, size_t start, size_t length);

NAUI_API Naui_String        naui_view_to_string                 (Naui_Arena *arena, Naui_StringView view);

NAUI_API Naui_String        naui_string_copy                    (Naui_Arena *arena, Naui_String str);

NAUI_API Naui_String        naui_capped_string_alloc            (Naui_Arena *arena, size_t max_length);
NAUI_API size_t             naui_capped_string_len              (Naui_String str);
NAUI_API size_t             naui_capped_string_capacity         (Naui_String str);
NAUI_API bool               naui_capped_string_append           (Naui_String str, Naui_String suffix);
NAUI_API bool               naui_capped_string_append_view      (Naui_String str, Naui_StringView suffix);
NAUI_API bool               naui_capped_string_append_char      (Naui_String str, char c);

static inline void          naui_string_builder_reserve         (Naui_StringBuilder builder, size_t capacity) { naui_list_reserve(builder, capacity); }
static inline void          naui_string_builder_clear           (Naui_StringBuilder builder) { naui_list_clear(builder); }
static inline void          naui_string_builder_free            (Naui_StringBuilder builder) { naui_list_free(builder); }
static inline size_t        naui_string_builder_len             (Naui_StringBuilder builder) { return naui_list_len(builder); }

NAUI_API void               naui_string_builder_append          (Naui_StringBuilder builder, Naui_String str);
NAUI_API void               naui_string_builder_append_view     (Naui_StringBuilder builder, Naui_StringView view);
NAUI_API void               naui_string_builder_append_char     (Naui_StringBuilder builder, char c);
NAUI_API Naui_StringView    naui_string_builder_to_view         (Naui_StringBuilder builder);

NAUI_API bool               naui_string_view_contains           (Naui_StringView haystack, Naui_StringView needle, bool case_sensitive);
NAUI_API bool               naui_string_contains                (Naui_String haystack, Naui_String needle, bool case_sensitive);

NAUI_API bool               naui_strings_equal                  (Naui_String str1, Naui_String str2, bool case_sensitive);
NAUI_API bool               naui_strings_with_len_equal         (Naui_String str1, Naui_String str2, size_t len, bool case_sensitive);

