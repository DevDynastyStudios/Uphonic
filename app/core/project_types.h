typedef struct
{
	double start_beat;
	double start_offset_beats;
	double length_beats;
	double reserved;
	uint16_t pattern_index;
} Uph_MidiTimelineBlock;

typedef struct
{
	double start_beat;
	double start_offset_beats;
	double length_beats;
	double stretched_scale;
	uint16_t sample_index;
} Uph_SampleTimelineBlock;

typedef union
{
	Uph_MidiTimelineBlock midi_block;
	Uph_SampleTimelineBlock sample_block;
} Uph_TimelineBlock;

typedef enum
{
	UPH_SAMPLE_MONO,
	UPH_SAMPLE_STEREO
} Uph_SampleChannelType;

typedef struct
{
	Naui_String name;
	Naui_Path* file_path;
	Uph_SampleChannelType channel_type;
	Naui_Color color;
	float* frame_data;
	uint64_t frame_count;
	uint32_t original_sample_rate;
} Uph_AudioSample;

typedef struct
{
	double start_beat;
	double length_beats;
	uint8_t key_number;
	uint8_t velocity;
} Uph_MidiNote;

typedef struct
{
	Naui_String name;
	Naui_List(Uph_MidiNote) notes;
	Naui_Color color;
} Uph_MidiPattern;

typedef enum
{
	UPH_TRACK_NONE,
	UPH_TRACK_AUDIO,
	UPH_TRACK_MIDI
} Uph_TrackType;

typedef uint8_t Uph_TrackState;
enum {
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
} Uph_Track;

typedef struct
{
	uint32_t numerator;
	uint32_t denominator;
} Uph_TimeSignature;

typedef struct
{
	uint8_t major;
	uint8_t minor;
	uint16_t patch;
} Uph_Version;

typedef struct
{
	Naui_String title;
	uint64_t time_created;
	uint64_t last_accessed;
	uint64_t last_modified;
	Uph_Version project_version;
} Uph_Project;