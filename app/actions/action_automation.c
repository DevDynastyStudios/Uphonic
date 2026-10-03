typedef struct
{
	Uph_ResourceIndex resource_index;
	Uph_Automation automation;
	Naui_List(Uph_ActionBlockSnapshot) blocks;
	bool deleted;
} Uph_ActionAutomationDelete;

typedef struct
{
	Uph_ResourceIndex source_index;
	Uph_ResourceIndex new_index;
	bool resource_exists;
} Uph_ActionAutomationDuplicate;

typedef struct
{
	Uph_ResourceIndex automation_index;
	uint32_t point_index;
	Uph_AutomationPoint point;
	bool applied_live;
} Uph_ActionAutomationPointCreate;

typedef struct
{
	Uph_ResourceIndex automation_index;
	uint32_t point_index;
	Uph_AutomationPoint point;
} Uph_ActionAutomationPointDelete;

typedef struct
{
	Uph_ResourceIndex automation_index;
	uint32_t point_index;
	Uph_AutomationPoint old_point;
	Uph_AutomationPoint new_point;
	bool applied_live;
} Uph_ActionAutomationPointMove;

#pragma region Automation Create
bool _uph_action_automation_create_execute(void* userdata)
{
	Uph_ActionResourceCreate* data = userdata;
	uph_resources_add_automation();
	data->resource_index = (uint32_t)naui_list_len(uph_state.project.automations) - 1;
	data->resource_exists = true;
	uph_action_resource_restored(UPH_RESOURCE_AUTOMATION, data->resource_index);
	return true;
}

bool _uph_action_automation_create_undo(void* userdata)
{
	Uph_ActionResourceCreate* data = userdata;
	if (!data->resource_exists || data->resource_index >= (uint32_t)naui_list_len(uph_state.project.automations))
		return false;

	naui_list_free(uph_state.project.automations[data->resource_index].points);
	naui_list_remove(uph_state.project.automations, data->resource_index);
	uph_action_resource_removed(UPH_RESOURCE_AUTOMATION, data->resource_index);
	data->resource_exists = false;
	return true;
}

bool _uph_action_automation_create_redo(void* userdata)
{
	return _uph_action_automation_create_execute(userdata);
}

#pragma endregion

#pragma region Automation Delete
bool _uph_action_automation_delete_execute(void* userdata)
{
	Uph_ActionAutomationDelete* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.automations))
		return false;

	naui_list_free(data->blocks);
	data->blocks = NULL;

	data->automation = uph_state.project.automations[data->resource_index];
	uph_block_unlink_resource(UPH_RESOURCE_AUTOMATION, data->resource_index, &data->blocks);
	naui_list_remove(uph_state.project.automations, data->resource_index);

	data->deleted = true;
	uph_action_resource_removed(UPH_RESOURCE_AUTOMATION, data->resource_index);
	return true;
}

bool _uph_action_automation_delete_undo(void* userdata)
{
	Uph_ActionAutomationDelete* data = userdata;
	if (!data->deleted || data->resource_index > (uint32_t)naui_list_len(uph_state.project.automations))
		return false;

	naui_list_insert(uph_state.project.automations, data->automation, data->resource_index);
	uph_block_relink_resource(UPH_RESOURCE_AUTOMATION, data->resource_index, data->blocks);

	data->deleted = false;
	uph_action_resource_restored(UPH_RESOURCE_AUTOMATION, data->resource_index);
	return true;
}

bool _uph_action_automation_delete_redo(void* userdata)
{
	return _uph_action_automation_delete_execute(userdata);
}

void _uph_action_automation_delete_destroy(void* userdata)
{
	Uph_ActionAutomationDelete* data = userdata;
	if (data->deleted)
		naui_list_free(data->automation.points);

	naui_list_free(data->blocks);
	data->blocks = NULL;
}

#pragma endregion

#pragma region Automation Rename
bool _uph_action_automation_rename_execute(void* userdata)
{
	Uph_ActionResourceRename* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.automations))
		return false;

	uph_state.project.automations[data->resource_index].name = data->new_name;
	return true;
}

bool _uph_action_automation_rename_undo(void* userdata)
{
	Uph_ActionResourceRename* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.automations))
		return false;

	uph_state.project.automations[data->resource_index].name = data->old_name;
	return true;
}

bool _uph_action_automation_rename_redo(void* userdata)
{
	return _uph_action_automation_rename_execute(userdata);
}

#pragma endregion

