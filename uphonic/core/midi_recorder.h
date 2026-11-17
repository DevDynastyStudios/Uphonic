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

void uph_midi_recorder_process_pattern(
	uint32_t frame_count,
	uint32_t sample_rate,
	float bpm
);

bool uph_midi_recorder_pattern_armed(void);

bool uph_midi_recorder_note_recording(
	uint32_t pattern_index,
	uint32_t note_index
);

void uph_midi_recorder_set_pattern_armed(
	bool armed
);

void uph_midi_recorder_finish_take(
	Uph_Track *track
);
