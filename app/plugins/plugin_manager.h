Uph_Plugin          uph_load_plugin             (Naui_Path path);
void                uph_unload_plugin           (Uph_Plugin *plugin);
void                uph_update_plugin           (Uph_Plugin *plugin);

void                uph_hide_plugin_window      (Uph_Plugin *plugin);
void                uph_show_plugin_window      (Uph_Plugin *plugin);

bool                uph_plugin_window_visible   (Uph_Plugin *plugin);

void uph_plugin_queue_note_event(
    Uph_Plugin *effect,
    bool note_on,
    uint8_t key,
    int16_t channel,
    uint8_t velocity,
    uint32_t sample_offset
);

void uph_plugin_queue_stop_all(Uph_Plugin *plugin, uint32_t sample_offset);
bool uph_plugin_note_active(Uph_Plugin *plugin, uint8_t key);

void uph_process_plugin(
    Uph_Plugin *effect,
    float **inputs,
    float **outputs,
    uint32_t frame_count,
    double playhead_beat,
    bool is_playing
);