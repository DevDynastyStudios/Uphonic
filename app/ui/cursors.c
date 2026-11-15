#define UPH_UI_CURSOR_SIZE 24

enum
{
	UPH_CURSOR_DRAW = NAUI_CURSOR_MAX,
	UPH_CURSOR_CUT
};

static void _uph_cursor_image(Naui_Cursor cursor, const char *image, float hotspot_x, float hotspot_y)
{
	naui_set_cursor_image(cursor, image, (Naui_Vec2){ hotspot_x / 32.0f, hotspot_y / 32.0f }, UPH_UI_CURSOR_SIZE);
}

void uph_ui_cursors_init(void)
{
	_uph_cursor_image(NAUI_CURSOR_ARROW, "uph_cursor_arrow", 5.0f, 3.0f);
	_uph_cursor_image(NAUI_CURSOR_IBEAM, "uph_cursor_ibeam", 16.0f, 16.0f);
	_uph_cursor_image(NAUI_CURSOR_CROSSHAIR, "uph_cursor_crosshair", 16.0f, 16.0f);
	_uph_cursor_image(NAUI_CURSOR_HAND, "uph_cursor_hand", 13.0f, 3.0f);
	_uph_cursor_image(NAUI_CURSOR_RESIZE_NS, "uph_cursor_resize_ns", 16.0f, 16.0f);
	_uph_cursor_image(NAUI_CURSOR_RESIZE_EW, "uph_cursor_resize_ew", 16.0f, 16.0f);
	_uph_cursor_image(NAUI_CURSOR_RESIZE_NESW, "uph_cursor_resize_nesw", 16.0f, 16.0f);
	_uph_cursor_image(NAUI_CURSOR_RESIZE_NWSE, "uph_cursor_resize_nwse", 16.0f, 16.0f);
	_uph_cursor_image(NAUI_CURSOR_RESIZE_ALL, "uph_cursor_resize_all", 16.0f, 16.0f);
	_uph_cursor_image(NAUI_CURSOR_NOT_ALLOWED, "uph_cursor_not_allowed", 16.0f, 16.0f);
	_uph_cursor_image(UPH_CURSOR_DRAW, "uph_cursor_draw", 6.1f, 25.9f);
	_uph_cursor_image(UPH_CURSOR_CUT, "uph_cursor_cut", 16.0f, 10.4f);
}
