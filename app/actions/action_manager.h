#define UPH_ACTION_TRACK_MAX_DEPTH 16

typedef struct
{
	uint32_t path[UPH_ACTION_TRACK_MAX_DEPTH];
	uint8_t depth;
} Uph_ActionTrackRef;

typedef struct
{
	float time_scale;
	float tempo_bpm;
	float gain;
	Uph_LoopRegion loop_region;
	int32_t pitch;
	Uph_SampleMode mode;
	bool follow_tempo;
	bool normalized;
	bool reversed;
} Uph_ActionSampleSettings;

static inline Uph_ActionTrackRef uph_action_track_ref_root(void);

static inline bool uph_action_track_ref_is_master(
	Uph_ActionTrackRef ref
);

static inline bool uph_action_track_ref_valid(
	Uph_ActionTrackRef ref
);

static inline uint32_t uph_action_track_ref_index(
	Uph_ActionTrackRef ref
);

static inline Uph_ActionTrackRef uph_action_track_ref_child(
	Uph_ActionTrackRef owner,
	uint32_t index
);

static inline Uph_ActionTrackRef uph_action_track_ref_top(
	uint32_t index
);

static inline Uph_ActionTrackRef uph_action_track_ref_owner(
	Uph_ActionTrackRef ref
);

static inline bool uph_action_track_ref_equal(
	Uph_ActionTrackRef a,
	Uph_ActionTrackRef b
);

static inline Uph_ActionTrackRef uph_action_track_ref_after_insert(
	Uph_ActionTrackRef ref,
	Uph_ActionTrackRef inserted
);

static inline Uph_ActionTrackRef uph_action_track_ref_after_remove(
	Uph_ActionTrackRef ref,
	Uph_ActionTrackRef removed
);

static inline Uph_ActionTrackRef uph_action_track_ref_after_move(
	Uph_ActionTrackRef ref,
	Uph_ActionTrackRef src,
	Uph_ActionTrackRef dst
);

static inline void uph_action_sample_settings_apply(
	Uph_Sample *sample,
	const Uph_ActionSampleSettings *settings
);

Uph_Track *uph_action_track_resolve(
	Uph_ActionTrackRef ref
);

Uph_Track *uph_action_track_resolve_in(
	Naui_List(Uph_Track) tracks,
	Uph_ActionTrackRef ref
);

void uph_action_refresh_block_defaults(void);

void uph_action_resource_removed(
	Uph_ResourceType type,
	Uph_ResourceIndex index
);

void uph_action_resource_restored(
	Uph_ResourceType type,
	Uph_ResourceIndex index
);
