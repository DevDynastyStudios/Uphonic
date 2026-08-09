#define NAUI_THEME_KEY_SCRATCH_SIZE (1 << 13)

typedef struct { char *key; float value; } Naui_ThemeFloatEntry;
typedef struct { char *key; Naui_Vec2 value; } Naui_ThemeVec2Entry;
typedef struct { char *key; Naui_Color value; } Naui_ThemeColorEntry;

typedef struct
{
    Naui_Map(Naui_ThemeColorEntry) color_map;
    Naui_Map(Naui_ThemeFloatEntry) float_map;
    Naui_Map(Naui_ThemeVec2Entry) vec2_map;
    Naui_Arena key_arena;
}
Naui_ThemeData;
static Naui_ThemeData tm;

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

void naui_themes_initialize(void) { naui_arena_init(&tm.key_arena, NAUI_THEME_KEY_SCRATCH_SIZE); }
void naui_themes_shutdown(void) { naui_arena_free(&tm.key_arena); }

// TODO(doomguy): move this into asset_manager
void naui_load_theme(const char *file_name)
{
    naui_arena_reset(&tm.key_arena);

    char final_file_name[64];
    strncpy(final_file_name, file_name, strlen(file_name) + 1);
    strncat(final_file_name, ".json", sizeof(final_file_name) - 1);
	Naui_Path json_path = NAUI_PATH("Assets/Themes", final_file_name);
    Naui_Json json = naui_json_parse_file(json_path);

    NAUI_JSON_FOREACH(json.root, key, val)
    {
        char *key_str = (char*)naui_arena_alloc(&tm.key_arena, 64);
        naui_json_copy_string(key, key_str, 64);

        if (val->type == NAUI_JSON_STRING)
        {
            char value_str[16];
            naui_json_copy_string(val, value_str, sizeof(value_str));
            naui_strmap_put(tm.color_map, key_str, naui_color_from_hex(value_str));
        }
        else if (val->type == NAUI_JSON_ARRAY)
        {
            Naui_Vec2 vec2 = {
                (float)naui_json_get_number(naui_json_array_get(val, 0), 0.0f),
                (float)naui_json_get_number(naui_json_array_get(val, 1), 0.0f),
            };
            naui_strmap_put(tm.vec2_map, key_str, vec2);
        }
        else if (val->type == NAUI_JSON_NUMBER)
            naui_strmap_put(tm.float_map, key_str, (float)naui_json_get_number(val, 0.0));
    }
	
    naui_json_free(&json);
}

Naui_Color naui_theme_color(const char *name)
{
    return naui_strmap_get(tm.color_map, name);
}

float naui_theme_float(const char *name)
{
    return naui_strmap_get(tm.float_map, name);
}

Naui_Vec2 naui_theme_vec2(const char *name)
{
    return naui_strmap_get(tm.vec2_map, name);
}
