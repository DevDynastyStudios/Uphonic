typedef uint32_t Uph_KeybindScope;
enum
{
	UPH_KEYBIND_SCOPE_ANY				= 0,
	UPH_KEYBIND_SCOPE_SONG_TIMELINE		= 1 << 0,
	UPH_KEYBIND_SCOPE_MIDI_EDITOR		= 1 << 1,
	UPH_KEYBIND_SCOPE_AUTOMATION_EDITOR	= 1 << 2,
	UPH_KEYBIND_SCOPE_RESOURCE_LIST		= 1 << 3,
	UPH_KEYBIND_SCOPE_SAMPLE_BROWSER	= 1 << 4
};

typedef struct
{
	const char *id;
	const char *label_key;
	const char *group_key;
	Uph_KeybindScope scope;
	Naui_Keybind *chord;
	Naui_Keybind default_chord;
}
Uph_KeybindInfo;

const Uph_KeybindInfo *uph_keybind_infos(uint32_t *out_count);
const Uph_KeybindInfo *uph_keybind_find(const char *id);

void uph_keybinds_set_defaults(void);
void uph_keybind_reset(const Uph_KeybindInfo *info);

const Uph_KeybindInfo *uph_keybind_conflict(const Uph_KeybindInfo *info);