NAUI_API void       naui_load_theme     (const char *file_name);

NAUI_API Naui_Color naui_theme_color    (const char *name);
NAUI_API bool       naui_theme_has      (const char *name);

NAUI_API const Naui_List(Naui_Color) naui_theme_color_list(const char *name);