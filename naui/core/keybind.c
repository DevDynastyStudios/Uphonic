typedef struct
{
	bool disabled;
	bool frame_blocked;
	bool capture_pending;
	bool capturing;
	Naui_Keybind capture;
}
Naui_KeybindState;
static Naui_KeybindState naui__keybind_state;

static const struct { Naui_KeyMods mod; const char *name; } naui__mod_names[] =
{
	{ NAUI_MOD_CTRL,	"Ctrl" },
	{ NAUI_MOD_SUPER,	NAUI_KEY_SUPER_NAME },
	{ NAUI_MOD_ALT,		NAUI_KEY_ALT_NAME },
	{ NAUI_MOD_SHIFT,	"Shift" }
};

static const struct { Naui_KeyMods mod; const char *name; } naui__mod_aliases[] =
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

static Naui_KeyMods naui__key_mods(void)
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

static Naui_KeyMods naui__key_own_mod(Naui_Key key)
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

static bool naui__key_is_modifier(Naui_Key key)
{
	return naui__key_own_mod(key) != NAUI_MOD_NONE;
}

static bool naui__key_held(Naui_Key key)
{
	return naui_key_down(key) || naui_key_pressed(key) || naui_key_released(key);
}

uint32_t naui_keybind_key_count(Naui_Keybind keybind)
{
	uint32_t count = 0;
	while (count < NAUI_KEYBIND_MAX_KEYS && keybind.keys[count])
		count++;

	return count;
}

static bool naui__keybind_has_key(Naui_Keybind keybind, Naui_Key key)
{
	const uint32_t count = naui_keybind_key_count(keybind);
	for (uint32_t i = 0; i < count; i++)
	{
		if (keybind.keys[i] == key)
			return true;
	}

	return false;
}

static Naui_KeyMods naui__keybind_own_mods(Naui_Keybind keybind)
{
	Naui_KeyMods own = NAUI_MOD_NONE;
	const uint32_t count = naui_keybind_key_count(keybind);
	for (uint32_t i = 0; i < count; i++)
		own |= naui__key_own_mod(keybind.keys[i]);

	return own;
}

bool naui_keybind_equal(Naui_Keybind a, Naui_Keybind b)
{
	const uint32_t count = naui_keybind_key_count(a);
	if (a.mods != b.mods || count != naui_keybind_key_count(b))
		return false;

	for (uint32_t i = 0; i < count; i++)
	{
		if (!naui__keybind_has_key(b, a.keys[i]))
			return false;
	}

	return true;
}

static bool naui__keybind_active(Naui_Keybind keybind)
{
	if (!naui_keybind_valid(keybind))
		return false;

	const Naui_KeybindState *state = &naui__keybind_state;
	if (state->disabled || state->frame_blocked || state->capturing || state->capture_pending)
		return false;

	const Naui_KeyMods own = naui__keybind_own_mods(keybind);
	if ((naui__key_mods() & ~own) != (keybind.mods & ~own))
		return false;

	for (uint32_t key = 1; key < NAUI_KEY_MAX; key++)
	{
		if (naui__key_is_modifier((Naui_Key)key) || !naui_key_name((Naui_Key)key))
			continue;

		if (naui__key_held((Naui_Key)key) != naui__keybind_has_key(keybind, (Naui_Key)key))
			return false;
	}

	const uint32_t count = naui_keybind_key_count(keybind);
	for (uint32_t i = 0; i < count; i++)
	{
		if (!naui__key_held(keybind.keys[i]))
			return false;
	}

	return true;
}

bool naui_keybind_down(Naui_Keybind keybind)
{
	if (!naui__keybind_active(keybind))
		return false;

	const uint32_t count = naui_keybind_key_count(keybind);
	for (uint32_t i = 0; i < count; i++)
	{
		if (!naui_key_down(keybind.keys[i]))
			return false;
	}

	return true;
}

bool naui_keybind_pressed(Naui_Keybind keybind)
{
	if (!naui__keybind_active(keybind))
		return false;

	const uint32_t count = naui_keybind_key_count(keybind);
	for (uint32_t i = 0; i < count; i++)
	{
		if (naui_key_pressed(keybind.keys[i]))
			return true;
	}

	return false;
}

bool naui_keybind_pressed_repeat(Naui_Keybind keybind)
{
	if (!naui__keybind_active(keybind))
		return false;

	const uint32_t count = naui_keybind_key_count(keybind);
	for (uint32_t i = 0; i < count; i++)
	{
		if (naui_key_pressed_repeat(keybind.keys[i]))
			return true;
	}

	return false;
}

bool naui_keybind_released(Naui_Keybind keybind)
{
	if (!naui__keybind_active(keybind))
		return false;

	const uint32_t count = naui_keybind_key_count(keybind);
	for (uint32_t i = 0; i < count; i++)
	{
		if (naui_key_released(keybind.keys[i]))
			return true;
	}

	return false;
}

void naui_keybind_set_enabled(bool enabled)
{
	naui__keybind_state.disabled = !enabled;
}

void naui_keybind_capture_begin(void)
{
	Naui_KeybindState *state = &naui__keybind_state;
	state->capture_pending = true;
	state->capturing = false;
	state->capture = NAUI_KEYBIND_NONE;
}

void naui_keybind_capture_cancel(void)
{
	Naui_KeybindState *state = &naui__keybind_state;
	if (state->capture_pending || state->capturing)
		state->frame_blocked = true;

	state->capture_pending = false;
	state->capturing = false;
	state->capture = NAUI_KEYBIND_NONE;
}

bool naui_keybind_capturing(void)
{
	return naui__keybind_state.capture_pending || naui__keybind_state.capturing;
}

