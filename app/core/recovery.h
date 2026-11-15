bool uph_recovery_acquire_lock(
	const Naui_Path project_folder
);

void uph_recovery_release_lock(void);

bool uph_recovery_has_autosave(
	const Naui_Path project_folder
);

void uph_recovery_discard_autosave(
	const Naui_Path project_folder
);

void uph_recovery_discard_project(
	const Naui_Path project_folder
);

void uph_recovery_invalidate_autosave(
	const Naui_Path project_folder
);

void uph_recovery_commit_autosave(
	const Naui_Path project_folder
);
