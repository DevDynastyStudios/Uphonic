#define NAUI_THEME_KEY_SCRATCH_SIZE (1 << 13)

typedef struct { char *key; Naui_Color value; } Naui_ThemeColorEntry;
typedef struct { char *key; Naui_List(Naui_Color) value; } Naui_ThemeColorListEntry;

typedef struct
{
    Naui_Map(Naui_ThemeColorListEntry) color_list_map;
    Naui_Map(Naui_ThemeColorEntry) color_map;
    Naui_Arena key_arena;
}
Naui_ThemeState;
static Naui_ThemeState naui__theme_state;

static Naui_Color naui_color_from_hex(const char *hex)
{
    if (*hex == '#') hex++;
    size_t len = strlen(hex);

    uint32_t r = 0, g = 0, b = 0, a = 255;

#if defined(_MSC_VER)
    if (len == 8)
        sscanf_s(hex, "%02x%02x%02x%02x", &r, &g, &b, &a);
    else sscanf_s(hex, "%02x%02x%02x", &r, &g, &b);
#else
    if (len == 8)
        sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a);
    else sscanf(hex, "%02x%02x%02x", &r, &g, &b);
#endif

    Naui_Color color;
    color.r = (uint8_t)r;
    color.g = (uint8_t)g;
    color.b = (uint8_t)b;
    color.a = (uint8_t)a;
    return color;
}

void naui_panel_refresh_colors(void);

void naui_themes_initialize(void) { naui_arena_init(&naui__theme_state.key_arena, NAUI_THEME_KEY_SCRATCH_SIZE); }
void naui_themes_shutdown(void) { naui_arena_free(&naui__theme_state.key_arena); }

bool naui_load_theme(const char *file_name)
{
    const char *display_name = file_name ? file_name : "";
    const Naui_Path theme_path = naui_path_join(naui_directory_get(NAUI_DIR_ASSETS), NAUI_PATH("Themes", file_name));
    Naui_Path json_path;
    snprintf(json_path.data, NAUI_PATH_MAX, "%s.json", theme_path.data);

    if (!naui_path_exists(json_path))
    {
        naui_log(NAUI_LOG_WARNING, "Failed to load theme '%s': file not found (%s)", display_name, json_path.data);
        return false;
    }

    Naui_Json json = naui_json_parse_file(json_path);
    if (!json.root || json.root->type != NAUI_JSON_OBJECT)
    {
        if (json.error_line > 0)
            naui_log(NAUI_LOG_WARNING, "Failed to load theme '%s': %s (line %d, col %d)", display_name, json.error ? json.error : "not a JSON object", json.error_line, json.error_col);
        else
            naui_log(NAUI_LOG_WARNING, "Failed to load theme '%s': %s", display_name, json.error ? json.error : "not a JSON object");

        naui_json_free(&json);
        return false;
    }

    for (ptrdiff_t i = 0; i < naui_strmap_len(naui__theme_state.color_list_map); i++)
        naui_list_free(naui__theme_state.color_list_map[i].value);

    naui_strmap_free(naui__theme_state.color_list_map);
    naui_strmap_free(naui__theme_state.color_map);

    naui_arena_reset(&naui__theme_state.key_arena);

    NAUI_JSON_FOREACH(json.root, key, val)
    {
        char *key_str = (char*)naui_arena_alloc(&naui__theme_state.key_arena, 64);
        naui_json_copy_cstr(key, key_str, 64);

        if (val->type == NAUI_JSON_STRING)
        {
            char value_str[16];
            naui_json_copy_cstr(val, value_str, sizeof(value_str));
            naui_strmap_put(naui__theme_state.color_map, key_str, naui_color_from_hex(value_str));
        }
        else if (val->type == NAUI_JSON_ARRAY)
        {
            Naui_List(Naui_Color) list = NULL;
            naui_list_reserve(list, val->array.count);
            for (uint32_t i = 0; i < val->array.count; i++)
            {
                char value_str[16] = { 0 };
                naui_json_copy_cstr(
                    naui_json_array_get(val, i),
                    value_str,
                    sizeof(value_str)
                );
                naui_list_push(list, naui_color_from_hex(value_str));
            }
            naui_strmap_put(naui__theme_state.color_list_map, key_str, list);
        }
    }
    
    naui_json_free(&json);
    naui_panel_refresh_colors();
    return true;
}

void naui_theme_set_color(const char *name, Naui_Color color)
{
    ptrdiff_t index = naui_strmap_get_index(naui__theme_state.color_map, name);
    if (index >= 0)
    {
        naui__theme_state.color_map[index].value = color;
        return;
    }

    char *key = naui_arena_alloc(&naui__theme_state.key_arena, strlen(name) + 1);
    strcpy(key, name);
    naui_strmap_put(naui__theme_state.color_map, key, color);
	naui_panel_refresh_colors();
}

Naui_Color naui_theme_color(const char *name)
{
    return naui_strmap_get(naui__theme_state.color_map, name);
}

bool naui_theme_has(const char *name)
{
    return
        naui_strmap_get_index(naui__theme_state.color_map, name) >= 0 ||
        naui_strmap_get_index(naui__theme_state.color_list_map, name) >= 0;
}

const Naui_List(Naui_Color) naui_theme_color_list(const char *name)
{
    return naui_strmap_get(naui__theme_state.color_list_map, name);
}