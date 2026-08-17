#pragma once

void uph_audio_engine_init(void);
void uph_audio_engine_shutdown(void);

Uph_Sample uph_audio_engine_load_sample(Naui_Path path);
void uph_audio_engine_unload_sample(Uph_Sample *sample);

double uph_audio_engine_get_song_length_beats(void);
double uph_audio_engine_get_song_length_seconds(void);
bool uph_audio_engine_export_to_wav(const char *filepath, double start_beat, double end_beat);