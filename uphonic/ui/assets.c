static Naui_String uph__active_theme;

static void uph__theme_fallback()
{
    naui_theme_set_color("uph_ui_text_color", naui_color_from_hex("#FFFFFF"));
    naui_theme_set_color("uph_ui_frame_bg_color", naui_color_from_hex("#2B2B2B"));
    naui_theme_set_color("uph_ui_frame_border", naui_color_from_hex("#404040"));
}

void uph_theme_load(const char *name)
{
	Uph_Colors *colors = &uph_state.colors;
	const Naui_String requested = naui_string_from_cstr(name);
	Naui_String active = { 0 };
	bool applied = false;

	if (requested.length > 0 && naui_load_theme(requested.data))
	{
		active = requested;
		applied = true;
	}
	else if (uph__active_theme.length > 0)
	{
		active = uph__active_theme;
	}
	else if (strcmp(requested.data, UPH_DEFAULT_THEME) != 0 && naui_load_theme(UPH_DEFAULT_THEME))
	{
		active = naui_string_from_cstr(UPH_DEFAULT_THEME);
		applied = true;
	}

	if (active.length == 0)
	{
		naui_log(NAUI_LOG_ERROR, "No usable theme found (requested '%s')", requested.data);
		uph__theme_fallback();
		return;
	}

	if (applied)
	{
#define X(field, key) colors->field = naui_theme_color(key);
		UPH_THEME_COLORS(X)
#undef X
		colors->track_palette = naui_theme_color_list("uph_track_palette");
		uph__active_theme = active;
	}

	if (strcmp(requested.data, active.data) != 0)
		naui_log(NAUI_LOG_WARNING, "Theme '%s' unavailable, using '%s'", requested.data, active.data);

	if (strcmp(uph_state.settings.general.theme.data, active.data) != 0)
		uph_state.settings.general.theme = active;
}

void uph_icons_load(void)
{
	Uph_Icons *icons = &uph_state.icons;
#define X(field, name) icons->field = naui_asset_image(name);
	UPH_ICONS(X)
#undef X
}