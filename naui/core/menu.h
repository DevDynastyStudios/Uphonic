typedef struct
{
	// Shortcut here
	Leaf_ID _id;
	const char* label;
	bool enabled;
} Naui_MenuItem;

typedef struct
{
	Leaf_ID _id;
	Naui_List(Naui_MenuItem) _items;
	const char* label;
} Naui_Menu;

Naui_Menu* naui_menu_create(const char* label);
Naui_MenuItem* naui_menu_item_create(Naui_Menu* menu, const char* label);
bool naui_menu(Naui_Menu* menu);
bool naui_menu_item(Naui_MenuItem* menu);
bool naui_menu_item_ex(Naui_Menu*, const char* label, bool enabled, const char* shortcut_label);

bool _naui_menu_render(void);