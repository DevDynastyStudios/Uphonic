enum
{
	UPH_CURSOR_DRAW = NAUI_CURSOR_MAX,
	UPH_CURSOR_CUT
};

typedef struct
{
	Naui_Cursor cursor;
	const char *images[NAUI_CURSOR_SCALE_COUNT];
	float hotspot_x;
	float hotspot_y;
}
Uph_CursorImage;

static const Uph_CursorImage uph__cursor_images[] = {
	{ NAUI_CURSOR_ARROW, { "uph_cursor_arrow.png", "uph_cursor_arrow_125.png", "uph_cursor_arrow_150.png", "uph_cursor_arrow_175.png", "uph_cursor_arrow_200.png", "uph_cursor_arrow_300.png" }, 5.0f, 3.0f },
	{ NAUI_CURSOR_IBEAM, { "uph_cursor_ibeam.png", "uph_cursor_ibeam_125.png", "uph_cursor_ibeam_150.png", "uph_cursor_ibeam_175.png", "uph_cursor_ibeam_200.png", "uph_cursor_ibeam_300.png" }, 16.0f, 16.0f },
	{ NAUI_CURSOR_CROSSHAIR, { "uph_cursor_crosshair.png", "uph_cursor_crosshair_125.png", "uph_cursor_crosshair_150.png", "uph_cursor_crosshair_175.png", "uph_cursor_crosshair_200.png", "uph_cursor_crosshair_300.png" }, 16.0f, 16.0f },
	{ NAUI_CURSOR_HAND, { "uph_cursor_hand.png", "uph_cursor_hand_125.png", "uph_cursor_hand_150.png", "uph_cursor_hand_175.png", "uph_cursor_hand_200.png", "uph_cursor_hand_300.png" }, 13.0f, 3.0f },
	{ NAUI_CURSOR_RESIZE_NS, { "uph_cursor_resize_ns.png", "uph_cursor_resize_ns_125.png", "uph_cursor_resize_ns_150.png", "uph_cursor_resize_ns_175.png", "uph_cursor_resize_ns_200.png", "uph_cursor_resize_ns_300.png" }, 16.0f, 16.0f },
	{ NAUI_CURSOR_RESIZE_EW, { "uph_cursor_resize_ew.png", "uph_cursor_resize_ew_125.png", "uph_cursor_resize_ew_150.png", "uph_cursor_resize_ew_175.png", "uph_cursor_resize_ew_200.png", "uph_cursor_resize_ew_300.png" }, 16.0f, 16.0f },
	{ NAUI_CURSOR_RESIZE_NESW, { "uph_cursor_resize_nesw.png", "uph_cursor_resize_nesw_125.png", "uph_cursor_resize_nesw_150.png", "uph_cursor_resize_nesw_175.png", "uph_cursor_resize_nesw_200.png", "uph_cursor_resize_nesw_300.png" }, 16.0f, 16.0f },
	{ NAUI_CURSOR_RESIZE_NWSE, { "uph_cursor_resize_nwse.png", "uph_cursor_resize_nwse_125.png", "uph_cursor_resize_nwse_150.png", "uph_cursor_resize_nwse_175.png", "uph_cursor_resize_nwse_200.png", "uph_cursor_resize_nwse_300.png" }, 16.0f, 16.0f },
	{ NAUI_CURSOR_RESIZE_ALL, { "uph_cursor_resize_all.png", "uph_cursor_resize_all_125.png", "uph_cursor_resize_all_150.png", "uph_cursor_resize_all_175.png", "uph_cursor_resize_all_200.png", "uph_cursor_resize_all_300.png" }, 16.0f, 16.0f },
	{ NAUI_CURSOR_NOT_ALLOWED, { "uph_cursor_not_allowed.png", "uph_cursor_not_allowed_125.png", "uph_cursor_not_allowed_150.png", "uph_cursor_not_allowed_175.png", "uph_cursor_not_allowed_200.png", "uph_cursor_not_allowed_300.png" }, 16.0f, 16.0f },
	{ UPH_CURSOR_DRAW, { "uph_cursor_draw.png", "uph_cursor_draw_125.png", "uph_cursor_draw_150.png", "uph_cursor_draw_175.png", "uph_cursor_draw_200.png", "uph_cursor_draw_300.png" }, 3.3f, 28.7f },
	{ UPH_CURSOR_CUT, { "uph_cursor_cut.png", "uph_cursor_cut_125.png", "uph_cursor_cut_150.png", "uph_cursor_cut_175.png", "uph_cursor_cut_200.png", "uph_cursor_cut_300.png" }, 16.0f, 14.8f },
};

void uph_ui_cursors_init(void)
{
	for (size_t i = 0; i < sizeof(uph__cursor_images) / sizeof(uph__cursor_images[0]); ++i)
	{
		const Uph_CursorImage *image = &uph__cursor_images[i];
		naui_set_cursor_image(image->cursor, image->images, (Naui_Vec2){ image->hotspot_x / 32.0f, image->hotspot_y / 32.0f });
	}
}
