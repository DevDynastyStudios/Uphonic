bool uph_resources_add_sample_from_file(Naui_Path path)
{
    Uph_SampleData data = uph_audio_engine_load_sample_data(path);
    if (!uph_audio_engine_sample_data_valid(&data))
        return false;

    data.ref_count = 1;

    Uph_Sample sample = {
        .data_index = naui_list_len(uph_state.project.sample_data),
        .name = naui_view_to_string(naui_file_stem(&path))
    };

    naui_list_push(uph_state.project.sample_data, data);
    naui_list_push(uph_state.project.samples, sample);
	return true;
}

void uph_resources_copy_sample(Uph_ResourceIndex sample_index)
{
    Uph_Sample sample = uph_state.project.samples[sample_index];
    uph_state.project.sample_data[sample.data_index].ref_count++;
    naui_list_push(uph_state.project.samples, sample);
}

static void uph_resources_clear_timeline_blocks_with_resource(Uph_ResourceType track_type, Uph_ResourceIndex resource_index)
{
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
    {
        if (uph_state.project.tracks[i].type != track_type)
            continue;

        Naui_List(Uph_TimelineBlock) blocks = uph_state.project.tracks[i].blocks;
        for (uint32_t j = 0; j < (uint32_t)naui_list_len(blocks); j++)
        {
            if (blocks[j].resource_index == resource_index)
                naui_list_uremove(blocks, j);
        }
    }
}

void uph_resources_remove_sample(Uph_ResourceIndex sample_index)
{
    Uph_Sample sample = uph_state.project.samples[sample_index];

    uph_resources_clear_timeline_blocks_with_resource(UPH_RESOURCE_SAMPLE, sample_index);

    if (--uph_state.project.sample_data[sample.data_index].ref_count == 0)
        naui_list_remove(uph_state.project.sample_data, sample.data_index);

    naui_list_remove(uph_state.project.samples, sample_index);
}

void uph_resources_add_pattern(void)
{
    Uph_MidiPattern pattern = {
        .name = naui_string_from_cstr("Untitled Pattern")
    };

    naui_list_push(uph_state.project.midi_patterns, pattern);
}