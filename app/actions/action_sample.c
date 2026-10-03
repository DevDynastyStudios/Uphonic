typedef struct
{
	Uph_ResourceIndex resource_index;
	Uph_Sample sample;
	Uph_SampleData data;
	Uph_ResourceIndex data_index;
	bool data_removed;
	bool deleted;
	Naui_List(Uph_ActionBlockSnapshot) blocks;
} Uph_ActionSampleDelete;

typedef struct
{
	Uph_ResourceIndex source_index;
	Uph_ResourceIndex new_index;
	bool resource_exists;
} Uph_ActionSampleDuplicate;

#pragma region Sample Delete
bool _uph_action_sample_delete_execute(void* userdata)
{
	Uph_ActionSampleDelete* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.samples))
		return false;

	const Uph_Sample sample = uph_state.project.samples[data->resource_index];
	if (sample.data_index >= (uint32_t)naui_list_len(uph_state.project.sample_data))
		return false;

	naui_list_free(data->blocks);
	data->blocks = NULL;

	data->sample = sample;
	data->data_index = sample.data_index;
	uph_block_unlink_resource(UPH_RESOURCE_SAMPLE, data->resource_index, &data->blocks);
	naui_list_remove(uph_state.project.samples, data->resource_index);

	Uph_SampleData* sample_data = &uph_state.project.sample_data[data->data_index];
	data->data_removed = sample_data->ref_count <= 1;
	if (data->data_removed)
	{
		data->data = *sample_data;
		naui_list_remove(uph_state.project.sample_data, data->data_index);

		for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.samples); i++)
		{
			if (uph_state.project.samples[i].data_index > data->data_index)
				uph_state.project.samples[i].data_index--;
		}
	}
	else
	{
		sample_data->ref_count--;
	}

	data->deleted = true;
	uph_action_resource_removed(UPH_RESOURCE_SAMPLE, data->resource_index);
	return true;
}

bool _uph_action_sample_delete_undo(void* userdata)
{
	Uph_ActionSampleDelete* data = userdata;
	if (!data->deleted || data->resource_index > (uint32_t)naui_list_len(uph_state.project.samples))
		return false;

	if (data->data_removed)
	{
		if (data->data_index > (uint32_t)naui_list_len(uph_state.project.sample_data))
			return false;

		for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.samples); i++)
		{
			if (uph_state.project.samples[i].data_index >= data->data_index)
				uph_state.project.samples[i].data_index++;
		}

		naui_list_insert(uph_state.project.sample_data, data->data, data->data_index);
	}
	else
	{
		uph_state.project.sample_data[data->data_index].ref_count++;
	}

	naui_list_insert(uph_state.project.samples, data->sample, data->resource_index);
	uph_block_relink_resource(UPH_RESOURCE_SAMPLE, data->resource_index, data->blocks);

	data->deleted = false;
	uph_action_resource_restored(UPH_RESOURCE_SAMPLE, data->resource_index);
	return true;
}

bool _uph_action_sample_delete_redo(void* userdata)
{
	return _uph_action_sample_delete_execute(userdata);
}

void _uph_action_sample_delete_destroy(void* userdata)
{
	Uph_ActionSampleDelete* data = userdata;
	if (data->deleted && data->data_removed)
		uph_audio_engine_unload_sample_data(&data->data);

	naui_list_free(data->blocks);
	data->blocks = NULL;
}

#pragma endregion

#pragma region Sample Rename
bool _uph_action_sample_rename_execute(void* userdata)
{
	Uph_ActionResourceRename* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.samples))
		return false;

	uph_state.project.samples[data->resource_index].name = data->new_name;
	return true;
}

bool _uph_action_sample_rename_undo(void* userdata)
{
	Uph_ActionResourceRename* data = userdata;
	if (data->resource_index >= (uint32_t)naui_list_len(uph_state.project.samples))
		return false;

	uph_state.project.samples[data->resource_index].name = data->old_name;
	return true;
}

bool _uph_action_sample_rename_redo(void* userdata)
{
	return _uph_action_sample_rename_execute(userdata);
}

#pragma endregion

#pragma region Sample Duplicate
bool _uph_action_sample_duplicate_execute(void* userdata)
{
	Uph_ActionSampleDuplicate* data = userdata;
	if (data->source_index >= (uint32_t)naui_list_len(uph_state.project.samples))
		return false;

	uph_resources_copy_sample(data->source_index);
	data->new_index = (uint32_t)naui_list_len(uph_state.project.samples) - 1;
	data->resource_exists = true;
	uph_action_resource_restored(UPH_RESOURCE_SAMPLE, data->new_index);
	return true;
}

bool _uph_action_sample_duplicate_undo(void* userdata)
{
	Uph_ActionSampleDuplicate* data = userdata;
	if (!data->resource_exists || data->new_index >= (uint32_t)naui_list_len(uph_state.project.samples))
		return false;

	const Uph_Sample copy = uph_state.project.samples[data->new_index];
	if (copy.data_index < (uint32_t)naui_list_len(uph_state.project.sample_data))
		uph_state.project.sample_data[copy.data_index].ref_count--;

	naui_list_remove(uph_state.project.samples, data->new_index);
	uph_action_resource_removed(UPH_RESOURCE_SAMPLE, data->new_index);
	data->resource_exists = false;
	return true;
}

bool _uph_action_sample_duplicate_redo(void* userdata)
{
	return _uph_action_sample_duplicate_execute(userdata);
}

#pragma endregion
