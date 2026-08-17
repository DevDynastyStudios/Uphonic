#define UPHONIC_FOLDER naui_path_join(naui_directory_get(NAUI_DIR_APPDATA), NAUI_PATH("Uphonic"))

bool uph_create_project(Naui_String project_name)
{
	Naui_Path project_dest = naui_path_join(UPHONIC_FOLDER, NAUI_PATH(project_name.data));
	if (naui_path_exists(project_dest))
		return false;

	uint64_t current_time = naui_unix_time();
	uph_state.project.bpm = 120.0f;
	uph_state.project.time_created = current_time;
	uph_state.project.last_accessed = current_time;
	uph_state.project.last_modified = current_time;
	uph_state.project.title = project_name;
	uph_state.project.time_signature = (Uph_TimeSignature){ .numerator = 4, .denominator = 4};
	naui_list_clear(uph_state.project.tracks);
	naui_path_lock(project_dest);
	return true;
}

bool uph_save_project(Uph_Project* project, Uph_SaveType save_type)
{
	const Naui_Path project_folder = naui_path_join(UPHONIC_FOLDER, NAUI_PATH(uph_state.project.title.data));
	const Naui_Path save_dest = (save_type == UPH_SAVE_TYPE_CANONICAL) ? project_folder : naui_path_join(project_folder, NAUI_PATH(".temp"));
	bool save_successful = uph_io_save_project(project, save_dest);

	if (save_successful && save_type == UPH_SAVE_TYPE_CANONICAL)
	{
		uint64_t current_time = naui_unix_time();
		uph_state._last_autosave_time = current_time;
		uph_state._last_modified_time = UINT64_MAX;
	}

	return save_successful;
}

bool uph_export_project(Uph_Project* project, Naui_Path output_path, Uph_ExportFormat format)
{
	bool created_dest_path;
	if (!naui_path_exists(output_path))
		created_dest_path = naui_directory_create(output_path);

	if (!created_dest_path)
		return false;
	
	if (format == UPH_EXPORT_UPH)
	{
		const Naui_Path canonical_save = naui_path_join(UPHONIC_FOLDER, NAUI_PATH(project->title.data));
		const Naui_Path temp_save = naui_path_join(canonical_save, NAUI_PATH(".temp"));
		// bool duplicated_project = naui_directory_merge_to(canonical_save, temp_save, output_path);	// Make this function real // Might not be needed, see if we can merge via archive
		// if(!duplicated_project)
		// 	return false;

		naui_path_unlock(canonical_save);
		// Use Archives to turn the project into a uph file at the output destination.
		Naui_Archive archive;
		//Create archive here
		if (naui_archive_is_valid(&archive))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to create archive");
			naui_directory_remove_all(output_path);
			return false;
		}

		if (!naui_archive_add_folder(&archive, canonical_save, NAUI_PATH("")))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to add canonical folder to archive");
			naui_directory_remove_all(output_path);
			return false;
		}

		if (!naui_archive_add_folder(&archive, temp_save, NAUI_PATH("")))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to add temp folder to archive");
			naui_directory_remove_all(output_path);
			return false;
		}


		naui_path_lock(canonical_save);
		naui_log(NAUI_LOG_INFO, "Project(%s) saved to: %s", project->title.data, output_path.data);
		return true;
	}

	return false;
}

bool uph_load_project(Naui_Path project_path)
{
	
	return false;
}

bool uph_add_file(Naui_Path file_path, bool make_copy)
{

	return false;
}