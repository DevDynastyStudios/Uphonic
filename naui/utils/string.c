size_t naui_string_len(Naui_String str)
{
    return strlen(str);
}

Naui_StringView naui_string_to_view(Naui_String str)
{
    return (Naui_StringView) {
        .data = str,
        .length = strlen(str)
    };
}

Naui_StringView naui_substring(Naui_String str, size_t start, size_t length)
{
    return (Naui_StringView) {
        .data = str + start,
        .length = length
    };
}

Naui_StringView naui_sub_string_view(Naui_StringView view, size_t start, size_t length)
{
    return (Naui_StringView) {
        .data = view.data + start,
        .length = length
    };
}

Naui_String naui_view_to_string(Naui_Arena *arena, Naui_StringView view)
{
    Naui_String string = naui_arena_alloc(arena, view.length + 1);
    memcpy(string, view.data, view.length);
    string[view.length] = '\0';
    return string;
}

Naui_String naui_string_copy(Naui_Arena *arena, Naui_String str)
{
    const size_t length = strlen(str) + 1;
    Naui_String string = naui_arena_alloc(arena, length);
    memcpy(string, str, length);
    return string;
}

typedef struct
{
    size_t length;
    size_t capacity;
}
Naui__CappedStringHeader;

#define NAUI__CAPPED_HEADER(str) ((Naui__CappedStringHeader *)(str) - 1)

Naui_String naui_capped_string_alloc(Naui_Arena *arena, size_t max_length)
{
    void *block = naui_arena_alloc(arena, sizeof(Naui__CappedStringHeader) + max_length + 1);

    Naui__CappedStringHeader *hdr = (Naui__CappedStringHeader *)block;
    hdr->length   = 0;
    hdr->capacity = max_length;

    Naui_String str = (Naui_String)(hdr + 1);
    str[0] = '\0';

    return str;
}

size_t naui_capped_string_len(Naui_String str)
{
    return NAUI__CAPPED_HEADER(str)->length;
}

size_t naui_capped_string_capacity(Naui_String str)
{
    return NAUI__CAPPED_HEADER(str)->capacity;
}

bool naui_capped_string_append_view(Naui_String str, Naui_StringView suffix)
{
    Naui__CappedStringHeader *hdr = NAUI__CAPPED_HEADER(str);

    if (hdr->length + suffix.length > hdr->capacity)
        return false;

    memcpy(str + hdr->length, suffix.data, suffix.length);
    hdr->length += suffix.length;
    str[hdr->length] = '\0';

    return true;
}

bool naui_capped_string_append(Naui_String str, Naui_String suffix)
{
    return naui_capped_string_append_view(str, naui_string_to_view(suffix));
}

bool naui_capped_string_append_char(Naui_String str, char c)
{
    Naui__CappedStringHeader *hdr = NAUI__CAPPED_HEADER(str);

    if (hdr->length + 1 > hdr->capacity)
        return false;

    str[hdr->length] = c;
    hdr->length += 1;
    str[hdr->length] = '\0';

    return true;
}

void naui_string_builder_append(Naui_StringBuilder builder, Naui_String str)
{
    naui_string_builder_append_view(builder, naui_string_to_view(str));
}

void naui_string_builder_append_view(Naui_StringBuilder builder, Naui_StringView view)
{
    if (view.length == 0)
        return;

    const size_t old_len = naui_string_builder_len(builder);
    naui_list_reserve(builder, old_len + view.length + 1);
    memcpy(builder + old_len - 1, view.data, view.length);
    naui_list_push(builder, '\0');
}

void naui_string_builder_append_char(Naui_StringBuilder builder, char c)
{
    const size_t old_len = naui_string_builder_len(builder);
    builder[old_len - 1] = c;
    naui_list_push(builder, '\0');
}

Naui_StringView naui_string_builder_to_view(Naui_StringBuilder builder)
{
    return (Naui_StringView) {
        .data = builder,
        .length = naui_list_len(builder) - 1
    };
}

bool naui_string_view_contains(Naui_StringView haystack, Naui_StringView needle, bool case_sensitive)
{
    if (needle.length == 0)
        return true;

    if (needle.length > haystack.length)
        return false;

    for (size_t i = 0; i + needle.length <= haystack.length; i++)
    {
        Naui_StringView slice = {
            .data = haystack.data + i,
            .length = needle.length
        };

        if (naui_strings_with_len_equal(slice.data, needle.data, needle.length, case_sensitive))
            return true;
    }

    return false;
}

bool naui_string_contains(Naui_String haystack, Naui_String needle, bool case_sensitive)
{
    return naui_string_view_contains(
        naui_string_to_view(haystack),
        naui_string_to_view(needle),
        case_sensitive
    );
}

bool naui_strings_equal(Naui_String str1, Naui_String str2, bool case_sensitive)
{
    if (case_sensitive)
        return !strcmp(str1, str2);
#if NAUI_WINDOWS
    return !_stricmp(str1, str2);
#else
    return !strcasecmp(str1, str2);
#endif
}

bool naui_strings_with_len_equal(Naui_String str1, Naui_String str2, size_t len, bool case_sensitive)
{
    if (case_sensitive)
        return !strncmp(str1, str2, len);
#if NAUI_WINDOWS
    return !_strnicmp(str1, str2, len);
#else
    return !strncasecmp(str1, str2, len);
#endif
}

bool naui_string_views_equal(Naui_StringView view1, Naui_StringView view2, bool case_sensitive)
{
    if (view1.length != view2.length)
        return false;

    return naui_strings_with_len_equal(
        view1.data,
        view2.data,
        view1.length,
        case_sensitive
    );
}