#define UPH_KEYBIND_GROUP_GENERAL		"settings.shortcuts.group.general"
#define UPH_KEYBIND_GROUP_TRANSPORT		"settings.shortcuts.group.transport"
#define UPH_KEYBIND_GROUP_EDITING		"settings.shortcuts.group.editing"
#define UPH_KEYBIND_GROUP_AUTOMATION	"settings.shortcuts.group.automation"
#define UPH_KEYBIND_GROUP_BROWSER		"settings.shortcuts.group.browser"

#define UPH_KB_TL	UPH_KEYBIND_SCOPE_SONG_TIMELINE
#define UPH_KB_MIDI	UPH_KEYBIND_SCOPE_MIDI_EDITOR
#define UPH_KB_AUTO	UPH_KEYBIND_SCOPE_AUTOMATION_EDITOR
#define UPH_KB_RL	UPH_KEYBIND_SCOPE_RESOURCE_LIST
#define UPH_KB_SB	UPH_KEYBIND_SCOPE_SAMPLE_BROWSER

static const Uph_KeybindInfo uph_keybind_info_table[] =
{
	{ "new_project", "settings.shortcuts.new_project", UPH_KEYBIND_GROUP_GENERAL, UPH_KEYBIND_SCOPE_ANY, &uph_state.settings.keybinds.new_project, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY, NAUI_KEY_N) },
	{ "open_project", "settings.shortcuts.open_project", UPH_KEYBIND_GROUP_GENERAL, UPH_KEYBIND_SCOPE_ANY, &uph_state.settings.keybinds.open_project, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY, NAUI_KEY_O) },
	{ "save_project", "settings.shortcuts.save_project", UPH_KEYBIND_GROUP_GENERAL, UPH_KEYBIND_SCOPE_ANY, &uph_state.settings.keybinds.save_project, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY, NAUI_KEY_S) },
	{ "save_project_as", "settings.shortcuts.save_project_as", UPH_KEYBIND_GROUP_GENERAL, UPH_KEYBIND_SCOPE_ANY, &uph_state.settings.keybinds.save_project_as, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY | NAUI_MOD_SHIFT, NAUI_KEY_S) },
	{ "undo", "settings.shortcuts.undo", UPH_KEYBIND_GROUP_GENERAL, UPH_KEYBIND_SCOPE_ANY, &uph_state.settings.keybinds.undo, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY, NAUI_KEY_Z) },
	{ "redo", "settings.shortcuts.redo", UPH_KEYBIND_GROUP_GENERAL, UPH_KEYBIND_SCOPE_ANY, &uph_state.settings.keybinds.redo, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY, NAUI_KEY_Y) },
	{ "open_settings", "settings.shortcuts.open_settings", UPH_KEYBIND_GROUP_GENERAL, UPH_KEYBIND_SCOPE_ANY, &uph_state.settings.keybinds.open_settings, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_F10) },

	{ "play_pause", "settings.shortcuts.play_pause", UPH_KEYBIND_GROUP_TRANSPORT, UPH_KB_TL | UPH_KB_MIDI, &uph_state.settings.keybinds.play_pause, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_SPACE) },

	{ "delete", "settings.shortcuts.delete", UPH_KEYBIND_GROUP_EDITING, UPH_KB_TL | UPH_KB_MIDI | UPH_KB_AUTO | UPH_KB_RL, &uph_state.settings.keybinds.delete_selected, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_DELETE) },
	{ "select_all", "settings.shortcuts.select_all", UPH_KEYBIND_GROUP_EDITING, UPH_KB_TL | UPH_KB_MIDI, &uph_state.settings.keybinds.select_all, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY, NAUI_KEY_A) },
	{ "deselect", "settings.shortcuts.deselect", UPH_KEYBIND_GROUP_EDITING, UPH_KB_TL | UPH_KB_MIDI | UPH_KB_AUTO, &uph_state.settings.keybinds.deselect, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_ESCAPE) },
	{ "duplicate", "settings.shortcuts.duplicate", UPH_KEYBIND_GROUP_EDITING, UPH_KB_TL | UPH_KB_MIDI, &uph_state.settings.keybinds.duplicate, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY, NAUI_KEY_D) },
	{ "quantize", "settings.shortcuts.quantize", UPH_KEYBIND_GROUP_EDITING, UPH_KB_MIDI, &uph_state.settings.keybinds.quantize, NAUI_KEYBIND_INIT(NAUI_MOD_PRIMARY, NAUI_KEY_Q) },

	{ "automation_nudge_up", "settings.shortcuts.automation_nudge_up", UPH_KEYBIND_GROUP_AUTOMATION, UPH_KB_AUTO, &uph_state.settings.keybinds.automation_nudge_up, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_UP) },
	{ "automation_nudge_down", "settings.shortcuts.automation_nudge_down", UPH_KEYBIND_GROUP_AUTOMATION, UPH_KB_AUTO, &uph_state.settings.keybinds.automation_nudge_down, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_DOWN) },
	{ "automation_nudge_left", "settings.shortcuts.automation_nudge_left", UPH_KEYBIND_GROUP_AUTOMATION, UPH_KB_AUTO, &uph_state.settings.keybinds.automation_nudge_left, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_LEFT) },
	{ "automation_nudge_right", "settings.shortcuts.automation_nudge_right", UPH_KEYBIND_GROUP_AUTOMATION, UPH_KB_AUTO, &uph_state.settings.keybinds.automation_nudge_right, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_RIGHT) },
	{ "automation_fine_nudge_up", "settings.shortcuts.automation_fine_nudge_up", UPH_KEYBIND_GROUP_AUTOMATION, UPH_KB_AUTO, &uph_state.settings.keybinds.automation_fine_nudge_up, NAUI_KEYBIND_INIT(NAUI_MOD_SHIFT, NAUI_KEY_UP) },
	{ "automation_fine_nudge_down", "settings.shortcuts.automation_fine_nudge_down", UPH_KEYBIND_GROUP_AUTOMATION, UPH_KB_AUTO, &uph_state.settings.keybinds.automation_fine_nudge_down, NAUI_KEYBIND_INIT(NAUI_MOD_SHIFT, NAUI_KEY_DOWN) },
	{ "automation_fine_nudge_left", "settings.shortcuts.automation_fine_nudge_left", UPH_KEYBIND_GROUP_AUTOMATION, UPH_KB_AUTO, &uph_state.settings.keybinds.automation_fine_nudge_left, NAUI_KEYBIND_INIT(NAUI_MOD_SHIFT, NAUI_KEY_LEFT) },
	{ "automation_fine_nudge_right", "settings.shortcuts.automation_fine_nudge_right", UPH_KEYBIND_GROUP_AUTOMATION, UPH_KB_AUTO, &uph_state.settings.keybinds.automation_fine_nudge_right, NAUI_KEYBIND_INIT(NAUI_MOD_SHIFT, NAUI_KEY_RIGHT) },

	{ "browser_up", "settings.shortcuts.browser_up", UPH_KEYBIND_GROUP_BROWSER, UPH_KB_SB, &uph_state.settings.keybinds.browser_up, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_UP) },
	{ "browser_down", "settings.shortcuts.browser_down", UPH_KEYBIND_GROUP_BROWSER, UPH_KB_SB, &uph_state.settings.keybinds.browser_down, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_DOWN) },
	{ "browser_expand", "settings.shortcuts.browser_expand", UPH_KEYBIND_GROUP_BROWSER, UPH_KB_SB, &uph_state.settings.keybinds.browser_expand, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_RIGHT) },
	{ "browser_collapse", "settings.shortcuts.browser_collapse", UPH_KEYBIND_GROUP_BROWSER, UPH_KB_SB, &uph_state.settings.keybinds.browser_collapse, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_LEFT) },
	{ "browser_activate", "settings.shortcuts.browser_activate", UPH_KEYBIND_GROUP_BROWSER, UPH_KB_SB, &uph_state.settings.keybinds.browser_activate, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_ENTER) },
	{ "browser_preview", "settings.shortcuts.browser_preview", UPH_KEYBIND_GROUP_BROWSER, UPH_KB_SB, &uph_state.settings.keybinds.browser_preview, NAUI_KEYBIND_INIT(NAUI_MOD_NONE, NAUI_KEY_SPACE) },
};

