typedef uint32_t Uph_UITextFieldFlags;
enum
{
    UPH_UI_TEXTFIELD_FLAG_NONE = 0,
    UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE = 1 << 0,
    UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY = 1 << 1,
    UPH_UI_TEXTFIELD_FLAG_SEARCH = 1 << 2,
    UPH_UI_TEXTFIELD_FLAG_INLINE = 1 << 3,
    UPH_UI_TEXTFIELD_FLAG_TINTED = 1 << 4
};

typedef uint64_t Uph_UIMenuID;

bool uph_ui_widget_hovered(
	const Leaf_ID id
);

bool uph_ui_any_widget_hovered(void);
bool uph_ui_any_menu_open(void);

Uph_UIMenuID uph_ui_menu(
	const char *name,
	const Leaf_ID element_id
);

Uph_UIMenuID uph_ui_submenu(
	Uph_UIMenuID parent_id,
	const char *name,
	const Leaf_ID element_id
);

bool uph_ui_menu_item(
	Uph_UIMenuID menu_id,
	const char *name,
	const Leaf_ID element_id
);

bool uph_ui_menu_keybind_item(
	Uph_UIMenuID menu_id,
	const char *name,
	const Leaf_ID element_id,
	Naui_Keybind chord
);

bool uph_ui_textfield(
	Naui_String* value,
	const Leaf_ID id,
	Uph_UITextFieldFlags flags,
	const char *placeholder
);
