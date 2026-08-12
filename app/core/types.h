// :settings
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
}
Uph_GeneralSettings;

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
}
Uph_AudioSettings;

typedef struct
{
	Naui_Vec2 scroll_sensitivity;
	Naui_Vec2 zoom_sensitivity;
	uint32_t default_snap;
	float playhead_lock_position;
	bool follow_playhead;
}
Uph_UISettings;

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
}
Uph_MIDISettings;

typedef struct
{
	Naui_List(Naui_Path) plugin_paths;
	Naui_List(Naui_Path) blacklist_paths;
	bool sandbox_plugins;
	// Box, you can fill in the rest
}
Uph_PluginSettings;

// :project
typedef struct
{
	double start_beat;
	double start_offset_beats;
	double length_beats;
	double reserved;
	uint16_t pattern_index;
}
Uph_MidiTimelineBlock;

typedef struct
{
	double start_beat;
	double start_offset_beats;
	double length_beats;
	double stretched_scale;
	uint16_t sample_index;
}
Uph_SampleTimelineBlock;

typedef union
{
	Uph_MidiTimelineBlock midi_block;
	Uph_SampleTimelineBlock sample_block;
}
Uph_TimelineBlock;

typedef uint8_t Uph_SampleChannelType;
enum
{
	UPH_SAMPLE_MONO,
	UPH_SAMPLE_STEREO
};

typedef uint32_t Uph_ResoureceID;

typedef struct
{
	Naui_Path file_path;
	float *frames;
	uint64_t frame_count;
	uint32_t original_sample_rate;
	Uph_SampleChannelType channel_type;
}
Uph_SampleData;

typedef struct
{
	Naui_String name;
	Naui_Color color;
	Uph_ResoureceID data_id;
}
Uph_Sample;

typedef struct
{
	double start_beat;
	double length_beats;
	uint8_t key_number;
	uint8_t velocity;
}
Uph_MidiNote;

typedef struct
{
	Naui_String name;
	Naui_List(Uph_MidiNote) notes;
	Naui_Color color;
}
Uph_MidiPattern;

typedef enum
{
	UPH_TRACK_NONE,
	UPH_TRACK_AUDIO,
	UPH_TRACK_MIDI
}
Uph_TrackType;

typedef uint8_t Uph_TrackState;
enum
{
	UPH_TRACK_STATE_NONE = 1 << 0,
	UPH_TRACK_STATE_MUTE = 1 << 1,
	UPH_TRACK_STATE_SOLO = 1 << 2,
	UPH_TRACK_STATE_ARMED = 1 << 3
};

typedef struct
{
	Naui_Color color;
	Uph_TrackType type;
	Naui_List(Uph_TimelineBlock) blocks;
	// Add Effects
	float volume;
	float pan;
	float peak_left, peak_right;
	float smooth_peak_left, smooth_peak_right;
	Uph_TrackState state;
}
Uph_Track;

typedef struct
{
	uint32_t numerator;
	uint32_t denominator;
}
Uph_TimeSignature;

typedef struct
{
	uint8_t major;
	uint8_t minor;
	uint16_t patch;
}
Uph_Version;

typedef struct
{
	Naui_String title;
	uint64_t time_created;
	uint64_t last_accessed;
	uint64_t last_modified;
	Uph_Version project_version;
}
Uph_Project;