#pragma once

#define UPL_PENDING_VALUE			(1u << 0)
#define UPL_PENDING_GESTURE_BEGIN	(1u << 1)
#define UPL_PENDING_GESTURE_END		(1u << 2)

#define UPL_FRAME_INTERVAL_MS		16
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
	uint32_t *pixels;
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
static void upl__window_present(Upl_Plugin *plugin);
static void upl__window_poll(Upl_Plugin *plugin);
static double upl__time(void);

static bool upl__file_dialog_begin(Upl_Plugin *plugin, const Upl_FileDialog *dialog);
static void upl__file_dialog_poll(Upl_Plugin *plugin);
static void upl__file_dialog_cancel(Upl_Plugin *plugin);
static void upl__file_dialog_finish(Upl_Plugin *plugin, const char *path);

static void upl__gui_tick(Upl_Plugin *plugin);
static void upl__gui_mark_dirty(Upl_Plugin *plugin);

static void upl__input_mouse_move(Upl_Plugin *plugin, int32_t x, int32_t y);
static void upl__input_mouse_button(Upl_Plugin *plugin, Upl_MouseButton button, bool down);
static void upl__input_mouse_wheel(Upl_Plugin *plugin, float delta);
static void upl__input_mouse_leave(Upl_Plugin *plugin);
static void upl__input_set_shift(Upl_Plugin *plugin, bool down);
