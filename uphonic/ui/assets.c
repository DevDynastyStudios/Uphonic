void uph_theme_load(const char *name)
{
	Uph_Colors *colors = &uph_state.colors;
	naui_load_theme(name);

#define X(field, key) colors->field = naui_theme_color(key);
	UPH_THEME_COLORS(X)
#undef X
	colors->track_palette = naui_theme_color_list("uph_track_palette");
}

void uph_icons_load(void)
{
	Uph_Icons *icons = &uph_state.icons;
#define X(field, name) icons->field = naui_asset_image(name);
	UPH_ICONS(X)
#undef X
}
