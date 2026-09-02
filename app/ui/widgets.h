typedef uint32_t Uph_UITextFieldFlags;
enum
{
    UPH_UI_TEXTFIELD_FLAGS_NONE = 0,
    UPH_UI_TEXTFIELD_ALWAYS_ACTIVE = 1 << 0,
    UPH_UI_TEXTFIELD_NUMBER_ONLY = 1 << 1
};

typedef uint32_t Uph_UIDragFlags;
enum
{
    UPH_UI_DRAG_FLAGS_NONE = 0,
    UPH_UI_DRAG_CLAMPED = 1 << 0,
};

typedef uint32_t Uph_UISliderFlags;
enum
{
    UPH_UI_SLIDER_FLAGS_NONE = 0,
    UPH_UI_SLIDER_VERTICAL = 1 << 0,
};

typedef uint64_t Uph_UIMenuID;

void uph_ui_widgets_init(void);
void uph_ui_widgets_flush(void);

bool uph_ui_widget_hovered(const Leaf_ID id);
bool uph_ui_any_widget_hovered(void);

Uph_UIMenuID    uph_ui_menu         (const char *name, const Leaf_ID element_id);
Uph_UIMenuID    uph_ui_submenu      (Uph_UIMenuID parent_id, const char *name, const Leaf_ID element_id);
bool            uph_ui_menu_item    (Uph_UIMenuID menu_id, const char *name, const Leaf_ID element_id);

Uph_UIMenuID    uph_ui_context_menu (void);
void            uph_ui_open_context_menu (Uph_UIMenuID menu);

bool uph_ui_text_button(const char *string, const Leaf_ID id);
bool uph_ui_image_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint);
bool uph_ui_image_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners);

bool uph_ui_text_toggle_button(const char *string, const Leaf_ID id, bool enabled);
bool uph_ui_image_toggle_button(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, bool enabled);
bool uph_ui_image_toggle_button_ex(const Naui_Image *image, const Leaf_ID id, Naui_Vec2 size, Naui_Color tint, Naui_Color bg_color, Naui_CornerFlags corners, bool enabled);

bool uph_ui_textfield(Naui_String* value, const Leaf_ID id, Uph_UITextFieldFlags flags, const char *placeholder);

bool uph_ui_drag_float(float *value, const Leaf_ID id, float speed, float min, float max, const char *format, Uph_UIDragFlags flags);
bool uph_ui_drag_int(int32_t *value, const Leaf_ID id, float speed, int32_t min, int32_t max, const char *format, Uph_UIDragFlags flags);

bool uph_ui_slider_float(float *value, const Leaf_ID id, float min, float max, const char *format, Uph_UISliderFlags flags);
bool uph_ui_slider_int(int32_t *value, const Leaf_ID id, int32_t min, int32_t max, const char *format, Uph_UISliderFlags flags);

bool uph_ui_checkbox(bool *value, const Leaf_ID id);