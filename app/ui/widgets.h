typedef uint32_t Uph_UIMenuFlags;
enum
{
    UPH_UI_MENU_FLAGS_NONE = 0,
    UPH_UI_MENU_ITEM_DISABLED = 1 << 0,
};

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

typedef void (*Uph_DropDownCallback)(void);

void uph_ui_widgets_flush(void);

bool uph_ui_widget_hovered(const Leaf_ID id);
bool uph_ui_any_widget_hovered(void);

void uph_ui_dropdown(Uph_DropDownCallback callback, Naui_Vec2 position);
void uph_ui_close_dropdown(void);

bool uph_ui_text_button(const char *string, const Leaf_ID id);
bool uph_ui_text_toggle_button(const char *string, const Leaf_ID id, bool *enabled);

void uph_ui_menu(const char *label, const Leaf_ID id, Uph_UIMenuFlags flags, Uph_DropDownCallback dropdown);

bool uph_ui_textfield(Naui_String* value, const Leaf_ID id, Uph_UITextFieldFlags flags, const char *placeholder);

bool uph_ui_drag_float(float *value, const Leaf_ID id, float speed, float min, float max, const char *format, Uph_UIDragFlags flags);
bool uph_ui_drag_int(int32_t *value, const Leaf_ID id, float speed, int32_t min, int32_t max, const char *format, Uph_UIDragFlags flags);

bool uph_ui_slider_float(float *value, const Leaf_ID id, float min, float max, const char *format, Uph_UISliderFlags flags);
bool uph_ui_slider_int(int32_t *value, const Leaf_ID id, int32_t min, int32_t max, const char *format, Uph_UISliderFlags flags);

bool uph_ui_checkbox(bool *value, const Leaf_ID id);