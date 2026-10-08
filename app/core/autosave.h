void uph_autosave_update(void);
 
// Clears any pending autosave.
void uph_autosave_mark_clean(void);
 
void uph_autosave_mark_recovered(void);

// Returns if the state of the canonical save differs from the project
bool uph_autosave_has_unsaved_changes(void);
 
// Writes pending changes to .temp immediately.
// Returns true if all data was written.
bool uph_autosave_flush(void);