static char naui__tolower(char c)
{
    return (char)tolower((unsigned char)c);
}

static int naui__char_eq(char a, char b, bool case_sensitive)
{
    if (case_sensitive)
        return a == b;
    return naui__tolower(a) == naui__tolower(b);
}

static size_t naui__clamp_start_len(size_t total_len, size_t start, size_t len, size_t *out_start)
{
    if (start > total_len)
        start = total_len;

    size_t max_len = total_len - start;
    if (len > max_len)
        len = max_len;

    *out_start = start;
    return len;
}

Naui_String naui_string_from_cstr(const char *cstr)
{
    Naui_String result;
    result.length = 0;
    result.data[0] = '\0';

    if (!cstr)
        return result;

    size_t src_len = strlen(cstr);
    size_t copy_len = NAUI_MIN(src_len, NAUI_STRING_MAX_SIZE - 1);

    memcpy(result.data, cstr, copy_len);
    result.data[copy_len] = '\0';
    result.length = copy_len;

    return result;
}

Naui_StringView naui_string_to_view(const Naui_String *str)
{
    Naui_StringView view;

    if (!str)
    {
        view.data = NULL;
        view.length = 0;
        return view;
    }

    view.data = (char *)str->data;
    view.length = str->length;
    return view;
}

Naui_String naui_view_to_string(Naui_StringView view)
{
    Naui_String result;
    result.length = 0;
    result.data[0] = '\0';

    if (!view.data || view.length == 0)
        return result;

    size_t copy_len = NAUI_MIN(view.length, NAUI_STRING_MAX_SIZE - 1);

    memcpy(result.data, view.data, copy_len);
    result.data[copy_len] = '\0';
    result.length = copy_len;

    return result;
}

Naui_StringView naui_sub_string(const Naui_String *str, size_t start, size_t len)
{
    Naui_StringView view;

    if (!str)
    {
        view.data = NULL;
        view.length = 0;
        return view;
    }

    size_t clamped_start = 0;
    size_t clamped_len = naui__clamp_start_len(str->length, start, len, &clamped_start);

    view.data = (char *)str->data + clamped_start;
    view.length = clamped_len;
    return view;
}

Naui_StringView naui_sub_string_view(Naui_StringView src, size_t start, size_t len)
{
    Naui_StringView view;
    size_t clamped_start = 0;
    size_t clamped_len = naui__clamp_start_len(src.length, start, len, &clamped_start);

    view.data = src.data ? src.data + clamped_start : NULL;
    view.length = clamped_len;
    return view;
}

char naui_string_pop(Naui_String *dest) {
    if (!dest->length)
        return '\0';
    
    char ch = dest->data[--dest->length];
    dest->data[dest->length] = '\0';
    
    return ch;
}

void naui_string_copy(Naui_String *dest, const Naui_String src)
{
    if (!dest)
        return;

    memcpy(dest->data, src.data, src.length);
    dest->data[src.length] = '\0';
    dest->length = src.length;
}

void naui_string_copy_view(Naui_String *dest, const Naui_StringView src)
{
    if (!dest)
        return;

    memcpy(dest->data, src.data, src.length);
    dest->data[src.length] = '\0';
    dest->length = src.length;
}

void naui_string_append_view(Naui_String *dest, Naui_StringView view)
{
    if (!dest || !view.data || view.length == 0)
        return;

    size_t space_left = (NAUI_STRING_MAX_SIZE - 1) - dest->length;
    size_t copy_len = NAUI_MIN(view.length, space_left);

    if (copy_len == 0)
        return;

    memcpy(dest->data + dest->length, view.data, copy_len);
    dest->length += copy_len;
    dest->data[dest->length] = '\0';
}

void naui_string_remove(Naui_String *dest, size_t index, size_t size)
{
    if (index + size > dest->length)
        return;

    memmove(
        &dest->data[index],
        &dest->data[index + size],
        dest->length - index - size + 1
    );

    dest->length -= size;

    memset(
        &dest->data[dest->length + 1],
        0,
        size
    );
}

void naui_string_append(Naui_String *dest, Naui_String str)
{
    naui_string_append_view(dest, naui_string_to_view(&str));
}