#define UPH_KEYBIND_COUNT ((uint32_t)(sizeof(uph_keybind_info_table) / sizeof(uph_keybind_info_table[0])))

const Uph_KeybindInfo *uph_keybind_infos(uint32_t *out_count)
{
	if (out_count)
		*out_count = UPH_KEYBIND_COUNT;

	return uph_keybind_info_table;
}

const Uph_KeybindInfo *uph_keybind_find(const char *id)
{
	for (uint32_t i = 0; i < UPH_KEYBIND_COUNT; i++)
	{
		if (strcmp(uph_keybind_info_table[i].id, id) == 0)
			return &uph_keybind_info_table[i];
	}

	return NULL;
}

void uph_keybind_reset(const Uph_KeybindInfo *info)
{
	*info->chord = info->default_chord;
}

void uph_keybinds_set_defaults(void)
{
	for (uint32_t i = 0; i < UPH_KEYBIND_COUNT; i++)
		uph_keybind_reset(&uph_keybind_info_table[i]);
}

const Uph_KeybindInfo *uph_keybind_conflict(const Uph_KeybindInfo *info)
{
	if (!naui_keybind_valid(*info->chord))
		return NULL;

	for (uint32_t i = 0; i < UPH_KEYBIND_COUNT; i++)
	{
		const Uph_KeybindInfo *other = &uph_keybind_info_table[i];
		if (other == info || !naui_keybind_equal(*other->chord, *info->chord))
			continue;

		if (info->scope == UPH_KEYBIND_SCOPE_ANY || other->scope == UPH_KEYBIND_SCOPE_ANY || (info->scope & other->scope))
			return other;
	}

	return NULL;
}
