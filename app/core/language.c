static Naui_String s_active_font = {0};

static bool _uph_language_load_font(const char *name)
{
	if (!naui_load_font(0, name))
		return false;

	s_active_font = naui_string_from_cstr(name);
	return true;
}

void uph_language_apply(void)
{
	const Uph_GeneralSettings *general = &uph_state.settings.general;
	naui_localization_set_current(naui_string_format("%s-%s", general->language_code.data, general->region_code.data));

	const Naui_Language *language = naui_localization_get_current();
	const char *wanted = (language && language->meta.font.length > 0) ? language->meta.font.data : UPH_DEFAULT_FONT_NAME;

	if (!naui_font_is_loaded(0) || strcmp(wanted, s_active_font.data) != 0)
	{
		if (!_uph_language_load_font(wanted))
		{
			const bool default_is_active = naui_font_is_loaded(0) && strcmp(s_active_font.data, UPH_DEFAULT_FONT_NAME) == 0;
			naui_log(NAUI_LOG_WARNING, "Couldn't load font '%s' for this language%s", wanted, default_is_active ? "" : ", falling back to the default font");

			if (!default_is_active)
				_uph_language_load_font(UPH_DEFAULT_FONT_NAME);
		}
	}

	naui_localization_check_font(0);
}

static void _uph_language_apply_event(void *data)
{
	(void)data;
	uph_language_apply();
}

void uph_language_request_change(void)
{
	naui_defer(_uph_language_apply_event, NULL, 0);
}
