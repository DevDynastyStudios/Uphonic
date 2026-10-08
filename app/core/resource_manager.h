void uph_resources_add_track(Naui_String name);
void uph_resources_add_automation_track(Uph_Track *parent, Naui_String name, Uph_PluginParam *param);

bool uph_plugin_owns_param(const Uph_Plugin *plugin, const Uph_PluginParam *param);
bool uph_resources_param_owner_index(const Uph_Track *track, const Uph_PluginParam *param, int32_t *out_effect_index);
Uph_PluginParam *uph_resources_find_param(Uph_Track *track, int32_t effect_index, uint64_t param_id);
void uph_resources_release_plugin_automation(Uph_Track *track, const Uph_Plugin *plugin);
void uph_resources_refresh_param_usage(Naui_List(Uph_Track) tracks);

void uph_resources_remove_track(Uph_Track *track);
void uph_resources_clear_tracks(void);

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