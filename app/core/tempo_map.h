#pragma once

#define UPH_TEMPO_MIN_BPM 20.0f
#define UPH_TEMPO_MAX_BPM 500.0f

Uph_Track *uph_tempo_find_lane(void);
bool uph_tempo_lane_active(void);

void uph_tempo_map_update(void);
void uph_tempo_map_shutdown(void);

void uph_tempo_map_audio_begin(void);
void uph_tempo_map_audio_end(void);

float uph_tempo_bpm_at(double beat);
double uph_tempo_beat_to_seconds(double beat);
double uph_tempo_seconds_to_beat(double seconds);
double uph_tempo_beat_after_seconds(double beat, double seconds);

void uph_tempo_set_current_bpm(float bpm);
float uph_tempo_current_bpm(void);
