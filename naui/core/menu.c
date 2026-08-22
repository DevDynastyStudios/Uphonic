static Naui_List(Naui_Menu) s_menus = NULL;
static int32_t menu_counter;
static int32_t item_counter;
static bool menu_mode;

Naui_Menu* naui_menu_create(const char* label)
{
	Naui_Menu menu = (Naui_Menu){
		.label = label,
		._id = leaf_id_indexed("Menu", menu_counter++)
	};

	naui_list_push(s_menus, menu);
	return &s_menus[naui_list_len(s_menus) - 1];
}

Naui_MenuItem* naui_menu_item_create(Naui_Menu* menu, const char* label)
{
	Naui_MenuItem menu_item = (Naui_MenuItem){
		.label = label,
		.enabled = false
	};

	naui_list_push(menu->_items, menu_item);
	return &s_menus->_items[naui_list_len(s_menus->_items) - 1];
}

bool naui_menu(Naui_Menu* menu)
{
	return menu_mode && leaf_hovered(menu->_id);
}

bool naui_menu_item(Naui_MenuItem* menu_item)
{
	return leaf_hovered(menu_item->_id) && naui_mouse_clicked(NAUI_MOUSE_LEFT);
}

bool naui_menu_item_ex(Naui_Menu* menu, const char* label, bool enabled, const char* shortcut_label)
{
	return false;	// Add Later
}

static void _naui_menu_render_dropdown(Naui_Menu* menu)
{
	leaf({
		.size = { LEAF_SIZE_FIXED(32), LEAF_SIZE_FIXED(16) },
		.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
    	.floating = {
        	.parent_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_BOTTOM },
        	.self_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP }
    	},
	})
	{
		size_t item_count = naui_list_len(menu->_items);
		for(size_t i = 0; i < item_count; i++)
		{

			leaf({
				.color = leaf_rgb(255, 255, 255),
			}){
				leaf_text(menu->_items[i].label, {
					.font_size = LEAF_SIZE_GROW
				});
			}
		}
	}
}

bool _naui_menu_render(void)
{
	bool hovered_menu = false;

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.padding = { NAUI_DPI(25), NAUI_DPI(25), NAUI_DPI(4), NAUI_DPI(4)}
	})
	{
		for (size_t i = 0; i < naui_list_len(s_menus); i++)
		{
			Naui_Menu* menu = &s_menus[i];
			Leaf_ID id = menu->_id;
			bool hovered = leaf_hovered(id);
			hovered_menu |= hovered;

			leaf({
				.id = id,
				.padding = LEAF_PADDING_AXES(NAUI_DPI(8), NAUI_DPI(4)),
				.color = hovered ? naui_theme_color(NAUI_PANEL_BUTTON_HOVERED_BG_COLOR_TAG) : LEAF_COLOR_TRANSPARENT,
				.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(4), LEAF_CORNER_ALL)
			})
			{
				leaf_text(menu->label, {
					.font_size = NAUI_DPI(naui_theme_float(NAUI_PANEL_FONT_SIZE_TAG)),
					.color = naui_theme_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG),
				});

				if (menu_mode)
					_naui_menu_render_dropdown(menu);
			}

			
		}

		if(naui_mouse_clicked(NAUI_MOUSE_LEFT))
			menu_mode = hovered_menu ? !menu_mode : false;
	}

	naui_list_clear(s_menus);
	menu_counter = 0;
	item_counter = 0;
	return hovered_menu;
}