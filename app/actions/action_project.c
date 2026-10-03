typedef struct
{

} Uph_ActionProjectSettings;

bool _uph_action_project_create_execute(void* userdata)
{
	//Uph_ActionPatternCreate* data = userdata;
	
	return true;
}

bool _uph_action_project_create_undo(void* userdata)
{
	//Uph_ActionPatternCreate* data = userdata;
	//if (!data->resource_exists)
	//	return false;
//
	//uph_resources_remove_pattern(data->resource_index);
	//data->resource_exists = false;
	return true;
}

bool _uph_action_project_create_redo(void* userdata)
{
	return _uph_action_pattern_create_execute(userdata);
}
