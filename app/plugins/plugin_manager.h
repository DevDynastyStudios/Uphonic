bool uph_plugin_format_from_path(
	Naui_Path path,
	Uph_PluginFormat *format
);

Uph_Plugin uph_plugin_load(
	Naui_Path path,
	bool show_window
);

void uph_plugin_show_window(
	Uph_Plugin *plugin
);

void uph_plugin_queue_midi_cc(
	Uph_Plugin *plugin,
	uint8_t channel,
	uint8_t cc,
	uint8_t value,
	uint32_t sample_offset
);

uint8_t uph_plugin_cc_value(
	Uph_Plugin *plugin,
	uint8_t cc
);

bool uph_plugin_take_touched_param(
	Uph_Plugin *plugin,
	uint64_t *out_param_id
);

bool uph_plugin_save_state(
	const Uph_Plugin *plug,
	const Naui_Path path
);

bool uph_plugin_load_state(
	Uph_Plugin *plug,
	const Naui_Path path
);
