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
	// Add Blocks
	// Add Effects
	float volume;
	float pan;
	float peak_left, peak_right;
	float smooth_peak_left, smooth_peak_right;
	Uph_TrackState state;
} Uph_Track;

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