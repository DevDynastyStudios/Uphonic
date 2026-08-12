typedef struct
{
	Naui_String theme;
	Naui_String language_code;
	Naui_String region_code;
	float ui_scale;
	int32_t autosave_timer;
	uint32_t undo_history_limit;
	bool confirm_on_exit;
	bool confirm_on_delete;
	bool show_tooltips;
} Uph_GeneralSettings;

typedef struct
{
	Naui_String output_device;
	Naui_String input_device;
	uint32_t sample_rate;
	uint32_t buffer_size;
	uint32_t channels;
	float monitoring_gain;
	float vu_meter_hold_time;
	float vu_meter_decay_time;
	bool exclusive_mode;
	bool software_monitoring;
	bool auto_restart_on_device_change;
	bool underrun_protection;
} Uph_AudioSettings;

typedef struct
{
	Naui_Vec2 scroll_sensitivity;
	Naui_Vec2 zoom_sensitivity;
	uint32_t default_snap;
	float playhead_lock_position;
	bool follow_playhead;
} Uph_UISettings;

typedef struct
{
	Naui_String midi_input_device_id;
	uint32_t record_quantize_grid;
	uint32_t count_in_bars;
	uint32_t grid_division;
	uint8_t default_velocity;
	uint8_t default_channel;
	bool filter_by_channel;
	bool midi_thru_enabled;
	bool record_over_dub;
	bool record_quantize;
} Uph_MIDISettings;

typedef struct
{
	Naui_List(Naui_Path) plugin_paths;
	Naui_List(Naui_Path) blacklist_paths;
	bool sandbox_plugins;
	// Box, you can fill in the rest
} Uph_PluginSettings;