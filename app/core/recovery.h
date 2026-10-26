#define UPH_RECOVERY_LOCK_FILE ".lock"
#define UPH_RECOVERY_AUTOSAVE_MARKER ".autosave"

bool uph_recovery_acquire_lock(const Naui_Path project_folder);
void uph_recovery_release_lock(void);

bool uph_recovery_project_crashed(const Naui_Path project_folder);
void uph_recovery_clear_crash(const Naui_Path project_folder);
bool uph_recovery_find_recoverable_project(const Naui_Path workspace_folder, Naui_Path *out_project_folder);

bool uph_recovery_has_autosave(const Naui_Path project_folder);
void uph_recovery_discard_autosave(const Naui_Path project_folder);
void uph_recovery_discard_project(const Naui_Path project_folder);

void uph_recovery_invalidate_autosave(const Naui_Path project_folder);
void uph_recovery_commit_autosave(const Naui_Path project_folder);