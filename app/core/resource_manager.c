#define UPH_TRACK_PALLETE_SIZE 8

#pragma region Helpers
static void uph_resources_link_track_list(Naui_List(Uph_Track) tracks, Uph_Track *parent)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		tracks[i].parent = parent;
		tracks[i].index = i;
		uph_resources_link_track_list(tracks[i].subtracks, &tracks[i]);
	}
}

static void uph_resources_remove_track_children(Uph_Track *track)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		uph_resources_remove_track_children(&track->subtracks[i]);
		uph_unload_plugin(&track->subtracks[i].instrument);
		naui_list_free(track->subtracks[i].blocks);
	}
	naui_list_clear(track->subtracks);
}

static void uph_resources_clear_tracks_recursive(Naui_List(Uph_Track) list)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
	{
		Uph_Track *track = &list[i];
		uph_resources_clear_tracks_recursive(track->subtracks);
		uph_resources_clear_plugin(&track->instrument);

		for (uint32_t e = 0; e < (uint32_t)naui_list_len(track->effects); e++)
			uph_resources_clear_plugin(&track->effects[e]);

		naui_list_free(track->effects);
		naui_list_free(track->blocks);
	}

	naui_list_free(list);
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
#pragma endregion

#pragma region Public API

Naui_Color uph_resources_track_color(const int32_t color_index)
{
	const int32_t wrapped = color_index % UPH_TRACK_PALLETE_SIZE;
	char key[32];
	snprintf(key, sizeof(key), "uph_palette_color_%d", wrapped);
	return naui_theme_color(key);
}

void uph_resources_link_tracks(Naui_List(Uph_Track) tracks)
{
	uph_resources_link_track_list(tracks, NULL);
}

void uph_resources_clear_plugin(Uph_Plugin *plugin)
{
	uph_unload_plugin(plugin);
	naui_list_free(plugin->params);
	*plugin = (Uph_Plugin){ 0 };
}

void uph_resources_add_track(Naui_String name)
{
	Uph_Track track = {
		.name = name,
		.color_index = 0,
		.volume = 1.0f,
		.index = naui_list_len(uph_state.project.tracks)
	};
	naui_list_push(uph_state.project.tracks, track);
}

void uph_resources_add_automation_track(Uph_Track *parent, Naui_String name, int32_t effect_index, uint64_t param_id)
{
	Uph_Track track = {
		.name = name,
		.type = UPH_RESOURCE_AUTOMATION,
		.color_index = 0,
		.index = naui_list_len(parent->subtracks),
		.parent = parent,
		.automation_param_id = param_id,
		.automation_target_effect_index = effect_index
	};
	naui_list_push(parent->subtracks, track);
}

void uph_resources_remove_track(Uph_Track *track)
{
	Naui_List(Uph_Track) list = track->parent ? track->parent->subtracks : uph_state.project.tracks;
	uint32_t removed_index = track->index;

	uph_resources_remove_track_children(track);
	uph_resources_clear_plugin(&track->instrument);
	naui_list_free(track->blocks);
	naui_list_remove(list, removed_index);

	for (uint32_t i = removed_index; i < (uint32_t)naui_list_len(list); i++)
		list[i].index--;
}

void uph_resources_clear_tracks(void)
{
	uph_resources_clear_tracks_recursive(uph_state.project.tracks);
	uph_state.project.tracks = NULL;
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
	pattern.notes = naui_list_clone(pattern.notes);
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

	naui_list_push(automation.points, point1);
	naui_list_push(automation.points, point2);

	naui_list_push(uph_state.project.automations, automation);
}

void uph_resources_copy_automation(Uph_ResourceIndex automation_index)
{
	Uph_Automation automation = uph_state.project.automations[automation_index];
	automation.points = naui_list_clone(automation.points);
	naui_list_push(uph_state.project.automations, automation);
}

void uph_resources_remove_automation(Uph_ResourceIndex automation_index)
{
	uph_resources_clear_timeline_blocks_with_resource(UPH_RESOURCE_AUTOMATION, automation_index);
	naui_list_remove(uph_state.project.automations, automation_index);
}
#pragma endregion