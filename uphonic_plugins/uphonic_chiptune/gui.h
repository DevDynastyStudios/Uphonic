#pragma once

// The picture is drawn at a small fixed size and enlarged by a whole number
// when it reaches the screen, which keeps the pixel-art edges sharp.
#define GUI_LOGICAL_WIDTH 400
#define GUI_LOGICAL_HEIGHT 346
#define GUI_BASE_PIXEL_SCALE 2
#define GUI_MAX_PIXEL_SCALE 6
#define GUI_SCALE_ROUNDING 0.3

// A canvas pixel nobody painted. Real colours are 0xRRGGBB, so the top byte is free.
#define GUI_TRANSPARENT 0xFF000000u

typedef struct GuiCanvas
{
	uint32_t* pixels;
	int width;
	int height;
} GuiCanvas;

GuiView* gui_view_create(Plugin* plugin);
void gui_view_destroy(GuiView* view);

// Declares the whole editor for one frame. Call it from the upl ui callback,
// where Leaf is mid-frame: it also reads the mouse and turns it into parameter
// edits and keyboard notes.
void gui_view_frame(GuiView* view);

void gui_draw_pixel(GuiCanvas* canvas, int x, int y, uint32_t color);
void gui_draw_fill_rect(GuiCanvas* canvas, int x, int y, int width, int height, uint32_t color);
void gui_draw_rect(GuiCanvas* canvas, int x, int y, int width, int height, uint32_t color);
void gui_draw_hline(GuiCanvas* canvas, int x, int y, int length, uint32_t color);
void gui_draw_vline(GuiCanvas* canvas, int x, int y, int length, uint32_t color);
void gui_draw_line(GuiCanvas* canvas, int x0, int y0, int x1, int y1, uint32_t color);
void gui_draw_text(GuiCanvas* canvas, int x, int y, const char* text, uint32_t color, int scale);
void gui_draw_text_centered(GuiCanvas* canvas, int center_x, int y, const char* text, uint32_t color, int scale);
int gui_draw_text_width(const char* text, int scale);
