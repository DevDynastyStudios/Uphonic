#pragma once

#define GUI_LOGICAL_WIDTH 400
#define GUI_LOGICAL_HEIGHT 346
#define GUI_BASE_PIXEL_SCALE 2
#define GUI_MAX_PIXEL_SCALE 6
#define GUI_SCALE_ROUNDING 0.3

#define GUI_NO_COLOR 0xFF000000u

typedef struct GuiPen
{
	float x;
	float y;
	float scale;
} GuiPen;

GuiView* gui_view_create(Plugin* plugin);
void gui_view_destroy(GuiView* view);

void gui_view_frame(GuiView* view);

Leaf_Color gui_color(uint32_t color);

void gui_draw_fill_rect(const GuiPen* pen, int x, int y, int width, int height, uint32_t color);
void gui_draw_pixel(const GuiPen* pen, int x, int y, uint32_t color);
void gui_draw_rect(const GuiPen* pen, int x, int y, int width, int height, uint32_t color);
void gui_draw_hline(const GuiPen* pen, int x, int y, int length, uint32_t color);
void gui_draw_vline(const GuiPen* pen, int x, int y, int length, uint32_t color);
void gui_draw_text(const GuiPen* pen, int x, int y, const char* text, uint32_t color, int scale);
void gui_draw_text_centered(const GuiPen* pen, int center_x, int y, const char* text, uint32_t color, int scale);
int gui_draw_text_width(const char* text, int scale);
