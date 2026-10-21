#pragma once

void uph_midi_preview_set_voice(Uph_Track *track);
Uph_Track *uph_midi_preview_voice(void);
void uph_midi_preview_refresh_voice(void);
bool uph_midi_preview_is_voice_candidate(const Uph_Track *track);

void uph_midi_preview_note_on(uint8_t key, uint8_t velocity);
void uph_midi_preview_note_off(uint8_t key);

void uph_midi_preview_play(Uph_ResourceIndex pattern_index);
void uph_midi_preview_pause(void);
void uph_midi_preview_stop(void);
void uph_midi_preview_seek(double beat);
void uph_midi_preview_set_loop(bool loop);
bool uph_midi_preview_looping(void);
bool uph_midi_preview_playing(void);
double uph_midi_preview_position(void);
double uph_midi_preview_loop_length(const Uph_MidiPattern *pattern);

void uph_midi_preview_begin_buffer(uint32_t frame_count, uint32_t sample_rate, float bpm, bool timeline_playing);
void uph_midi_preview_render_track(Uph_Track *track);
void uph_midi_preview_end_buffer(void);
bool uph_midi_preview_key_held(const Uph_Track *track, uint8_t key);
