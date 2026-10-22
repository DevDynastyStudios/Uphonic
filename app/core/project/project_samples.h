#define UPH_PENDING_DELETE_FILE ".pending_delete"

bool uph_project_add_sample_file(Uph_Project *project, const Naui_Path file_path);
void uph_project_adopt_sample_files(Uph_Project *project, const Naui_Path project_folder);
void uph_project_clear_adopted_samples(void);
void uph_project_rebase_sample_paths(Uph_Project *project, const Naui_Path from_folder, const Naui_Path to_folder);
bool uph_project_references_sample_file(const Uph_Project *project, const char *name);
Naui_List(Naui_String) uph_project_collect_sample_candidates(const Naui_Path project_folder, const Naui_Path temp_folder);	// Call before a canonical save merges the temp folder
void uph_project_queue_dropped_samples(const Uph_Project *project, const Naui_Path project_folder, Naui_List(Naui_String) candidates);	// Call after a canonical save
void uph_project_sweep_deleted_samples(const Uph_Project *project);