typedef struct
{
	bool disabled;
	bool frame_blocked;
	bool capture_pending;
	bool capturing;
}
Naui_KeybindState;
static Naui_KeybindState _naui_keybind_state;

static const struct { Naui_KeyMods mod; const char *name; } _naui_mod_names[] =
{
	{ NAUI_MOD_CTRL,	"Ctrl" },
	{ NAUI_MOD_SUPER,	NAUI_KEY_SUPER_NAME },
	{ NAUI_MOD_ALT,		NAUI_KEY_ALT_NAME },
	{ NAUI_MOD_SHIFT,	"Shift" }
};

static const struct { Naui_KeyMods mod; const char *name; } _naui_mod_aliases[] =
{
	{ NAUI_MOD_CTRL,	"Control" },
	{ NAUI_MOD_SUPER,	"Super" },
	{ NAUI_MOD_SUPER,	"Cmd" },
	{ NAUI_MOD_SUPER,	"Command" },
	{ NAUI_MOD_SUPER,	"Win" },
	{ NAUI_MOD_SUPER,	"Meta" },
	{ NAUI_MOD_ALT,		"Alt" },
	{ NAUI_MOD_ALT,		"Option" },
	{ NAUI_MOD_PRIMARY,	"Primary" }
};

static Naui_KeyMods _naui_key_mods(void)
{
	Naui_KeyMods mods = NAUI_MOD_NONE;

	if (naui_key_down(NAUI_KEY_CONTROL) || naui_key_down(NAUI_KEY_LCONTROL) || naui_key_down(NAUI_KEY_RCONTROL))
		mods |= NAUI_MOD_CTRL;
	if (naui_key_down(NAUI_KEY_SHIFT) || naui_key_down(NAUI_KEY_LSHIFT) || naui_key_down(NAUI_KEY_RSHIFT))
		mods |= NAUI_MOD_SHIFT;
	if (naui_key_down(NAUI_KEY_ALT) || naui_key_down(NAUI_KEY_LALT) || naui_key_down(NAUI_KEY_RALT))
		mods |= NAUI_MOD_ALT;
	if (naui_key_down(NAUI_KEY_LSUPER) || naui_key_down(NAUI_KEY_RSUPER))
		mods |= NAUI_MOD_SUPER;

	return mods;
}

static Naui_KeyMods _naui_key_own_mod(Naui_Key key)
{
	switch (key)
	{
		case NAUI_KEY_CONTROL: case NAUI_KEY_LCONTROL: case NAUI_KEY_RCONTROL: return NAUI_MOD_CTRL;
		case NAUI_KEY_SHIFT: case NAUI_KEY_LSHIFT: case NAUI_KEY_RSHIFT: return NAUI_MOD_SHIFT;
		case NAUI_KEY_ALT: case NAUI_KEY_LALT: case NAUI_KEY_RALT: return NAUI_MOD_ALT;
		case NAUI_KEY_LSUPER: case NAUI_KEY_RSUPER: return NAUI_MOD_SUPER;
		default: return NAUI_MOD_NONE;
	}
}

static bool _naui_key_is_modifier(Naui_Key key)
{
	return _naui_key_own_mod(key) != NAUI_MOD_NONE;
}

static bool _naui_keybind_active(Naui_Keybind keybind)
{
	if (!naui_keybind_valid(keybind))
		return false;

	const Naui_KeybindState *state = &_naui_keybind_state;
	if (state->disabled || state->frame_blocked || state->capturing || state->capture_pending)
		return false;

	const Naui_KeyMods own = _naui_key_own_mod(keybind.key);
	return (_naui_key_mods() & ~own) == (keybind.mods & ~own);
}

bool naui_keybind_down(Naui_Keybind keybind)
{
	return _naui_keybind_active(keybind) && naui_key_down(keybind.key);
}

bool naui_keybind_pressed(Naui_Keybind keybind)
{
	return _naui_keybind_active(keybind) && naui_key_pressed(keybind.key);
}

bool naui_keybind_pressed_repeat(Naui_Keybind keybind)
{
	return _naui_keybind_active(keybind) && naui_key_pressed_repeat(keybind.key);
}

bool naui_keybind_released(Naui_Keybind keybind)
{
	return _naui_keybind_active(keybind) && naui_key_released(keybind.key);
}

void naui_keybind_set_enabled(bool enabled)
{
	_naui_keybind_state.disabled = !enabled;
}

void naui_keybind_capture_begin(void)
{
	_naui_keybind_state.capture_pending = true;
	_naui_keybind_state.capturing = false;
}

void naui_keybind_capture_cancel(void)
{
	if (_naui_keybind_state.capture_pending || _naui_keybind_state.capturing)
		_naui_keybind_state.frame_blocked = true;

	_naui_keybind_state.capture_pending = false;
	_naui_keybind_state.capturing = false;
}

