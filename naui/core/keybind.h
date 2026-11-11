typedef uint8_t Naui_KeyMods;
enum
{
	NAUI_MOD_NONE	= 0,
	NAUI_MOD_CTRL	= 1 << 0,
	NAUI_MOD_SHIFT	= 1 << 1,
	NAUI_MOD_ALT	= 1 << 2,
	NAUI_MOD_SUPER	= 1 << 3,
	NAUI_MOD_ALL	= NAUI_MOD_CTRL | NAUI_MOD_SHIFT | NAUI_MOD_ALT | NAUI_MOD_SUPER,

#if defined(NAUI_MACOS)
	NAUI_MOD_PRIMARY = NAUI_MOD_SUPER
#else
	NAUI_MOD_PRIMARY = NAUI_MOD_CTRL
#endif
};

typedef struct
{
	Naui_Key key;
	Naui_KeyMods mods;
}
Naui_Keybind;

#define NAUI_KEYBIND(mods_, key_)	((Naui_Keybind){ .key = (Naui_Key)(key_), .mods = (Naui_KeyMods)(mods_) })
#define NAUI_KEYBIND_KEY(key_)	NAUI_KEYBIND(NAUI_MOD_NONE, (key_))
#define NAUI_KEYBIND_NONE			((Naui_Keybind){ 0 })
#define NAUI_KEYBIND_INIT(mods_, key_)	{ .key = (Naui_Key)(key_), .mods = (Naui_KeyMods)(mods_) }

static inline bool naui_keybind_valid(Naui_Keybind keybind) { return keybind.key != 0; }
static inline bool naui_keybind_equal(Naui_Keybind a, Naui_Keybind b) { return a.key == b.key && a.mods == b.mods; }

NAUI_API bool naui_keybind_down(Naui_Keybind keybind);
NAUI_API bool naui_keybind_pressed(Naui_Keybind keybind);
NAUI_API bool naui_keybind_pressed_repeat(Naui_Keybind keybind);
NAUI_API bool naui_keybind_released(Naui_Keybind keybind);

NAUI_API void naui_keybind_set_enabled(bool enabled);

NAUI_API void naui_keybind_capture_begin(void);
NAUI_API void naui_keybind_capture_cancel(void);
NAUI_API bool naui_keybind_capturing(void);
NAUI_API bool naui_keybind_capture(Naui_Keybind *out);

NAUI_API Naui_String naui_keybind_to_string(Naui_Keybind keybind);
NAUI_API bool naui_keybind_from_string(const char *text, Naui_Keybind *out);

void naui_keybind_update(void);