void naui_string_append_cstr(Naui_String *dest, const char *cstr)
{
    if (!dest || !cstr)
        return;

    Naui_StringView view;
    view.data = (char *)cstr;
    view.length = strlen(cstr);
    naui_string_append_view(dest, view);
}

void naui_string_append_char(Naui_String *dest, char ch)
{
    if (!dest)
        return;

    if (dest->length >= NAUI_STRING_MAX_SIZE - 1)
        return;

    dest->data[dest->length] = ch;
    dest->length += 1;
    dest->data[dest->length] = '\0';
}

void naui_string_append_char_at(Naui_String *dest, char ch, size_t index)
{
    if (dest->length >= NAUI_STRING_MAX_SIZE - 1 || index > dest->length)
        return;

    if (dest->length)
        memmove(
            &dest->data[index + 1],
            &dest->data[index],
            dest->length - index
        );

    dest->data[index] = ch;
    dest->length++;
}

size_t naui_string_view_find(Naui_StringView haystack, Naui_StringView needle, bool case_sensitive)
{
    if (!haystack.data || !needle.data)
        return (size_t)-1;

    if (needle.length == 0)
        return 0;

    if (needle.length > haystack.length)
        return (size_t)-1;

    size_t last_start = haystack.length - needle.length;
    for (size_t i = 0; i <= last_start; ++i)
    {
        size_t j = 0;
        for (; j < needle.length; ++j)
        {
            if (!naui__char_eq(haystack.data[i + j], needle.data[j], case_sensitive))
                break;
        }
        if (j == needle.length)
            return i;
    }

    return (size_t)-1;
}

size_t naui_string_find(Naui_String haystack, Naui_String needle, bool case_sensitive)
{
    return naui_string_view_find(naui_string_to_view(&haystack), naui_string_to_view(&needle), case_sensitive);
}

bool naui_string_view_contains(Naui_StringView haystack, Naui_StringView needle, bool case_sensitive)
{
    return naui_string_view_find(haystack, needle, case_sensitive) != (size_t)-1;
}

bool naui_string_contains(Naui_String haystack, Naui_String needle, bool case_sensitive)
{
    return naui_string_view_contains(naui_string_to_view(&haystack), naui_string_to_view(&needle), case_sensitive);
}

bool naui_string_views_equal(Naui_StringView view1, Naui_StringView view2, bool case_sensitive)
{
    if (view1.length != view2.length)
        return false;

    for (size_t i = 0; i < view1.length; ++i)
    {
        if (!naui__char_eq(view1.data[i], view2.data[i], case_sensitive))
            return false;
    }

    return true;
}

bool naui_strings_equal(Naui_String str1, Naui_String str2, bool case_sensitive)
{
    return naui_string_views_equal(naui_string_to_view(&str1), naui_string_to_view(&str2), case_sensitive);
}

static bool naui__is_space(char c)
{
    return isspace((unsigned char)c) != 0;
}

Naui_StringView naui_string_view_trim_left(Naui_StringView view)
{
    size_t start = 0;
    while (start < view.length && naui__is_space(view.data[start]))
        ++start;

    Naui_StringView result;
    result.data = view.data ? view.data + start : NULL;
    result.length = view.length - start;
    return result;
}

Naui_StringView naui_string_view_trim_right(Naui_StringView view)
{
    size_t end = view.length;
    while (end > 0 && naui__is_space(view.data[end - 1]))
        --end;

    Naui_StringView result;
    result.data = view.data;
    result.length = end;
    return result;
}

Naui_StringView naui_string_view_trim(Naui_StringView view)
{
    return naui_string_view_trim_right(naui_string_view_trim_left(view));
}

Naui_String naui_string_trim(Naui_String str)
{
    Naui_StringView trimmed = naui_string_view_trim(naui_string_to_view(&str));
    return naui_view_to_string(trimmed);
}

void naui_string_to_lower_inplace(Naui_String *str)
{
    if (!str)
        return;

    for (size_t i = 0; i < str->length; ++i)
        str->data[i] = (char)tolower((unsigned char)str->data[i]);
}

