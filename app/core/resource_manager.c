void uph_resources_add_track(Naui_String name)
{
    Uph_Track track = {
        .name = name,
        .color = naui_theme_color("uph_palette_color_1"),
        .volume = 1.0f,
        .index = naui_list_len(uph_state.project.tracks)
    };
    naui_list_push(uph_state.project.tracks, track);
}

void uph_resources_add_automation_track(Uph_Track *parent, Naui_String name)
{
    Uph_Track track = {
        .name = name,
        .type = UPH_RESOURCE_AUTOMATION,
        .color = naui_theme_color("uph_palette_color_1"),
        .index = naui_list_len(parent->subtracks),
        .parent = parent
    };
    naui_list_push(parent->subtracks, track);
}

// TODO: make this recursive
void uph_resources_remove_track(Uph_Track *track)
{
    Naui_List(Uph_Track) list = track->parent ? track->parent->subtracks : uph_state.project.tracks;
    uint32_t removed_index = track->index;

    uph_unload_plugin_effect(&track->instrument);
    naui_list_free(track->blocks);
    naui_list_remove(list, removed_index);

    for (uint32_t i = removed_index; i < (uint32_t)naui_list_len(list); i++)
        list[i].index--;
}

static void uph_resources_clear_tracks_recursive(Naui_List(Uph_Track) list)
{
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
        uph_resources_clear_tracks_recursive(&list[i]);
	naui_list_clear(list);
	naui_list_free(list);
}

void uph_resources_clear_tracks(void)
{
    uph_resources_clear_tracks_recursive(uph_state.project.tracks);
}

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
            {
                naui_list_uremove(blocks, j);
                j--;
            }
            else if (blocks[j].resource_index > resource_index)
            {
                blocks[j].resource_index--;
            }
        }

        if (naui_list_len(blocks) == 0)
            uph_state.project.tracks[i].type = UPH_RESOURCE_NONE;
    }
}

void uph_resources_remove_sample(Uph_ResourceIndex sample_index)
{
    Uph_Sample sample = uph_state.project.samples[sample_index];

    uph_resources_clear_timeline_blocks_with_resource(UPH_RESOURCE_SAMPLE, sample_index);

    if (--uph_state.project.sample_data[sample.data_index].ref_count == 0)
    {
        naui_list_remove(uph_state.project.sample_data, sample.data_index);

        for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.samples); i++)
        {
            if (uph_state.project.samples[i].data_index > sample.data_index)
                uph_state.project.samples[i].data_index--;
        }
    }

    naui_list_remove(uph_state.project.samples, sample_index);
}

void uph_resources_add_pattern(void)
{
    Uph_MidiPattern pattern = {
        .name = naui_string_from_cstr(NAUI_TR("patterns.default.name"))
    };

    naui_list_push(uph_state.project.midi_patterns, pattern);
}

void uph_resources_copy_pattern(Uph_ResourceIndex pattern_index)
{
    Uph_MidiPattern pattern = uph_state.project.midi_patterns[pattern_index];
    naui_list_push(uph_state.project.midi_patterns, pattern);
}

void uph_resources_remove_pattern(Uph_ResourceIndex pattern_index)
{
    uph_resources_clear_timeline_blocks_with_resource(UPH_RESOURCE_PATTERN, pattern_index);
    naui_list_remove(uph_state.project.midi_patterns, pattern_index);
}

void uph_resources_add_automation(void)
{
    Uph_Automation automation = {
        .name = naui_string_from_cstr(NAUI_TR("automation.default.name"))
    };

    const Uph_AutomationPoint point1 = (Uph_AutomationPoint){.beat = 0.0, .value = 0.5f};
    const Uph_AutomationPoint point2 = (Uph_AutomationPoint){.beat = 4.0, .value = 0.5f};
    const Uph_AutomationPoint point3 = (Uph_AutomationPoint){.beat = 6.0, .value = 0.8f};
    const Uph_AutomationPoint point4 = (Uph_AutomationPoint){.beat = 12.0, .value = 0.2f};

    naui_list_push(automation.points, point1);
    naui_list_push(automation.points, point2);
    naui_list_push(automation.points, point3);
    naui_list_push(automation.points, point4);

    naui_list_push(uph_state.project.automations, automation);
}

void uph_resources_copy_automation(Uph_ResourceIndex automation_index)
{
    Uph_Automation automation = uph_state.project.automations[automation_index];
    naui_list_push(uph_state.project.automations, automation);
}

void uph_resources_remove_automation(Uph_ResourceIndex automation_index)
{
    uph_resources_clear_timeline_blocks_with_resource(UPH_RESOURCE_AUTOMATION, automation_index);
    naui_list_remove(uph_state.project.automations, automation_index);
}