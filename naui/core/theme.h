NAUI_API void       naui_load_theme     (const char *file_name);

NAUI_API Naui_Color naui_theme_color    (const char *name);
NAUI_API Naui_Vec2  naui_theme_vec2     (const char *name);
NAUI_API float      naui_theme_float    (const char *name);

NAUI_API const Naui_List(Naui_Color) naui_theme_color_list(const char *name);