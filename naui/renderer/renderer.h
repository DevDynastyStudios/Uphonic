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

#define NAUI_IMAGE_ASPECT_RATIO(image) ((float)image->width / (float)image->height)

NAUI_API void naui_fill_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float rounding, Naui_CornerFlags flags);
NAUI_API void naui_draw_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width, float rounding, Naui_CornerFlags flags, Naui_SideFlags sides);
NAUI_API void naui_fill_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float rounding, Naui_CornerFlags flags);
NAUI_API void naui_draw_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float line_width, float rounding, Naui_CornerFlags flags, Naui_SideFlags sides);

NAUI_API void naui_draw_line(Naui_Vec2 a, Naui_Vec2 b, Naui_Color color, float line_width);

NAUI_API void naui_draw_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Color tint, float rounding, Naui_CornerFlags flags);
NAUI_API void naui_draw_gradient_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient tint, float rounding, Naui_CornerFlags flags);

NAUI_API void naui_load_font(uint8_t index, const char *file_path);
NAUI_API void naui_unload_font(uint8_t index);
NAUI_API void naui_draw_text(Naui_Vec2 position, const char *text, float size, uint8_t font_index, Naui_Color color);
NAUI_API Naui_Vec2 naui_measure_text(const char *text, uint32_t length, float font_size, uint8_t font_index);

NAUI_API void naui_push_clip_rect(float x, float y, float width, float height);
NAUI_API void naui_pop_clip_rect(void);

NAUI_API void naui_draw_shadow(Naui_Vec2 position, Naui_Vec2 scale, float blur_radius, Naui_Color color, float rounding, Naui_CornerFlags corners);

NAUI_API void naui_fill_polygon(const Naui_Vec2 *points, int point_count, Naui_Color color);
NAUI_API Naui_Vec4 naui_current_clip_rect(void);
