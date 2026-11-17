void uph_sample_preview_seek(
	double beat
);

void uph_sample_preview_mix(
	float *out,
	uint32_t frame_count,
	uint32_t sample_rate,
	float bpm,
	bool timeline_playing
);
