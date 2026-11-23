void uph_resources_link_tracks(
	Naui_List(Uph_Track) tracks
);

void uph_resources_clear_tracks_recursive(
	Naui_List(Uph_Track) list
);

int32_t uph_resources_random_track_color_index(void);

void uph_resources_add_track(
	Naui_String name,
	int32_t color_index
);

void uph_resources_add_automation_track(
	Uph_Track *parent,
	Naui_String name,
	Uph_AutomationTarget target,
	Uph_PluginParam *param,
	int32_t color_index
);

bool uph_resources_track_has_automation_target(
	const Uph_Track *track,
	Uph_AutomationTarget target
);

Uph_Track *uph_resources_first_recordable_track(
	Naui_List(Uph_Track) tracks
);

Uph_Track uph_resources_make_master_track(void);
Uph_Track *uph_resources_master_track(void);
void uph_resources_insert_master_track(void);

bool uph_plugin_owns_param(
	const Uph_Plugin *plugin,
	const Uph_PluginParam *param
);

bool uph_resources_param_owner_index(
	const Uph_Track *track,
	const Uph_PluginParam *param,
	int32_t *out_effect_index
);

Uph_PluginParam *uph_resources_find_param(
	Uph_Track *track,
	int32_t effect_index,
	uint64_t param_id
);

Uph_Plugin *uph_resources_plugin_at(
	Uph_Track *track,
	int32_t effect_index
);

bool uph_resources_pending_owner_index(
	const Uph_Track *track,
	uint64_t load_ticket,
	int32_t *out_effect_index
);

bool uph_resources_lane_targets_plugin(
	const Uph_Track *lane,
	const Uph_Plugin *plugin
);

void uph_resources_link_pending_automation(
	Uph_Track *track,
	uint64_t load_ticket,
	Uph_Plugin *plugin
);

Uph_PluginParam *uph_resources_last_touched_param(
	Uph_Track *track,
	int32_t *out_effect_index
);

void uph_resources_refresh_param_usage(
	Naui_List(Uph_Track) tracks
);

void uph_resources_remove_track(
	Uph_Track *track
);

void uph_resources_clear_tracks(void);

Uph_ResourceIndex uph_resources_add_sample_data(
	Uph_SampleData data,
	Naui_String name
);

bool uph_resources_add_sample_from_file(
	Naui_Path path
);

void uph_resources_copy_sample(
	Uph_ResourceIndex sample_index
);

void uph_resources_add_pattern(void);

void uph_resources_copy_pattern(
	Uph_ResourceIndex pattern_index
);

void uph_resources_add_automation(void);

void uph_resources_copy_automation(
	Uph_ResourceIndex automation_index
);

void uph_resources_remove_all_automation(void);