Naui_Keybind naui_keybind_capture_progress(void)
{
	return naui__keybind_state.capture;
}

bool naui_keybind_capture(Naui_Keybind *out)
{
	Naui_KeybindState *state = &naui__keybind_state;
	if (!state->capturing)
		return false;

	bool any_down = false;
	for (uint32_t key = 1; key < NAUI_KEY_MAX; key++)
	{
		if (!naui_key_name((Naui_Key)key))
			continue;

		any_down |= naui_key_down((Naui_Key)key);
		if (naui__key_is_modifier((Naui_Key)key) || !naui_key_pressed((Naui_Key)key) || naui__keybind_has_key(state->capture, (Naui_Key)key))
			continue;

		const uint32_t count = naui_keybind_key_count(state->capture);
		if (count < NAUI_KEYBIND_MAX_KEYS)
			state->capture.keys[count] = (Naui_Key)key;
	}

	if (naui_keybind_valid(state->capture))
		state->capture.mods |= naui__key_mods();

	if (any_down || !naui_keybind_valid(state->capture))
		return false;

	if (out)
		*out = state->capture;

	state->capture = NAUI_KEYBIND_NONE;
	state->capturing = false;
	state->frame_blocked = true;
	return true;
}

static bool naui__keybind_name_equal(const char *a, size_t a_length, const char *b)
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

static Naui_Key naui__key_from_name(const char *name, size_t length)
{
	if (!name || length == 0)
		return 0;

	for (uint32_t key = 1; key < NAUI_KEY_MAX; key++)
	{
		const char *key_name = naui_key_name((Naui_Key)key);
		if (key_name && naui__keybind_name_equal(name, length, key_name))
			return (Naui_Key)key;
	}

	if (naui__keybind_name_equal(name, length, "Alt") || naui__keybind_name_equal(name, length, "Option"))
		return NAUI_KEY_ALT;
	if (naui__keybind_name_equal(name, length, "Esc"))
		return NAUI_KEY_ESCAPE;
	if (naui__keybind_name_equal(name, length, "Del"))
		return NAUI_KEY_DELETE;
	if (naui__keybind_name_equal(name, length, "Return"))
		return NAUI_KEY_ENTER;
	if (naui__keybind_name_equal(name, length, "Plus"))
		return NAUI_KEY_EQUAL;

	return 0;
}

static Naui_KeyMods naui__mod_from_name(const char *name, size_t length)
{
	for (size_t i = 0; i < sizeof(naui__mod_names) / sizeof(naui__mod_names[0]); i++)
	{
		if (naui__keybind_name_equal(name, length, naui__mod_names[i].name))
			return naui__mod_names[i].mod;
	}

	for (size_t i = 0; i < sizeof(naui__mod_aliases) / sizeof(naui__mod_aliases[0]); i++)
	{
		if (naui__keybind_name_equal(name, length, naui__mod_aliases[i].name))
			return naui__mod_aliases[i].mod;
	}

	return NAUI_MOD_NONE;
}

uint32_t naui_keybind_names(Naui_Keybind keybind, const char *out[NAUI_KEYBIND_MAX_NAMES])
{
	if (!naui_keybind_valid(keybind))
		return 0;

	const uint32_t count = naui_keybind_key_count(keybind);
	for (uint32_t i = 0; i < count; i++)
	{
		if (!naui_key_name(keybind.keys[i]))
			return 0;
	}

	uint32_t name_count = 0;
	const Naui_KeyMods own = naui__keybind_own_mods(keybind);
	for (size_t i = 0; i < sizeof(naui__mod_names) / sizeof(naui__mod_names[0]); i++)
	{
		if ((keybind.mods & naui__mod_names[i].mod) && !(own & naui__mod_names[i].mod))
			out[name_count++] = naui__mod_names[i].name;
	}

	for (uint32_t i = 0; i < count; i++)
		out[name_count++] = naui_key_name(keybind.keys[i]);

	return name_count;
}

Naui_String naui_keybind_to_string(Naui_Keybind keybind)
{
	Naui_String result = { 0 };
	const char *names[NAUI_KEYBIND_MAX_NAMES];
	const uint32_t count = naui_keybind_names(keybind, names);
	for (uint32_t i = 0; i < count; i++)
	{
		if (i > 0)
			naui_string_append_char(&result, '+');

		naui_string_append_cstr(&result, names[i]);
	}

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
	uint32_t count = 0;
	const char *cursor = text;
	while (*cursor)
	{
		const char *end = cursor + 1;
		bool matched = false;
		while (!matched)
		{
			const char *plus = strchr(end, '+');
			const size_t length = plus ? (size_t)(plus - cursor) : strlen(cursor);
			const Naui_KeyMods mod = plus ? naui__mod_from_name(cursor, length) : NAUI_MOD_NONE;
			const Naui_Key key = mod ? 0 : naui__key_from_name(cursor, length);

			if (mod || key)
			{
				if (key && !naui__keybind_has_key(keybind, key))
				{
					if (count >= NAUI_KEYBIND_MAX_KEYS)
						return false;

					keybind.keys[count++] = key;
				}

				keybind.mods |= mod;
				cursor = plus ? plus + 1 : cursor + length;
				matched = true;
			}
			else if (!plus)
				return false;
			else
				end = plus + 1;
		}
	}

	if (!count)
		return false;

	*out = keybind;
	return true;
}

void naui_keybind_update(void)
{
	Naui_KeybindState *state = &naui__keybind_state;
	state->frame_blocked = false;

	if (state->capture_pending)
	{
		state->capture_pending = false;
		state->capturing = true;
	}
}
