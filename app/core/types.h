#define UPH_VERSION (Uph_Version) { .major = 0, .minor = 0, .patch = 1 };
#define UPH_SAMPLE_FRAME_COUNT 512

// :settings
typedef struct Uph_GeneralSettings
{
	Naui_String theme;
	Naui_String language_code;
	Naui_String region_code;
	float ui_scale;
	int32_t autosave_timer;
	uint32_t undo_history_limit;
	bool confirm_on_exit;
	bool confirm_on_delete;
	bool copy_resources;
}
Uph_GeneralSettings;

typedef struct Uph_AudioSettings
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

typedef struct Uph_UISettings
{
	Naui_Vec2 scroll_sensitivity;
	Naui_Vec2 zoom_sensitivity;
	uint32_t default_snap;
	float playhead_lock_position;
	bool follow_playhead;
	bool show_tooltips;
}
Uph_UISettings;

typedef struct Uph_MIDISettings
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

typedef struct Uph_PluginSettings
{
	Naui_List(Naui_Path) plugin_paths;
	// Naui_List(Naui_Path) blacklist_paths; this is useless bro
	bool sandbox_plugins;
	// Box, you can fill in the rest
}
Uph_PluginSettings;

typedef uint8_t Uph_SaveType;
enum
{
	UPH_SAVE_TYPE_CANONICAL,
	UPH_SAVE_TYPE_TEMP
};

typedef uint8_t Uph_ExportFormat;
enum
{
	UPH_EXPORT_UPH,
	UPH_EXPORT_WAV,
	UPH_EXPORT_MP3,
	UPH_EXPORT_OGG,
	UPH_EXPORT_FLAC,
	UPH_EXPORT_MIDI
};

typedef uint32_t Uph_ResourceIndex;

// :project
typedef uint8_t Uph_ResourceType;
enum
{
	UPH_RESOURCE_NONE,
	UPH_RESOURCE_SAMPLE,
	UPH_RESOURCE_PATTERN,
	UPH_RESOURCE_AUTOMATION
};

typedef struct
{
	double start_beat;
	double start_offset_beats;
	double length_beats;
	float visual_lifetime;
    Uph_ResourceIndex resource_index;
    Uph_ResourceType type;
}
Uph_TimelineBlock;

typedef uint8_t Uph_SampleChannelType;
enum
{
	UPH_SAMPLE_MONO,
	UPH_SAMPLE_STEREO
};

typedef struct
{
    uint16_t min;
    uint16_t max;
}
Uph_WaveformPeak;

typedef struct
{
	Naui_Path file_path;
	Naui_List(Uph_WaveformPeak) waveform_peaks;
	float *frames;
	uint64_t frame_count;
	uint32_t original_sample_rate;
	uint32_t ref_count;
	Uph_SampleChannelType channel_type;
}
Uph_SampleData;

typedef uint8_t Uph_SampleMode;
enum
{
	UPH_SAMPLE_RESAMPLE,
	UPH_SAMPLE_STRETCH
};

typedef struct
{
	Naui_String name;
	double time_scale;
	Uph_ResourceIndex data_index;
	Uph_SampleMode mode;
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
}
Uph_MidiPattern;

typedef uint8_t Uph_TrackState;
enum
{
	UPH_TRACK_MUTED = 1 << 0,
	UPH_TRACK_SOLOED = 1 << 1,
	UPH_TRACK_ARMED = 1 << 2,
	UPH_TRACK_SILENCED = 1 << 3
};

typedef uint8_t Uph_PluginType;
enum
{
	UPH_PLUGIN_CLAP,
	UPH_PLUGIN_VST3
};

typedef struct
{
	Naui_Path file_path;
	void *internal_handle;
	Uph_PluginType type;
	bool loaded;
}
Uph_Plugin;

typedef struct Uph_Track Uph_Track;
struct Uph_Track
{
	Naui_String name;
	Naui_Color color;

	Uph_Plugin instrument;
	Naui_List(Uph_Plugin) effects;

	Naui_List(Uph_TimelineBlock) blocks;
	Naui_List(Uph_Track) subtracks;

	float volume;
	float pan;
	float peak_left, peak_right;
	float smooth_peak_left, smooth_peak_right;
	float glow_effect;

	Uph_ResourceType type;
	Uph_TrackState state;
};

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
	Uph_Version project_version;
	Uph_TimeSignature time_signature;

    Naui_List(Uph_Track) tracks;
    Naui_List(Uph_MidiPattern) midi_patterns;
    Naui_List(Uph_Sample) samples;
    Naui_List(Uph_SampleData) sample_data;

	uint64_t time_created;
	uint64_t last_accessed;
	uint64_t last_modified;
	float bpm;
}
Uph_Project;

typedef struct
{
	Uph_GeneralSettings general;
	Uph_AudioSettings audio;
	Uph_UISettings ui;
	Uph_MIDISettings midi;
	Uph_PluginSettings plugin;
}
Uph_Settings;

typedef uint8_t Uph_ActionMode;
enum
{
	UPH_ACTION_SELECT,
	UPH_ACTION_DRAW,
	UPH_ACTION_CUT
};

typedef struct
{
	double song_timeline_current_block_length;
	double song_timeline_current_block_start_offset;
	
    double song_timeline_playhead_position;
    bool song_timeline_playing;

	bool current_pattern_updated;

	struct
	{
		Uph_ResourceIndex index;
		Uph_ResourceType type;
	}
	selected_resource;
}
Uph_SharedState;

typedef struct
{
	Uph_Project project;
	Uph_Settings settings;
	Uph_SharedState shared;

	uint64_t _last_autosave_time;
	uint64_t _last_modified_time;
	int32_t _modified_counter;
}
Uph_State;

extern Uph_State uph_state;