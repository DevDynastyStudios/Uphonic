#if NAUI_WINDOWS

Uph_PluginEffect uph_load_plugin_effect(Naui_Path path)
{
    Uph_PluginEffect effect = { 0 };
    effect.file_path = path;
    return effect;
}

void uph_unload_plugin_effect(Uph_PluginEffect effect)
{
    
}

void uph_update_plugin_effect(Uph_PluginEffect *effect)
{
    
}

void uph_process_plugin_effect(Uph_PluginEffect *effect, float **inputs, float **outputs, uint32_t frame_count)
{

}

#endif