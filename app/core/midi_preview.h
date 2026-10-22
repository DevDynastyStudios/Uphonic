#pragma once

typedef struct
{
	Uph_ResourceIndex pattern_index;
	double start_beat;
	double end_beat;
	double length;
	bool loop;
}
Uph_MidiPreviewRange;

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
bool uph_midi_preview_playback_range(const Uph_Track *track, Uph_MidiPreviewRange *range);
void uph_midi_preview_queue_audition(Uph_Track *track, bool should_be_held[128]);
void uph_midi_preview_end_buffer(void);
