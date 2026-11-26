#pragma once

#define UPL_PENDING_VALUE			(1u << 0)
#define UPL_PENDING_GESTURE_BEGIN	(1u << 1)
#define UPL_PENDING_GESTURE_END		(1u << 2)

#define UPL_FRAME_INTERVAL_MS		16
#define UPL_ATLAS_SIZE				512
#define UPL_DOUBLE_CLICK_TIME		0.35
#define UPL_DOUBLE_CLICK_DISTANCE	4

typedef struct
{
	int32_t x;
	int32_t y;
	bool inside;
	bool shift;
	float wheel;
	bool down[UPL_MOUSE_BUTTON_COUNT];
	bool pressed[UPL_MOUSE_BUTTON_COUNT];
	bool released[UPL_MOUSE_BUTTON_COUNT];
	bool double_clicked[UPL_MOUSE_BUTTON_COUNT];
	double last_click_time[UPL_MOUSE_BUTTON_COUNT];
	int32_t last_click_x;
	int32_t last_click_y;
}
Upl_Input;

typedef struct
{
	int32_t param;
	float start_y;
	double start_value;
}
Upl_WidgetDrag;

typedef struct
{
	void *window;
	Leaf_Context *leaf;
	int32_t width;
	int32_t height;
	float scale;
	uint32_t dirty_frames;
	clap_id timer_id;
	bool created;
	bool visible;
	bool timer_registered;
	Upl_Input input;
	Upl_WidgetDrag drag;
}
Upl_Gui;

typedef struct
{
	Upl_FileDialogCallback callback;
	void *user_data;
	void *platform;
	bool pending;
}
Upl_FileDialogState;

typedef struct
{
	int32_t x0;
	int32_t y0;
	int32_t x1;
	int32_t y1;
}
Upl_Clip;

typedef struct
{
	uint8_t pixels[UPL_ATLAS_SIZE * UPL_ATLAS_SIZE];
	int32_t cursor_x;
	int32_t cursor_y;
	int32_t row_height;
	uint32_t generation;
}
Upl_Atlas;

typedef struct
{
	void *native;
	uint32_t font_id;
	int32_t size;
	int32_t codepoint;
	int32_t atlas_x;
	int32_t atlas_y;
	int32_t width;
	int32_t height;
	int32_t x_offset;
	int32_t y_offset;
	float advance;
	bool used;
}
Upl_Glyph;

struct Upl_Image
{
	uint8_t *pixels;
	void *native;
	uint64_t serial;
	uint32_t native_tint;
	int32_t width;
	int32_t height;
};

struct Upl_Plugin
{
	clap_plugin_t clap;
	const clap_host_t *host;
	const clap_host_params_t *host_params;
	const clap_host_timer_support_t *host_timer;
	const clap_host_gui_t *host_gui;
	void *user_data;
	_Atomic double *values;
	atomic_uint *pending;
	atomic_bool ui_dirty;
	double sample_rate;
	bool active;
	Upl_Gui gui;
	Upl_FileDialogState file_dialog;
};

static bool upl__window_create(Upl_Plugin *plugin, const clap_window_t *parent);
static void upl__window_destroy(Upl_Plugin *plugin);
static void upl__window_resize(Upl_Plugin *plugin);
static void upl__window_set_visible(Upl_Plugin *plugin, bool visible);
static void upl__window_redraw(Upl_Plugin *plugin);
static void upl__window_poll(Upl_Plugin *plugin);
static double upl__time(void);

static bool upl__file_dialog_begin(Upl_Plugin *plugin, const Upl_FileDialog *dialog);
static void upl__file_dialog_poll(Upl_Plugin *plugin);
static void upl__file_dialog_cancel(Upl_Plugin *plugin);
static void upl__file_dialog_finish(Upl_Plugin *plugin, const char *path);

static void upl__gfx_clear(Leaf_Color color);
static void upl__gfx_fill_rect(float x, float y, float width, float height, const float radii[4], Leaf_Color color);
static void upl__gfx_stroke_rect(float x, float y, float width, float height, const float radii[4], float line_width, Leaf_Color color);
static void upl__gfx_shadow(float x, float y, float width, float height, const float radii[4], float blur, Leaf_Color color);
static void upl__gfx_line(float x0, float y0, float x1, float y1, float width, Leaf_Color color);
static void upl__gfx_fill_circle(float cx, float cy, float radius, Leaf_Color color);
static void upl__gfx_stroke_arc(float cx, float cy, float radius, float start_angle, float sweep, float width, Leaf_Color color);
static void upl__gfx_glyph(const Upl_Atlas *atlas, Upl_Glyph *glyph, float x, float y, Leaf_Color color);
static void upl__gfx_image(Upl_Image *image, float x, float y, float width, float height, const float radii[4], Leaf_Color tint);
static void upl__gfx_set_clip(const Upl_Clip *clip);
static void upl__gfx_atlas_reset(void);
static void upl__gfx_glyph_release(Upl_Glyph *glyph);
static void upl__gfx_image_release(Upl_Image *image);

static void upl__shadow_layers(float x, float y, float width, float height, const float radii[4], float blur, Leaf_Color color);
static uint8_t *upl__image_premultiply(const Upl_Image *image, Leaf_Color tint, bool bgra);
static uint32_t upl__color_key(Leaf_Color color);

static void upl__gui_tick(Upl_Plugin *plugin);
static void upl__gui_draw(Upl_Plugin *plugin);
static void upl__gui_update_size(Upl_Plugin *plugin);
static void upl__gui_mark_dirty(Upl_Plugin *plugin);

static void upl__input_mouse_move(Upl_Plugin *plugin, int32_t x, int32_t y);
static void upl__input_mouse_button(Upl_Plugin *plugin, Upl_MouseButton button, bool down);
static void upl__input_mouse_wheel(Upl_Plugin *plugin, float delta);
static void upl__input_mouse_leave(Upl_Plugin *plugin);
static void upl__input_set_shift(Upl_Plugin *plugin, bool down);
