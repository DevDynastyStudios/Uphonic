#pragma once

#define UPH_WAVEFORM_INV_65535 0.000015259f
#define UPH_WAVEFORM_INV_65535_2 0.000030518f

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

static inline double uph_beats_to_seconds(double beats, float bpm)
{
    return (beats / (double)bpm) * 60.0;
}

static inline double uph_seconds_to_beats(double seconds, float bpm)
{
    return (seconds / 60.0) * (double)bpm;
}

void uph_audio_engine_init(void);
void uph_audio_engine_shutdown(void);

Uph_SampleData uph_audio_engine_load_sample_data(Naui_Path path);
void uph_audio_engine_unload_sample_data(Uph_SampleData *data);

bool uph_audio_engine_sample_data_valid(const Uph_SampleData *data);

double uph_audio_engine_get_song_length_beats(void);
double uph_audio_engine_get_song_length_seconds(void);
bool uph_audio_engine_export_to_wav(const char *filepath, double start_beat, double end_beat);