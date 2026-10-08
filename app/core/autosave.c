#define UPH_AUTOSAVE_FORCE_SECONDS 30

static uint64_t s_autosave_seen_revision;
static uint64_t s_autosave_dirty_since;
static bool s_autosave_dirty;
static bool s_autosave_unsaved;

#pragma region Static Helpers
static uint64_t _uph_autosave_elapsed(uint64_t now, uint64_t since)
{
	return now > since ? now - since : 0;
}

static void _uph_autosave_poll_changes(uint64_t now)
{
	const uint64_t revision = naui_action_get_revision();
	if (revision == s_autosave_seen_revision)
		return;
 
	s_autosave_seen_revision = revision;
	s_autosave_unsaved = true;
	uph_state._last_modified_time = now;
 
	if (!s_autosave_dirty)
	{
		s_autosave_dirty = true;
		s_autosave_dirty_since = now;
	}
}
#pragma endregion
 
#pragma region Public API
void uph_autosave_mark_clean(void)
{
	s_autosave_seen_revision = naui_action_get_revision();
	s_autosave_dirty = false;
	s_autosave_unsaved = false;
}
 
void uph_autosave_mark_recovered(void)
{
	s_autosave_seen_revision = naui_action_get_revision();
	s_autosave_dirty = false;
	s_autosave_unsaved = true;
}
 
bool uph_autosave_has_unsaved_changes(void)
{
	return s_autosave_unsaved || naui_action_get_revision() != s_autosave_seen_revision;
}
 
static bool _uph_autosave_write(uint64_t now, const char *reason)
{
	if (!uph_project_save(&uph_state.project, UPH_SAVE_TYPE_TEMP))
		return false;
 
	s_autosave_dirty = false;
	uph_state._last_autosave_time = now;
	naui_log(NAUI_LOG_INFO, "Autosaved project (%s)", reason);
	return true;
}
 
bool uph_autosave_flush(void)
{
	const uint64_t now = naui_unix_time();
	_uph_autosave_poll_changes(now);
 
	if (!s_autosave_dirty)
		return true;
 
	if (naui_string_is_empty(uph_state.project.title))
		return false;
 
	return _uph_autosave_write(now, "on close");
}
 
void uph_autosave_update(void)
{
	const uint64_t now = naui_unix_time();
	_uph_autosave_poll_changes(now);
 
	const int32_t idle_seconds = uph_state.settings.general.autosave_idle_timer;
	if (!s_autosave_dirty || idle_seconds <= 0)
		return;
 
	if (uph_state.shared.song_timeline_playing || naui_string_is_empty(uph_state.project.title))
		return;

	const uint64_t force_seconds = NAUI_MAX((uint64_t)UPH_AUTOSAVE_FORCE_SECONDS, (uint64_t)idle_seconds);
	const bool idle = _uph_autosave_elapsed(now, uph_state._last_modified_time) >= (uint64_t)idle_seconds;
	const bool forced = _uph_autosave_elapsed(now, s_autosave_dirty_since) >= force_seconds;
	if (!idle && !forced)
		return;
 
	if (_uph_autosave_write(now, forced && !idle ? "forced" : "idle"))
		return;
 
	naui_log(NAUI_LOG_WARNING, "Autosave failed, retrying in %d seconds", idle_seconds);
	uph_state._last_modified_time = now;
	s_autosave_dirty_since = now;
}
#pragma endregion