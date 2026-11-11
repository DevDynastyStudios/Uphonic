#pragma once

void uph_sample_preview_play(Uph_ResourceIndex sample_index);
void uph_sample_preview_pause(void);
void uph_sample_preview_stop(void);
void uph_sample_preview_seek(double beat);
void uph_sample_preview_set_loop(bool loop);
bool uph_sample_preview_looping(void);
bool uph_sample_preview_playing(void);
double uph_sample_preview_position(void);
double uph_sample_preview_length(const Uph_Sample *sample, const Uph_SampleData *data);

void uph_sample_preview_mix(float *out, uint32_t frame_count, uint32_t sample_rate, float bpm, bool timeline_playing);
