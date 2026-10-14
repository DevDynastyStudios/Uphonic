Uph_Plugin          uph_plugin_load             (Naui_Path path);
void                uph_plugin_unload           (Uph_Plugin *plugin);
void                uph_plugin_update           (Uph_Plugin *plugin);

void                uph_plugin_hide_window      (Uph_Plugin *plugin);
void                uph_plugin_show_window      (Uph_Plugin *plugin);

bool                uph_plugin_window_visible   (Uph_Plugin *plugin);

void uph_plugin_queue_note_event(
    Uph_Plugin *effect,
    bool note_on,
    uint8_t key,
    int16_t channel,
    uint8_t velocity,
    uint32_t sample_offset
);

void uph_plugin_queue_param_change(
    Uph_Plugin *plug,
    clap_id param_id,
    double value,
    uint32_t sample_offset
);

void uph_plugin_queue_stop_all(Uph_Plugin *plugin, uint32_t sample_offset);
bool uph_plugin_note_active(Uph_Plugin *plugin, uint8_t key);

void uph_plugin_process(
    Uph_Plugin *plug,
    float **inputs,
    float **outputs,
    uint32_t frame_count,
    double playhead_beat,
    bool is_playing
);

bool uph_plugin_save_state(const Uph_Plugin *plug, const Naui_Path path);
bool uph_plugin_load_state(Uph_Plugin *plug, const Naui_Path path);