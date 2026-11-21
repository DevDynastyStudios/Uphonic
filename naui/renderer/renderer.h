typedef Leaf_Color Naui_Color;

typedef struct
{
    Naui_Color color1, color2;
    float percent1, percent2;
    float angle;
}
Naui_Gradient;

typedef struct
{
    uint32_t width, height;
    float texture_area[4];
}
Naui_Image;

typedef uint8_t Naui_CornerFlags;
enum
{
    NAUI_CORNER_NONE = 0,
    NAUI_CORNER_TL = 1 << 0,
    NAUI_CORNER_TR = 1 << 1,
    NAUI_CORNER_BR = 1 << 2,
    NAUI_CORNER_BL = 1 << 3,
    NAUI_CORNER_ALL = NAUI_CORNER_TL | NAUI_CORNER_TR | NAUI_CORNER_BR | NAUI_CORNER_BL
};

typedef uint8_t Naui_SideFlags;
enum
{
    NAUI_SIDE_NONE = 0,
    NAUI_SIDE_TOP = 1 << 0,
    NAUI_SIDE_RIGHT = 1 << 1,
    NAUI_SIDE_BOTTOM = 1 << 2,
    NAUI_SIDE_LEFT = 1 << 3,
    NAUI_SIDE_ALL = NAUI_SIDE_TOP | NAUI_SIDE_RIGHT | NAUI_SIDE_BOTTOM | NAUI_SIDE_LEFT
};

typedef void *Naui_Texture;

typedef struct
{
    void *display;
    void *draw;
    void *read;
    void *context;
}
Naui_ForeignGpuContext;

typedef uint8_t Naui_TextureFormat;
enum
{
    NAUI_TEXTURE_FORMAT_RGBA8,
    NAUI_TEXTURE_FORMAT_BGRA8
};

typedef uint8_t Naui_MemoryType;
enum
{
    NAUI_MEMORY_DEVICE,
    NAUI_MEMORY_SHARED
};

#define NAUI_IMAGE_ASPECT_RATIO(image) ((float)image->width / (float)image->height)

NAUI_API void naui_fill_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float rounding, Naui_CornerFlags flags);
NAUI_API void naui_draw_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width, float rounding, Naui_CornerFlags flags, Naui_SideFlags sides);
NAUI_API void naui_fill_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float rounding, Naui_CornerFlags flags);
NAUI_API void naui_draw_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float line_width, float rounding, Naui_CornerFlags flags, Naui_SideFlags sides);

NAUI_API void naui_draw_rotated_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Vec2 pivot, float angle, Naui_Color color, float line_width, float rounding, Naui_CornerFlags corners, Naui_SideFlags sides);
NAUI_API void naui_fill_rotated_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Vec2 pivot, float angle, Naui_Color color, float rounding, Naui_CornerFlags corners);

NAUI_API void naui_draw_line(Naui_Vec2 a, Naui_Vec2 b, Naui_Color color, float line_width);
NAUI_API void naui_draw_polyline(const Naui_Vec2 *points, int point_count, Naui_Color color, float line_width);
NAUI_API void naui_draw_bezier(Naui_Vec2 p0, Naui_Vec2 p1, Naui_Vec2 p2, Naui_Vec2 p3, Naui_Color color, float line_width);
NAUI_API void naui_fill_bezier_area(Naui_Vec2 p0, Naui_Vec2 p1, Naui_Vec2 p2, Naui_Vec2 p3, float baseline_y, Naui_Color color);

NAUI_API void naui_draw_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Color tint, float rounding, Naui_CornerFlags flags);
NAUI_API Naui_Texture naui_texture_create(uint32_t width, uint32_t height, Naui_TextureFormat format, Naui_MemoryType memory);
NAUI_API void naui_texture_update(Naui_Texture texture, const void *pixels);
NAUI_API void naui_texture_destroy(Naui_Texture texture);
NAUI_API void naui_draw_texture(Naui_Texture texture, Naui_Vec2 position, Naui_Vec2 scale, Naui_Color tint);

NAUI_API void naui_draw_gradient_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient tint, float rounding, Naui_CornerFlags flags);

NAUI_API bool naui_load_fonts(const char *const *file_names, uint32_t file_count);
NAUI_API void naui_unload_fonts(void);
NAUI_API bool naui_fonts_are_loaded(void);
NAUI_API bool naui_font_has_codepoint(uint32_t codepoint);
NAUI_API void naui_draw_text(Naui_Vec2 position, const char *text, float size, Naui_Color color);
NAUI_API Naui_Vec2 naui_measure_text(const char *text, uint32_t length, float font_size);

NAUI_API void naui_set_opacity(float opacity);
NAUI_API float naui_get_opacity(void);

NAUI_API void naui_push_clip_rect(float x, float y, float width, float height);
NAUI_API void naui_pop_clip_rect(void);

NAUI_API void naui_draw_shadow(Naui_Vec2 position, Naui_Vec2 scale, float blur_radius, Naui_Color color, float rounding, Naui_CornerFlags corners);

NAUI_API void naui_fill_polygon(const Naui_Vec2 *points, int point_count, Naui_Color color);
NAUI_API Naui_Vec4 naui_current_clip_rect(void);

NAUI_API void naui_foreign_gpu_context_enter(const Naui_ForeignGpuContext *context);
NAUI_API void naui_foreign_gpu_context_leave(Naui_ForeignGpuContext *context);
