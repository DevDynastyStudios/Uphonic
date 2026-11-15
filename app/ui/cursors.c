#define UPH_UI_CURSOR_SIZE 24
#define UPH_UI_CURSOR_ART_SIZE 32.0f

typedef struct
{
	const char *image;
	Naui_Vec2 hotspot;
}
Uph_CursorImage;

static const Uph_CursorImage _uph_cursor_images[UPH_CURSOR_COUNT] = {
	[UPH_CURSOR_ARROW] = { "uph_cursor_arrow", { 5.0f, 3.0f } },
	[UPH_CURSOR_IBEAM] = { "uph_cursor_ibeam", { 16.0f, 16.0f } },
	[UPH_CURSOR_CROSSHAIR] = { "uph_cursor_crosshair", { 16.0f, 16.0f } },
	[UPH_CURSOR_HAND] = { "uph_cursor_hand", { 13.0f, 3.0f } },
	[UPH_CURSOR_RESIZE_NS] = { "uph_cursor_resize_ns", { 16.0f, 16.0f } },
	[UPH_CURSOR_RESIZE_EW] = { "uph_cursor_resize_ew", { 16.0f, 16.0f } },
	[UPH_CURSOR_RESIZE_NESW] = { "uph_cursor_resize_nesw", { 16.0f, 16.0f } },
	[UPH_CURSOR_RESIZE_NWSE] = { "uph_cursor_resize_nwse", { 16.0f, 16.0f } },
	[UPH_CURSOR_RESIZE_ALL] = { "uph_cursor_resize_all", { 16.0f, 16.0f } },
	[UPH_CURSOR_NOT_ALLOWED] = { "uph_cursor_not_allowed", { 16.0f, 16.0f } },
	[UPH_CURSOR_DRAW] = { "uph_cursor_draw", { 6.1f, 25.9f } },
	[UPH_CURSOR_CUT] = { "uph_cursor_cut", { 16.0f, 10.4f } }
};

static Uph_Cursor _uph_cursor_current;

void uph_ui_set_cursor(Uph_Cursor cursor)
{
	_uph_cursor_current = cursor;
}

static void _uph_cursor_draw(Leaf_BoundingBox box, void *user_data)
{
	const Uph_CursorImage *cursor = &_uph_cursor_images[_uph_cursor_current];
	_uph_cursor_current = UPH_CURSOR_ARROW;

	const Naui_Image *image = naui_asset_image(cursor->image);
	if (!image)
		return;

	const float size = (float)NAUI_DPI(UPH_UI_CURSOR_SIZE);
	const float scale = size / UPH_UI_CURSOR_ART_SIZE;
	const Naui_Vec2 position = {
		floorf((float)naui_mouse_x() - cursor->hotspot.x * scale),
		floorf((float)naui_mouse_y() - cursor->hotspot.y * scale)
	};
	naui_draw_image(image, position, (Naui_Vec2){ size, size }, (Naui_Color){ 255, 255, 255, 255 }, 0.0f, NAUI_CORNER_NONE);
}

void uph_ui_cursor_render(void)
{
	if (!naui_mouse_inside())
	{
		_uph_cursor_current = UPH_CURSOR_ARROW;
		return;
	}

	naui_set_cursor(NAUI_CURSOR_HIDDEN);
	leaf({
		.custom_draw = _uph_cursor_draw,
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
	});
}
