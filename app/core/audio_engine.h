#pragma once

#define UPH_WAVEFORM_INV_65535 0.000015259f
#define UPH_WAVEFORM_INV_65535_2 0.000030518f
#define UPH_WAVEFORM_BIN_FRAMES 256

static const char* uph_audio_engine_extensions[] = { ".wav", ".mp3", ".flac", ".ogg", NULL };

static inline uint16_t uph_waveform_encode_uint16(float v)
{
    if (v < -1.0f) v = -1.0f;
    if (v >  1.0f) v =  1.0f;
    return (uint16_t)lrintf((v * 0.5f + 0.5f) * 65535.0f);
}

static inline float uph_waveform_decode_uint16(uint16_t v)
{
    return (float)v * UPH_WAVEFORM_INV_65535_2 - 1.0f;
}

static inline double uph_frames_to_seconds(uint64_t frames, uint32_t sample_rate)
{
	return (double)frames / (double)sample_rate;
}

static inline uint64_t uph_seconds_to_frames(double seconds, uint32_t sample_rate)
{
	return (uint64_t)(seconds * (double)sample_rate);
}

static inline double uph_beats_to_seconds(double beats, float bpm)
{
    return (beats / (double)bpm) * 60.0;
}

static inline double uph_seconds_to_beats(double seconds, float bpm)
{
    return (seconds / 60.0) * (double)bpm;
}

static inline double uph_frames_to_beats(uint64_t frames, uint32_t sample_rate, double bpm)
{
	double seconds = uph_frames_to_seconds(frames, sample_rate);
	return uph_seconds_to_beats(seconds, bpm);
}

static inline double uph_beats_to_frames(double beats, uint32_t sample_rate, double bpm)
{
	double seconds = uph_beats_to_seconds(beats, bpm);
	return uph_seconds_to_frames(seconds, sample_rate);
}

void uph_audio_engine_init(void);
void uph_audio_engine_shutdown(void);
bool uph_audio_engine_recreate(void);
Uph_AudioDriver uph_audio_engine_active_driver(void);
uint32_t uph_audio_engine_input_channel_count(void);
void uph_audio_engine_update(void);
ma_context *uph_audio_engine_get_context(void);
bool uph_track_mix_automation_value(const Uph_Track *lanes_owner, Uph_AutomationTarget target, double beat, float *out_value);

void uph_audio_engine_stop(void);
void uph_audio_engine_start(void);

void uph_audio_engine_lock(void);
void uph_audio_engine_unlock(void);

bool uph_audio_engine_device_running(void);

Uph_SampleData uph_audio_engine_load_sample_data(Naui_Path path);
void uph_audio_engine_unload_sample_data(Uph_SampleData *data);

bool uph_audio_engine_sample_data_valid(const Uph_SampleData *data);
float uph_sample_gain(const Uph_Sample *sample, const Uph_SampleData *data);
double uph_sample_effective_time_scale(const Uph_Sample *sample, double bpm);
double uph_sample_block_source_seconds(const Uph_Sample *sample, double block_start_beat, double start_offset_beats, double beat);
double uph_sample_block_length_beats(const Uph_Sample *sample, const Uph_SampleData *data, double start_beat);

void uph_audio_engine_stop_all_notes(void);

typedef struct Uph_PreviewStream Uph_PreviewStream;

typedef struct
{
	Uph_SampleData *data;
	Uph_PreviewStream *stream;
}
Uph_PreviewSource;

typedef struct
{
	const Naui_Path *path;
	const Uph_WaveformPeak *peaks;
	uint64_t peak_count;
	uint64_t peak_total;
	uint64_t frame_count;
	uint32_t original_sample_rate;
	Uph_SampleChannelType channel_type;
	bool streaming;
}
Uph_PreviewInfo;

Uph_PreviewSource uph_audio_engine_preview_open(const Naui_Path path);
bool uph_audio_engine_preview_source_valid(const Uph_PreviewSource *source);
void uph_audio_engine_preview_close(Uph_PreviewSource *source);
void uph_audio_engine_preview_play(Uph_PreviewSource source);
void uph_audio_engine_preview_restart(void);
void uph_audio_engine_preview_stop(void);
bool uph_audio_engine_preview_playing(void);
double uph_audio_engine_preview_progress(void);
bool uph_audio_engine_preview_info(Uph_PreviewInfo *out);

double uph_audio_engine_get_song_length_beats(void);
double uph_audio_engine_get_song_length_seconds(void);
double uph_audio_engine_get_song_length(void);
bool uph_audio_engine_export_to_wav(const char *filepath, double start_beat, double end_beat);
bool uph_audio_engine_render_begin(const char *filepath, const Uph_RenderSettings *settings);
Uph_RenderStatus uph_audio_engine_render_step(double budget_seconds);
float uph_audio_engine_render_progress(void);
bool uph_audio_engine_render_active(void);
void uph_audio_engine_render_cancel(void);
uint32_t uph_audio_engine_sample_rate(void);