static inline float uph_waveform_decode_uint16(
	uint16_t v
);

static inline double uph_beats_to_seconds(
	double beats,
	float bpm
);

static inline double uph_seconds_to_beats(
	double seconds,
	float bpm
);

ma_context *uph_audio_engine_get_context(void);
void uph_audio_engine_stop(void);
void uph_audio_engine_start(void);
void uph_audio_engine_lock(void);
void uph_audio_engine_unlock(void);
bool uph_audio_engine_device_running(void);

Uph_SampleData uph_audio_engine_load_sample_data(
	Naui_Path path
);

void uph_audio_engine_unload_sample_data(
	Uph_SampleData *data
);

bool uph_audio_engine_sample_data_valid(
	const Uph_SampleData *data
);

float uph_sample_gain(
	const Uph_Sample *sample,
	const Uph_SampleData *data
);

double uph_sample_effective_time_scale(
	const Uph_Sample *sample,
	double bpm
);

double uph_sample_block_source_seconds(
	const Uph_Sample *sample,
	double block_start_beat,
	double start_offset_beats,
	double beat
);

double uph_sample_block_length_beats(
	const Uph_Sample *sample,
	const Uph_SampleData *data,
	double start_beat
);

void uph_audio_engine_stop_all_notes(void);
double uph_audio_engine_get_song_length_beats(void);
double uph_audio_engine_get_song_length(void);

bool uph_audio_engine_export_to_wav(
	const char *filepath,
	double start_beat,
	double end_beat
);

void uph_audio_engine_render_cancel(void);
