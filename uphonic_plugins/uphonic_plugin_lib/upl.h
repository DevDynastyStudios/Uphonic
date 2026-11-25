#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "leaf/leaf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UPL_MAX_CHANNELS 2
#define UPL_DEFAULT_FONT 0

#define UPL_DPI(x) ((x) * upl_scale())

typedef struct Upl_Plugin Upl_Plugin;
typedef struct Upl_Image Upl_Image;

typedef uint8_t Upl_ParamFlags;
enum
{
	UPL_PARAM_STEPPED = 1 << 0,
	UPL_PARAM_BYPASS = 1 << 1,
	UPL_PARAM_HIDDEN = 1 << 2
};

typedef struct
{
	const char *name;
	const char *unit;
	double min;
	double max;
	double default_value;
	Upl_ParamFlags flags;
}
Upl_ParamInfo;

typedef uint8_t Upl_NoteEventType;
enum
{
	UPL_NOTE_ON,
	UPL_NOTE_OFF,
	UPL_NOTE_CHOKE
};

typedef struct
{
	Upl_NoteEventType type;
	int16_t key;
	int16_t channel;
	int32_t note_id;
	double velocity;
}
Upl_NoteEvent;

typedef struct
{
	const float *const *inputs;
	float *const *outputs;
	uint32_t input_channels;
	uint32_t output_channels;
	uint32_t frame_count;
	double sample_rate;
}
Upl_AudioBlock;

typedef uint8_t Upl_MouseButton;
enum
{
	UPL_MOUSE_LEFT,
	UPL_MOUSE_RIGHT,
	UPL_MOUSE_MIDDLE,
	UPL_MOUSE_BUTTON_COUNT
};

typedef struct
{
	const char *id;
	const char *name;
	const char *vendor;
	const char *url;
	const char *version;
	const char *description;
	const char *const *features;

	const Upl_ParamInfo *params;
	uint32_t param_count;

	uint32_t input_channels;
	uint32_t output_channels;
	bool note_input;

	uint32_t gui_width;
	uint32_t gui_height;

	void *(*create)(Upl_Plugin *plugin);
	void (*destroy)(Upl_Plugin *plugin);
	void (*activate)(Upl_Plugin *plugin, double sample_rate, uint32_t max_frame_count);
	void (*deactivate)(Upl_Plugin *plugin);
	void (*process)(Upl_Plugin *plugin, const Upl_AudioBlock *block);
	void (*note)(Upl_Plugin *plugin, const Upl_NoteEvent *event);
	void (*ui)(Upl_Plugin *plugin);
}
Upl_Descriptor;

extern const Upl_Descriptor upl_descriptor;

typedef struct
{
	Leaf_Color background;
	Leaf_Color panel;
	Leaf_Color track;
	Leaf_Color accent;
	Leaf_Color accent_hover;
	Leaf_Color thumb;
	Leaf_Color text;
	Leaf_Color text_dim;
	int32_t font_size;
	int32_t rounding;
}
Upl_Theme;

void *upl_user_data(Upl_Plugin *plugin);

double upl_param(Upl_Plugin *plugin, uint32_t index);
double upl_param_normalized(Upl_Plugin *plugin, uint32_t index);
void upl_param_set(Upl_Plugin *plugin, uint32_t index, double value);
void upl_param_set_normalized(Upl_Plugin *plugin, uint32_t index, double normalized);
void upl_param_begin_gesture(Upl_Plugin *plugin, uint32_t index);
void upl_param_end_gesture(Upl_Plugin *plugin, uint32_t index);
void upl_param_text(Upl_Plugin *plugin, uint32_t index, double value, char *buffer, uint32_t size);

Upl_Plugin *upl_current(void);
float upl_scale(void);
void upl_redraw(void);
Upl_Theme *upl_theme(void);

int32_t upl_mouse_x(void);
int32_t upl_mouse_y(void);
bool upl_mouse_down(Upl_MouseButton button);
bool upl_mouse_pressed(Upl_MouseButton button);
bool upl_mouse_released(Upl_MouseButton button);
bool upl_mouse_double_clicked(Upl_MouseButton button);
float upl_mouse_wheel(void);
bool upl_shift_down(void);

uint32_t upl_font_load(const void *ttf_data, size_t size);

Upl_Image *upl_image_create(const uint8_t *rgba, int32_t width, int32_t height);
void upl_image_destroy(Upl_Image *image);

void upl_fill_rect(float x, float y, float width, float height, Leaf_Color color, float rounding);
void upl_stroke_rect(float x, float y, float width, float height, Leaf_Color color, float line_width, float rounding);
void upl_draw_line(float x0, float y0, float x1, float y1, Leaf_Color color, float width);
void upl_fill_circle(float cx, float cy, float radius, Leaf_Color color);
void upl_stroke_arc(float cx, float cy, float radius, float start_angle, float end_angle, Leaf_Color color, float width);
void upl_draw_text(float x, float y, const char *text, int32_t font_size, uint32_t font_id, Leaf_Color color);
void upl_draw_image(Upl_Image *image, float x, float y, float width, float height, Leaf_Color tint, float rounding);
Leaf_Dimensions upl_measure_text(const char *text, uint32_t length, int32_t font_size, uint32_t font_id);

bool upl_knob(uint32_t param, int32_t size);
bool upl_slider(uint32_t param, int32_t width, int32_t height);
bool upl_toggle(uint32_t param, int32_t width, int32_t height);
void upl_param_label(uint32_t param, Leaf_Color color, int32_t font_size);

#ifdef __cplusplus
}
#endif
