typedef uint32_t Uph_UIMenuFlags;
enum
{
    UPH_UI_MENU_FLAGS_NONE     = 0,
    UPH_UI_MENU_ITEM_DISABLED  = 1 << 0,
};

typedef void (*Uph_DropDownCallback)(void);

void uph_ui_widgets_flush(void);
bool uph_ui_any_widget_hovered(void);
void uph_ui_on_char_event(uint32_t codepoint);

void uph_ui_dropdown(Uph_DropDownCallback callback, Naui_Vec2 position);
void uph_ui_close_dropdown(void);

bool uph_ui_text_button(const char *string, const Leaf_ID id);
bool uph_ui_text_toggle_button(const char *string, const Leaf_ID id, bool *enabled);

void uph_ui_menu(const char *label, const Leaf_ID id, Uph_UIMenuFlags flags, Uph_DropDownCallback dropdown);

void uph_ui_textfield(Naui_String* value, const Leaf_ID id, const char *placeholder);