enum
{
	UPH_CURSOR_DRAW = NAUI_CURSOR_MAX,
	UPH_CURSOR_CUT
};

static void uph__cursor_image(Naui_Cursor cursor, const char *image, float hotspot_x, float hotspot_y)
{
	naui_set_cursor_image(cursor, image, (Naui_Vec2){ hotspot_x / 32.0f, hotspot_y / 32.0f });
}

void uph_ui_cursors_init(void)
{
	uph__cursor_image(NAUI_CURSOR_ARROW, "uph_cursor_arrow.png", 5.0f, 3.0f);
	uph__cursor_image(NAUI_CURSOR_IBEAM, "uph_cursor_ibeam.png", 16.0f, 16.0f);
	uph__cursor_image(NAUI_CURSOR_CROSSHAIR, "uph_cursor_crosshair.png", 16.0f, 16.0f);
	uph__cursor_image(NAUI_CURSOR_HAND, "uph_cursor_hand.png", 13.0f, 3.0f);
	uph__cursor_image(NAUI_CURSOR_RESIZE_NS, "uph_cursor_resize_ns.png", 16.0f, 16.0f);
	uph__cursor_image(NAUI_CURSOR_RESIZE_EW, "uph_cursor_resize_ew.png", 16.0f, 16.0f);
	uph__cursor_image(NAUI_CURSOR_RESIZE_NESW, "uph_cursor_resize_nesw.png", 16.0f, 16.0f);
	uph__cursor_image(NAUI_CURSOR_RESIZE_NWSE, "uph_cursor_resize_nwse.png", 16.0f, 16.0f);
	uph__cursor_image(NAUI_CURSOR_RESIZE_ALL, "uph_cursor_resize_all.png", 16.0f, 16.0f);
	uph__cursor_image(NAUI_CURSOR_NOT_ALLOWED, "uph_cursor_not_allowed.png", 16.0f, 16.0f);
	uph__cursor_image(UPH_CURSOR_DRAW, "uph_cursor_draw.png", 3.3f, 28.7f);
	uph__cursor_image(UPH_CURSOR_CUT, "uph_cursor_cut.png", 16.0f, 14.8f);
}
