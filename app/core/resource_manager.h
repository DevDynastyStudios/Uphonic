void uph_resources_unload_all_plugins(void);
void uph_resources_link_tracks(Naui_List(Uph_Track) tracks);

void uph_resources_clear_tracks_recursive(Naui_List(Uph_Track) list);

void uph_resources_add_track(Naui_String name, int32_t color_index);
void uph_resources_add_automation_track(Uph_Track *parent, Naui_String name, Uph_PluginParam *param);

void uph_resources_init_master_track(void);

static void uph_resources_toggle_track_state(Uph_Track *track, Uph_TrackState flag, const char *action);
static void uph_resources_toggle_master_state(Uph_TrackState flag, const char *action);

bool uph_plugin_owns_param(const Uph_Plugin *plugin, const Uph_PluginParam *param);
bool uph_resources_param_owner_index(const Uph_Track *track, const Uph_PluginParam *param, int32_t *out_effect_index);
Uph_PluginParam *uph_resources_find_param(Uph_Track *track, int32_t effect_index, uint64_t param_id);
void uph_resources_refresh_param_usage(Naui_List(Uph_Track) tracks);

void uph_resources_remove_track(Uph_Track *track);
void uph_resources_clear_tracks(void);

bool uph_resources_mixer_track_selected(const Uph_Track *track);
void uph_resources_clear_mixer_selection(void);
void uph_resources_select_mixer_track(Uph_Track *track);
void uph_resources_toggle_mixer_track(Uph_Track *track);
void uph_resources_select_mixer_track_range(Uph_Track *to);
void uph_resources_select_mixer_master(void);

bool uph_resources_can_group_track(const Uph_Track *track);

uint32_t uph_resources_count_groupable_tracks(Naui_List(Uph_TrackPtr) tracks);
Uph_Track *uph_resources_group_tracks(Naui_List(Uph_TrackPtr) tracks, const char *name, int32_t color_index); // NULL if nothing could be grouped
bool uph_resources_ungroup_track(Uph_Track *group); // children go back to the group's parent where the group was

bool uph_resources_move_track(Uph_Track *track, Uph_Track *dest_group, uint32_t dest_index);

bool uph_resources_move_tracks(Naui_List(Uph_TrackPtr) tracks, Uph_Track *dest_group, uint32_t dest_index);
uint32_t uph_resources_count_movable_tracks(Naui_List(Uph_TrackPtr) tracks); // how many a move would actually carry

Uph_ResourceIndex uph_resources_add_sample_data(Uph_SampleData data, Naui_String name);
bool uph_resources_add_sample_from_file(Naui_Path path);
void uph_resources_copy_sample(Uph_ResourceIndex sample_index);
void uph_resources_remove_sample(Uph_ResourceIndex sample_index);

void uph_resources_add_pattern(void);
void uph_resources_copy_pattern(Uph_ResourceIndex pattern_index);
void uph_resources_remove_pattern(Uph_ResourceIndex pattern_index);

void uph_resources_add_automation(void);
void uph_resources_copy_automation(Uph_ResourceIndex automation_index);
void uph_resources_remove_automation(Uph_ResourceIndex automation_index);
void uph_resources_remove_all_automation(void);

void uph_resources_unload_all_plugins(void);

static inline Naui_Color uph_resources_track_color(const int32_t color_index)
{
	const Naui_List(Naui_Color) list = naui_theme_color_list("uph_track_palette");
	return list[color_index % naui_list_len(list)];
}

static inline bool uph_track_monitors_instrument(const Uph_Track *track)
{
	return track->type == UPH_RESOURCE_PATTERN && track->instrument.loaded && track->monitored;
}

static inline bool uph_track_is_descendant_of(const Uph_Track *track, const Uph_Track *ancestor)
{
	for (const Uph_Track *p = track ? track->parent : NULL; p; p = p->parent)
	{
		if (p == ancestor)
			return true;
	}
	return false;
}

static inline bool uph_track_is_silenced(const Uph_Project *project, const Uph_Track *track)
{
	const Uph_Track *solo = project->soloed_track;
	if (!solo)
		return false;

	return solo != track && !uph_track_is_descendant_of(track, solo) && !uph_track_is_descendant_of(solo, track);
}

static inline bool uph_track_is_audible(const Uph_Project *project, const Uph_Track *track)
{
	for (const Uph_Track *t = track; t; t = t->parent)
	{
		if (t->state & UPH_TRACK_MUTED)
			return false;
	}
	return !uph_track_is_silenced(project, track);
}