void naui_string_to_upper_inplace(Naui_String *str)
{
    if (!str)
        return;

    for (size_t i = 0; i < str->length; ++i)
        str->data[i] = (char)toupper((unsigned char)str->data[i]);
}

Naui_String naui_string_to_lower(Naui_String str)
{
    Naui_String result = str;
    naui_string_to_lower_inplace(&result);
    return result;
}

Naui_String naui_string_to_upper(Naui_String str)
{
    Naui_String result = str;
    naui_string_to_upper_inplace(&result);
    return result;
}

size_t naui_string_view_split(Naui_StringView view, char delim, Naui_StringView *out_parts, size_t max_parts)
{
    if (!out_parts || max_parts == 0)
        return 0;

    if (!view.data || view.length == 0)
    {
        out_parts[0].data = view.data;
        out_parts[0].length = 0;
        return 1;
    }

    size_t part_count = 0;
    size_t segment_start = 0;

    for (size_t i = 0; i < view.length; ++i)
    {
        if (part_count + 1 >= max_parts)
            break;

        if (view.data[i] == delim)
        {
            out_parts[part_count].data = view.data + segment_start;
            out_parts[part_count].length = i - segment_start;
            ++part_count;
            segment_start = i + 1;
        }
    }

    out_parts[part_count].data = view.data + segment_start;
    out_parts[part_count].length = view.length - segment_start;
    ++part_count;

    return part_count;
}

size_t naui_string_split(const Naui_String *str, char delim, Naui_StringView *out_parts, size_t max_parts)
{
    return naui_string_view_split(naui_string_to_view(str), delim, out_parts, max_parts);
}

Naui_String naui_string_replace(Naui_String str, Naui_String find, Naui_String replace, bool case_sensitive)
{
    Naui_String result;
    result.length = 0;
    result.data[0] = '\0';

    Naui_StringView src_view = naui_string_to_view(&str);
    Naui_StringView find_view = naui_string_to_view(&find);

    if (find.length == 0)
    {
        naui_string_copy(&result, str);
        return result;
    }

    size_t pos = 0;
    while (pos <= src_view.length)
    {
        Naui_StringView remaining;
        remaining.data = src_view.data + pos;
        remaining.length = src_view.length - pos;

        size_t found = naui_string_view_find(remaining, find_view, case_sensitive);

        if (found == (size_t)-1)
        {
            naui_string_append_view(&result, remaining);
            break;
        }

        Naui_StringView before;
        before.data = remaining.data;
        before.length = found;
        naui_string_append_view(&result, before);

        naui_string_append(&result, replace);

        pos += found + find.length;
    }

    return result;
}

bool naui_string_view_starts_with(Naui_StringView view, Naui_StringView prefix, bool case_sensitive)
{
    if (prefix.length > view.length)
        return false;

    Naui_StringView head;
    head.data = view.data;
    head.length = prefix.length;

    return naui_string_views_equal(head, prefix, case_sensitive);
}

bool naui_string_view_ends_with(Naui_StringView view, Naui_StringView suffix, bool case_sensitive)
{
    if (suffix.length > view.length)
        return false;

    Naui_StringView tail;
    tail.data = view.data + (view.length - suffix.length);
    tail.length = suffix.length;

    return naui_string_views_equal(tail, suffix, case_sensitive);
}

bool naui_string_starts_with(Naui_String str, Naui_String prefix, bool case_sensitive)
{
    return naui_string_view_starts_with(naui_string_to_view(&str), naui_string_to_view(&prefix), case_sensitive);
}

bool naui_string_ends_with(Naui_String str, Naui_String suffix, bool case_sensitive)
{
    return naui_string_view_ends_with(naui_string_to_view(&str), naui_string_to_view(&suffix), case_sensitive);
}

bool naui_string_is_empty(Naui_String str)
{
    return str.length == 0;
}

bool naui_string_view_is_empty(Naui_StringView view)
{
    return view.length == 0;
}

Naui_String naui_string_format(char *const fmt, ...)
{
    Naui_String result;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(result.data, NAUI_STRING_MAX_SIZE, fmt, ap);
    va_end(ap);

    return result;
}