#pragma region Automation Duplicate
bool _uph_action_automation_duplicate_execute(void* userdata)
{
	Uph_ActionAutomationDuplicate* data = userdata;
	if (data->source_index >= (uint32_t)naui_list_len(uph_state.project.automations))
		return false;

	uph_resources_copy_automation(data->source_index);
	data->new_index = (uint32_t)naui_list_len(uph_state.project.automations) - 1;
	data->resource_exists = true;
	uph_action_resource_restored(UPH_RESOURCE_AUTOMATION, data->new_index);
	return true;
}

bool _uph_action_automation_duplicate_undo(void* userdata)
{
	Uph_ActionAutomationDuplicate* data = userdata;
	if (!data->resource_exists || data->new_index >= (uint32_t)naui_list_len(uph_state.project.automations))
		return false;

	naui_list_free(uph_state.project.automations[data->new_index].points);
	naui_list_remove(uph_state.project.automations, data->new_index);
	uph_action_resource_removed(UPH_RESOURCE_AUTOMATION, data->new_index);
	data->resource_exists = false;
	return true;
}

bool _uph_action_automation_duplicate_redo(void* userdata)
{
	return _uph_action_automation_duplicate_execute(userdata);
}

#pragma endregion

#pragma region Automation Points
static Uph_Automation* uph_automation_get(Uph_ResourceIndex automation_index)
{
	if (automation_index >= (uint32_t)naui_list_len(uph_state.project.automations))
		return NULL;

	return &uph_state.project.automations[automation_index];
}

static bool uph_automation_point_insert(Uph_ResourceIndex automation_index, uint32_t point_index, Uph_AutomationPoint point)
{
	Uph_Automation* automation = uph_automation_get(automation_index);
	if (!automation)
		return false;

	if (point_index > (uint32_t)naui_list_len(automation->points))
		point_index = (uint32_t)naui_list_len(automation->points);

	naui_list_insert(automation->points, point, point_index);
	return true;
}

static bool uph_automation_point_take(Uph_ResourceIndex automation_index, uint32_t point_index, Uph_AutomationPoint *out)
{
	Uph_Automation* automation = uph_automation_get(automation_index);
	if (!automation || point_index >= (uint32_t)naui_list_len(automation->points))
		return false;

	*out = automation->points[point_index];
	naui_list_remove(automation->points, point_index);
	return true;
}

static bool uph_automation_point_set(Uph_ResourceIndex automation_index, uint32_t point_index, Uph_AutomationPoint point)
{
	Uph_Automation* automation = uph_automation_get(automation_index);
	if (!automation || point_index >= (uint32_t)naui_list_len(automation->points))
		return false;

	automation->points[point_index] = point;
	return true;
}

bool _uph_action_automation_point_create_execute(void* userdata)
{
	Uph_ActionAutomationPointCreate* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_automation_get(data->automation_index) != NULL;
	}

	return uph_automation_point_insert(data->automation_index, data->point_index, data->point);
}

bool _uph_action_automation_point_create_undo(void* userdata)
{
	Uph_ActionAutomationPointCreate* data = userdata;
	return uph_automation_point_take(data->automation_index, data->point_index, &data->point);
}

bool _uph_action_automation_point_create_redo(void* userdata)
{
	Uph_ActionAutomationPointCreate* data = userdata;
	return uph_automation_point_insert(data->automation_index, data->point_index, data->point);
}

bool _uph_action_automation_point_delete_execute(void* userdata)
{
	Uph_ActionAutomationPointDelete* data = userdata;
	return uph_automation_point_take(data->automation_index, data->point_index, &data->point);
}

bool _uph_action_automation_point_delete_undo(void* userdata)
{
	Uph_ActionAutomationPointDelete* data = userdata;
	return uph_automation_point_insert(data->automation_index, data->point_index, data->point);
}

bool _uph_action_automation_point_delete_redo(void* userdata)
{
	return _uph_action_automation_point_delete_execute(userdata);
}

bool _uph_action_automation_point_move_execute(void* userdata)
{
	Uph_ActionAutomationPointMove* data = userdata;
	if (data->applied_live)
	{
		data->applied_live = false;
		return uph_automation_get(data->automation_index) != NULL;
	}

	return uph_automation_point_set(data->automation_index, data->point_index, data->new_point);
}

bool _uph_action_automation_point_move_undo(void* userdata)
{
	Uph_ActionAutomationPointMove* data = userdata;
	return uph_automation_point_set(data->automation_index, data->point_index, data->old_point);
}

bool _uph_action_automation_point_move_redo(void* userdata)
{
	Uph_ActionAutomationPointMove* data = userdata;
	return uph_automation_point_set(data->automation_index, data->point_index, data->new_point);
}

#pragma endregion
