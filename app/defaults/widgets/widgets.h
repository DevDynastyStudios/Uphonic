#pragma once

void naui_widgets_reset(void);

void naui_text(const char* text);
bool naui_button(const char* label);
bool naui_toggle(const char* label, bool* value);
bool naui_dropdown(const char* label, uint32_t* select, const char** items, size_t item_count);
bool naui_slider_int(const char* label, int32_t* value, int32_t min, int32_t max);
bool naui_slider_float(const char* label, float* value, float min, float max, const char* format);
bool naui_drag_int(const char* label, int* value, int speed, int min, int max);
bool naui_drag_float(const char* label, float* value, float speed, float min, float max, const char* format);
bool naui_text_field(const char* label, const char* hint, char* buffer, size_t buffer_size);
bool naui_knob(const char* label, float* value, float min, float max, const char* format);
