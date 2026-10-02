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

static bool uph_resources_tree_contains(Naui_List(Uph_Track) list, const Uph_Track *needle)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
	{
		if (&list[i] == needle || uph_resources_tree_contains(list[i].subtracks, needle))
			return true;
	}
	return false;
}

static void uph_resources_push_track_locked(Naui_List(Uph_Track) *list, Uph_Track *parent, Uph_Track track)
{
	const uintptr_t old_base = (uintptr_t)*list;
	const uintptr_t old_end = old_base + (uintptr_t)naui_list_len(*list) * sizeof(Uph_Track);
	const uintptr_t selected = (uintptr_t)uph_state.shared.selected_mixer_track;

	naui_list_push(*list, track);

	if ((uintptr_t)*list != old_base && selected >= old_base && selected < old_end)
		uph_state.shared.selected_mixer_track = (Uph_Track*)((uintptr_t)*list + (selected - old_base));

	uph_resources_link_track_list(*list, parent);
}

static void uph_resources_detach_plugin_locked(Naui_List(Uph_Plugin) *detached, Uph_Plugin *plugin)
{
	if (!plugin->loaded || !plugin->internal_handle)
		return;

	naui_list_push(*detached, *plugin);

	plugin->loaded = false;
	plugin->internal_handle = NULL;
	plugin->params = NULL;
}

static void uph_resources_detach_plugins_recursive_locked(Naui_List(Uph_Plugin) *detached, Naui_List(Uph_Track) list)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
	{
		Uph_Track *track = &list[i];
		uph_resources_detach_plugins_recursive_locked(detached, track->subtracks);
		uph_resources_detach_plugin_locked(detached, &track->instrument);

		for (uint32_t e = 0; e < (uint32_t)naui_list_len(track->effects); e++)
			uph_resources_detach_plugin_locked(detached, &track->effects[e].plugin);
	}
}

static void uph_resources_unload_detached(Naui_List(Uph_Plugin) detached)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(detached); i++)
		uph_unload_plugin(&detached[i]);

	naui_list_free(detached);
}

static void uph_resources_clear_tracks_recursive(Naui_List(Uph_Track) list);

static void uph_resources_free_track_contents(Uph_Track *track)
{
	uph_resources_clear_tracks_recursive(track->subtracks);
	track->subtracks = NULL;

	uph_unload_plugin(&track->instrument);

	for (uint32_t e = 0; e < (uint32_t)naui_list_len(track->effects); e++)
		uph_unload_plugin(&track->effects[e].plugin);

	naui_list_free(track->effects);
	track->effects = NULL;
	naui_list_free(track->blocks);
	track->blocks = NULL;
}

static void uph_resources_clear_tracks_recursive(Naui_List(Uph_Track) list)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
		uph_resources_free_track_contents(&list[i]);

	naui_list_free(list);
}

static void uph_resources_clear_timeline_blocks_with_resource_locked(Uph_ResourceType track_type, Uph_ResourceIndex resource_index)
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

void uph_resources_unload_all_plugins(void)
{
	Naui_List(Uph_Plugin) detached = NULL;

	uph_audio_engine_lock();
	uph_resources_detach_plugins_recursive_locked(&detached, uph_state.project.tracks);
	uph_audio_engine_unlock();

	uph_resources_unload_detached(detached);
}

void uph_resources_link_tracks(Naui_List(Uph_Track) tracks)
{
	uph_resources_link_track_list(tracks, NULL);
}

void uph_resources_replace_tracks(Uph_Project *project, Naui_List(Uph_Track) tracks)
{
	uph_audio_engine_lock();
	Naui_List(Uph_Track) previous = project->tracks;
	project->tracks = tracks;
	uph_state.shared.selected_mixer_track = NULL;
	uph_audio_engine_unlock();

	uph_resources_clear_tracks_recursive(previous);
}

void uph_resources_add_track(Naui_String name)
{
	Uph_Track track = {
		.name = name,
		.color_index = 0,
		.volume = 1.0f,
		.index = naui_list_len(uph_state.project.tracks)
	};

	uph_audio_engine_lock();
	uph_resources_push_track_locked(&uph_state.project.tracks, NULL, track);
	uph_audio_engine_unlock();
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

	uph_audio_engine_lock();
	uph_resources_push_track_locked(&parent->subtracks, parent, track);
	uph_audio_engine_unlock();
}

void uph_resources_remove_track(Uph_Track *track)
{
	Uph_Track *parent = track->parent;
	Naui_List(Uph_Track) list = parent ? parent->subtracks : uph_state.project.tracks;
	const uint32_t removed_index = track->index;
	const uint32_t old_count = (uint32_t)naui_list_len(list);

	Uph_Track dead;

	uph_audio_engine_lock();

	dead = *track;

	Uph_Track *selected = uph_state.shared.selected_mixer_track;
	if (selected && (selected == track || uph_resources_tree_contains(dead.subtracks, selected)))
		selected = NULL;
	else if (selected && selected >= &list[removed_index + 1] && selected < &list[old_count])
		selected--;

	naui_list_remove(list, removed_index);
	uph_resources_link_track_list(list, parent);
	uph_state.shared.selected_mixer_track = selected;

	uph_audio_engine_unlock();

	uph_resources_free_track_contents(&dead);
}

void uph_resources_clear_tracks(void)
{
	uph_audio_engine_lock();
	Naui_List(Uph_Track) old = uph_state.project.tracks;
	uph_state.project.tracks = NULL;
	uph_state.shared.selected_mixer_track = NULL;
	uph_audio_engine_unlock();

	uph_resources_clear_tracks_recursive(old);
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
	uph_audio_engine_lock();

	Uph_Sample sample = uph_state.project.samples[sample_index];

	uph_resources_clear_timeline_blocks_with_resource_locked(UPH_RESOURCE_SAMPLE, sample_index);

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

	uph_audio_engine_unlock();
}

void uph_resources_add_pattern(void)
{
	Uph_MidiPattern pattern = {
		.name = naui_string_from_cstr(NAUI_TR("pattern.default.name"))
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
	uph_audio_engine_lock();
	uph_resources_clear_timeline_blocks_with_resource_locked(UPH_RESOURCE_PATTERN, pattern_index);
	naui_list_remove(uph_state.project.midi_patterns, pattern_index);
	uph_audio_engine_unlock();
}

void uph_resources_add_automation(void)
{
	Uph_Automation automation = {
		.name = naui_string_from_cstr(NAUI_TR("automation.default.name"))
	};

	const Uph_AutomationPoint point1 = (Uph_AutomationPoint){.beat = 0.0, .value = 0.5f};
	const Uph_AutomationPoint point2 = (Uph_AutomationPoint){.beat = (double)uph_state.project.time_signature.denominator, .value = 0.5f};

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
	uph_audio_engine_lock();
	uph_resources_clear_timeline_blocks_with_resource_locked(UPH_RESOURCE_AUTOMATION, automation_index);
	naui_list_remove(uph_state.project.automations, automation_index);
	uph_audio_engine_unlock();
}

void uph_resources_remove_all_automation(void)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.automations); i++)
		naui_list_free(uph_state.project.automations[i].points);

	naui_list_clear(uph_state.project.automations);
}

#pragma endregion