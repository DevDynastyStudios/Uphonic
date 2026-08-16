#pragma once

Leaf_ID naui_widgets_alloc_id(void);
void naui_widgets_new_frame(void);
bool naui_button(const char *text);
bool naui_toggle_button(const char *text, float font_size, Naui_Color text_color, bool *toggled);
bool naui_toggle_image_button(Naui_Image *image, Naui_Vec2 size, Naui_Color tint, bool *toggled);
bool naui_image_button(Naui_Image *image, Naui_Vec2 size, Naui_Color tint);