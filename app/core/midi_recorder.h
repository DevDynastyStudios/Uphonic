bool uph_midi_recorder_key_held(
	uint8_t key
);

uint8_t uph_midi_recorder_pedal_value(
	Uph_MidiPedalType type
);

void uph_midi_recorder_process(
	double buffer_start_beat,
	uint32_t frame_count,
	uint32_t sample_rate,
	float bpm,
	bool playing
);

void uph_midi_recorder_finish_take(
	Uph_Track *track
);
