#if NAUI_WINDOWS

Uph_PluginEffect uph_load_plugin_effect(Naui_Path path)
{
    Uph_PluginEffect effect = { 0 };
    effect.file_path = path;
    return effect;
}

void uph_unload_plugin_effect(Uph_PluginEffect *effect)
{
    
}

void uph_update_plugin_effect(Uph_PluginEffect *effect)
{
    
}

void uph_hide_plugin_window(Uph_PluginEffect *effect)
{

}

void uph_show_plugin_window(Uph_PluginEffect *effect)
{

}

void uph_process_plugin_effect(Uph_PluginEffect *effect, float **inputs, float **outputs, uint32_t frame_count, double playhead_beat, bool is_playing)
{

}

bool uph_plugin_window_visible(Uph_PluginEffect *effect)
{
	return false;
}

void uph_plugin_queue_note_event(Uph_PluginEffect *effect, bool note_on, uint8_t key, int16_t channel, uint8_t velocity, uint32_t sample_offset)
{

}

void uph_plugin_queue_stop_all(Uph_PluginEffect *effect, uint32_t sample_offset)
{

}

bool uph_plugin_note_active(Uph_PluginEffect *effect, uint8_t key)
{
	return false;
}

#endif