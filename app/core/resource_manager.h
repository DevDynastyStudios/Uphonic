void uph_resources_link_track_list(Naui_List(Uph_Track) tracks, Uph_Track *parent);
void uph_resources_link_tracks(Naui_List(Uph_Track) tracks);
void uph_resources_clear_plugin(Uph_Plugin *plugin);

void uph_resources_add_track(Naui_String name);
void uph_resources_add_automation_track(Uph_Track *parent, Naui_String name, int32_t effect_index, uint64_t param_id);

void uph_resources_remove_track(Uph_Track *track);
void uph_resources_clear_tracks_recursive(Naui_List(Uph_Track) list);
void uph_resources_clear_tracks(void);

bool uph_resources_add_sample_from_file(Naui_Path path);
void uph_resources_copy_sample(Uph_ResourceIndex sample_index);
void uph_resources_remove_sample(Uph_ResourceIndex sample_index);

void uph_resources_add_pattern(void);
void uph_resources_copy_pattern(Uph_ResourceIndex pattern_index);
void uph_resources_remove_pattern(Uph_ResourceIndex pattern_index);

void uph_resources_add_automation(void);
void uph_resources_copy_automation(Uph_ResourceIndex automation_index);
void uph_resources_remove_automation(Uph_ResourceIndex automation_index);