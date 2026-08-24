#define UPHONIC_FOLDER naui_path_join(naui_directory_get(NAUI_DIR_APPDATA), NAUI_PATH("Uphonic"))

bool uph_project_create(Naui_String project_name)
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

bool uph_project_save(Uph_Project* project, Uph_SaveType save_type)
{
	const Naui_Path project_folder = uph_project_get_path(project);
	const Naui_Path save_dest = (save_type == UPH_SAVE_TYPE_CANONICAL) ? project_folder : naui_path_join(project_folder, NAUI_PATH(".temp"));
	bool save_successful = uph_io_save_project(project, save_dest);

	if (save_successful && save_type == UPH_SAVE_TYPE_CANONICAL)
	{
		uint64_t current_time = naui_unix_time();
		uph_state._last_autosave_time = current_time;
		uph_state._last_modified_time = current_time;
	}

	return save_successful;
}

bool uph_project_export(Uph_Project* project, const Naui_Path output_path, Uph_ExportFormat format)
{
	if (format == UPH_EXPORT_UPH)
	{
		bool created_dest_path;
		if (!naui_path_exists(output_path))
			created_dest_path = naui_directory_create(output_path);

		if (!created_dest_path)
			return false;
	
		const Naui_Path canonical_save = uph_project_get_path(project);
		const Naui_Path temp_save = naui_path_join(canonical_save, NAUI_PATH(".temp"));
		naui_path_unlock(canonical_save);
		Naui_Archive archive = NAUI_ARCHIVE_INIT;
		naui_archive_open(&archive, output_path, NAUI_ARCHIVE_MODE_WRITE);

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

		if (naui_path_exists(temp_save) && !naui_archive_add_folder(&archive, temp_save, NAUI_PATH("")))
		{
			naui_log(NAUI_LOG_ERROR, "Failed to add temp folder to archive");
			naui_directory_remove_all(output_path);
			return false;
		}

		naui_path_lock(canonical_save);
		naui_log(NAUI_LOG_INFO, "Project(%s) saved to: %s", project->title.data, output_path.data);
		return true;
	}
	else if (format == UPH_EXPORT_WAV)
	{
		double length = uph_audio_engine_get_song_length_beats();
		if (fabs(length) < NAUI_EPSILON)
		{
			naui_log(NAUI_LOG_WARNING, "Unable to export WAV: project contains no audio to render.");
			return false;
		}

		uph_audio_engine_export_to_wav(naui_file_filename(&output_path).data, 0, length);
		return true;
	}

	return false;
}

bool uph_project_load(Uph_Project* project, const Naui_Path project_path)
{
	
	return false;
}

bool uph_project_add_file(Uph_Project* project, const Naui_Path file_path)
{
	if (!naui_path_exists(file_path) || naui_path_is_directory(file_path))
		return false;

	Naui_Path res_dest = file_path;
	if (!uph_state.settings.general.copy_resources)
	{
		Naui_Path copy_path = naui_path_join(uph_project_get_path(project), NAUI_PATH(".temp"));
		// Copy Audio into this folder
		// res_dest = Audio Path Here
	}

	// TODO (from box to chimpchi): If you are gonna do this do it with the new sample data implementation in mind
	// You can use the resource manager I added or drag the code from it in the project manager if you don't want them separate

	//Uph_Sample sample_resource = uph_audio_engine_load_sample(res_dest);
    //naui_list_push(uph_state.project.samples, sample_resource);
	return true;
}

Naui_Path uph_project_get_path(Uph_Project* project)
{
	return naui_path_join(UPHONIC_FOLDER, NAUI_PATH(project->title.data));
}