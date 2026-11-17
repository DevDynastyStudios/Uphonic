#define UPH_AUTOSAVE_FORCE_SECONDS 30

typedef struct
{
	uint64_t seen_revision;
	uint64_t dirty_since;
	bool dirty;
	bool unsaved;
}
Uph_AutosaveState;

static Uph_AutosaveState uph__autosave_state;

#pragma region Static Helpers
static uint64_t uph__autosave_elapsed(uint64_t now, uint64_t since)
{
	return now > since ? now - since : 0;
}

static void uph__autosave_poll_changes(uint64_t now)
{
	Uph_AutosaveState *state = &uph__autosave_state;
	const uint64_t revision = naui_action_get_revision();
	if (revision == state->seen_revision)
		return;
 
	state->seen_revision = revision;
	state->unsaved = true;
	uph_state._last_modified_time = now;
 
	if (!state->dirty)
	{
		state->dirty = true;
		state->dirty_since = now;
	}
}
#pragma endregion
 
#pragma region Public API
void uph_autosave_mark_clean(void)
{
	Uph_AutosaveState *state = &uph__autosave_state;
	state->seen_revision = naui_action_get_revision();
	state->dirty = false;
	state->unsaved = false;
}
 
void uph_autosave_mark_recovered(void)
{
	Uph_AutosaveState *state = &uph__autosave_state;
	state->seen_revision = naui_action_get_revision();
	state->dirty = false;
	state->unsaved = true;
}
 
bool uph_autosave_has_unsaved_changes(void)
{
	return uph__autosave_state.unsaved || naui_action_get_revision() != uph__autosave_state.seen_revision;
}
 
static bool uph__autosave_write(uint64_t now, const char *reason)
{
	Uph_AutosaveState *state = &uph__autosave_state;
	if (!uph_project_save(&uph_state.project, UPH_SAVE_TYPE_TEMP))
		return false;
 
	state->dirty = false;
	uph_state._last_autosave_time = now;
	naui_log(NAUI_LOG_INFO, "Autosaved project (%s)", reason);
	return true;
}
 
bool uph_autosave_flush(void)
{
	Uph_AutosaveState *state = &uph__autosave_state;
	const uint64_t now = naui_unix_time();
	uph__autosave_poll_changes(now);
 
	if (!state->dirty)
		return true;
 
	if (naui_string_is_empty(uph_state.project.title))
		return false;
 
	return uph__autosave_write(now, "on close");
}
 
void uph_autosave_update(void)
{
	Uph_AutosaveState *state = &uph__autosave_state;
	const uint64_t now = naui_unix_time();
	uph__autosave_poll_changes(now);
 
	const int32_t idle_seconds = uph_state.settings.general.autosave_idle_timer;
	if (!state->dirty || idle_seconds <= 0)
		return;
 
	if (uph_state.shared.song_timeline_playing || naui_string_is_empty(uph_state.project.title))
		return;

	const uint64_t force_seconds = NAUI_MAX((uint64_t)UPH_AUTOSAVE_FORCE_SECONDS, (uint64_t)idle_seconds);
	const bool idle = uph__autosave_elapsed(now, uph_state._last_modified_time) >= (uint64_t)idle_seconds;
	const bool forced = uph__autosave_elapsed(now, state->dirty_since) >= force_seconds;
	if (!idle && !forced)
		return;
 
	if (uph__autosave_write(now, forced && !idle ? "forced" : "idle"))
		return;
 
	naui_log(NAUI_LOG_WARNING, "Autosave failed, retrying in %d seconds", idle_seconds);
	uph_state._last_modified_time = now;
	state->dirty_since = now;
}
#pragma endregion