bool naui_keybind_capturing(void)
{
	return _naui_keybind_state.capture_pending || _naui_keybind_state.capturing;
}

bool naui_keybind_capture(Naui_Keybind *out)
{
	if (!_naui_keybind_state.capturing)
		return false;

	for (uint32_t key = 1; key < NAUI_KEY_MAX; key++)
	{
		if (_naui_key_is_modifier((Naui_Key)key) || !naui_key_pressed((Naui_Key)key) || !naui_key_name((Naui_Key)key))
			continue;

		if (out)
			*out = NAUI_KEYBIND(_naui_key_mods(), key);

		_naui_keybind_state.capturing = false;
		_naui_keybind_state.frame_blocked = true;
		return true;
	}

	return false;
}

static bool _naui_keybind_name_equal(const char *a, size_t a_length, const char *b)
{
	const size_t b_length = strlen(b);
	if (a_length != b_length)
		return false;

	for (size_t i = 0; i < a_length; i++)
	{
		if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
			return false;
	}

	return true;
}

static Naui_Key _naui_key_from_name(const char *name, size_t length)
{
	if (!name || length == 0)
		return 0;

	for (uint32_t key = 1; key < NAUI_KEY_MAX; key++)
	{
		const char *key_name = naui_key_name((Naui_Key)key);
		if (key_name && _naui_keybind_name_equal(name, length, key_name))
			return (Naui_Key)key;
	}

	if (_naui_keybind_name_equal(name, length, "Alt") || _naui_keybind_name_equal(name, length, "Option"))
		return NAUI_KEY_ALT;
	if (_naui_keybind_name_equal(name, length, "Esc"))
		return NAUI_KEY_ESCAPE;
	if (_naui_keybind_name_equal(name, length, "Del"))
		return NAUI_KEY_DELETE;
	if (_naui_keybind_name_equal(name, length, "Return"))
		return NAUI_KEY_ENTER;
	if (_naui_keybind_name_equal(name, length, "Plus"))
		return NAUI_KEY_EQUAL;

	return 0;
}

Naui_String naui_keybind_to_string(Naui_Keybind keybind)
{
	Naui_String result = { 0 };
	const char *key_name = naui_key_name(keybind.key);
	if (!naui_keybind_valid(keybind) || !key_name)
		return result;

	const Naui_KeyMods own = _naui_key_own_mod(keybind.key);
	for (size_t i = 0; i < sizeof(_naui_mod_names) / sizeof(_naui_mod_names[0]); i++)
	{
		if ((keybind.mods & _naui_mod_names[i].mod) && !(own & _naui_mod_names[i].mod))
		{
			naui_string_append_cstr(&result, _naui_mod_names[i].name);
			naui_string_append_char(&result, '+');
		}
	}

	naui_string_append_cstr(&result, key_name);
	return result;
}

bool naui_keybind_from_string(const char *text, Naui_Keybind *out)
{
	if (!text || !out)
		return false;

	while (*text == ' ')
		text++;

	if (*text == '\0')
	{
		*out = NAUI_KEYBIND_NONE;
		return true;
	}

	Naui_Keybind keybind = NAUI_KEYBIND_NONE;
	const char *cursor = text;
	while (*cursor)
	{
		const Naui_Key key = _naui_key_from_name(cursor, strlen(cursor));
		if (key)
		{
			keybind.key = key;
			break;
		}

		const char *plus = strchr(cursor + 1, '+');
		if (!plus)
			return false;

		const size_t length = (size_t)(plus - cursor);
		Naui_KeyMods mod = NAUI_MOD_NONE;
		for (size_t i = 0; i < sizeof(_naui_mod_names) / sizeof(_naui_mod_names[0]) && !mod; i++)
		{
			if (_naui_keybind_name_equal(cursor, length, _naui_mod_names[i].name))
				mod = _naui_mod_names[i].mod;
		}
		for (size_t i = 0; i < sizeof(_naui_mod_aliases) / sizeof(_naui_mod_aliases[0]) && !mod; i++)
		{
			if (_naui_keybind_name_equal(cursor, length, _naui_mod_aliases[i].name))
				mod = _naui_mod_aliases[i].mod;
		}

		if (!mod)
			return false;

		keybind.mods |= mod;
		cursor = plus + 1;
	}

	if (!keybind.key)
		return false;

	*out = keybind;
	return true;
}

void naui_keybind_update(void)
{
	Naui_KeybindState *state = &_naui_keybind_state;
	state->frame_blocked = false;

	if (state->capture_pending)
	{
		state->capture_pending = false;
		state->capturing = true;
	}
}
