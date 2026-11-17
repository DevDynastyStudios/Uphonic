static const char *const uph__language_fonts[] = { "MYRIADPRO-REGULAR", "NotoSansJP-Regular" };

void uph_language_apply(void)
{
	const Uph_GeneralSettings *general = &uph_state.settings.general;
	naui_localization_set_current(naui_string_format("%s-%s", general->language_code.data, general->region_code.data));

	if (!naui_fonts_are_loaded())
		naui_load_fonts(uph__language_fonts, sizeof(uph__language_fonts) / sizeof(uph__language_fonts[0]));

	naui_localization_check_font();
}

static void uph__language_apply_event(void *data)
{
	(void)data;
	uph_language_apply();
}

void uph_language_request_change(void)
{
	naui_defer(uph__language_apply_event, NULL, 0);
}
