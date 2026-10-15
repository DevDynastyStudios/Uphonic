#pragma once

void uph_midi_recorder_input(bool note_on, uint8_t key, uint8_t velocity);
bool uph_midi_recorder_key_held(uint8_t key);
void uph_midi_recorder_process(double buffer_start_beat, uint32_t frame_count, uint32_t sample_rate, float bpm, bool playing);
void uph_midi_recorder_finish_take(Uph_Track *track);