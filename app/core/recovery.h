#define UPH_RECOVERY_LOCK_FILE ".lock"
#define UPH_RECOVERY_AUTOSAVE_MARKER ".autosave"

// Holds the lock of the current project. Returns false if another running project is locked.
bool uph_recovery_acquire_lock(const Naui_Path project_folder);

// Unlocks the held project and deletes `.lock`. Does nothing if no lock is held.
void uph_recovery_release_lock(void);

// Returns true if the project's `.lock` is stale, i.e. the last session didn't close cleanly.
bool uph_recovery_project_crashed(const Naui_Path project_folder);

// Deletes a stale `.lock` so the project no longer counts as crashed.
void uph_recovery_clear_crash(const Naui_Path project_folder);

// Scans the workspace for a project that crashed and has an autosave to restore.
// If several qualify, the most recently autosaved one is returned.
bool uph_recovery_find_crashed_project(const Naui_Path workspace_folder, Naui_Path *out_project_folder);

bool uph_recovery_has_autosave(const Naui_Path project_folder);
void uph_recovery_discard_autosave(const Naui_Path project_folder);

// Wrap the write of an autosave with these (invalidate before writing, commit after succeeding).
void uph_recovery_invalidate_autosave(const Naui_Path project_folder);
void uph_recovery_commit_autosave(const Naui_Path project_folder);