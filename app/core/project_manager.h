typedef uint8_t Uph_SaveType;
enum
{
	UPH_SAVE_TYPE_CANONICAL,
	UPH_SAVE_TYPE_TEMP
};

bool uph_project_save(
	Uph_Project* project,
	Uph_SaveType save_type
);

bool uph_project_has_been_saved(
	const Uph_Project* project
);

Naui_Path uph_project_get_path(
	const Uph_Project* project
);
