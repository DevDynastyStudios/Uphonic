typedef struct
{
	uint32_t scanned;
	uint32_t total;
	bool active;
}
Uph_PluginScanProgress;

// Reads the saved list. Returns false if there is none or it can't be read.
bool uph_plugin_cache_read(Naui_List(Uph_PluginInfo) *info_list);
void uph_plugin_scan_start(const Naui_List(Uph_PluginInfo) known);

// Call from the main thread every frame.
bool uph_plugin_scan_poll(Naui_List(Uph_PluginInfo) *info_list);

Uph_PluginScanProgress uph_plugin_scan_progress(void);

// Call before naui_jobs_shutdown
void uph_plugin_scan_shutdown(void);
