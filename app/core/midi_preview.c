typedef struct
{
	Uph_Track *voice;
	Uph_ActionTrackRef voice_ref;
	bool none_chosen;

	bool playing;
	bool loop;
	Uph_ResourceIndex pattern_index;
	double position;

	bool audition_held[128];
	uint8_t audition_velocity[128];

	bool buffer_playing;
	double buffer_position;
	Uph_MidiPreviewRange buffer_range;
}
Uph_MidiPreviewData;

static Uph_MidiPreviewData uph_midi_preview_data = { .loop = true };

#pragma region Voice

bool uph_midi_preview_is_voice_candidate(const Uph_Track *track)
{
	return track && track->type == UPH_RESOURCE_PATTERN && track->instrument.loaded;
}

static bool _uph_midi_preview_track_in(Naui_List(Uph_Track) tracks, const Uph_Track *track)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		if (&tracks[i] == track || _uph_midi_preview_track_in(tracks[i].subtracks, track))
			return true;
	}

	return false;
}

static Uph_Track *_uph_midi_preview_first_candidate(Naui_List(Uph_Track) tracks)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		if (uph_midi_preview_is_voice_candidate(&tracks[i]))
			return &tracks[i];

		Uph_Track *found = _uph_midi_preview_first_candidate(tracks[i].subtracks);
		if (found)
			return found;
	}

	return NULL;
}

void uph_midi_preview_set_voice(Uph_Track *track)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	p->none_chosen = track == NULL;
	p->voice_ref = uph_action_track_ref(track);
	p->voice = track;
}

Uph_Track *uph_midi_preview_voice(void)
{
	return uph_midi_preview_data.voice;
}

void uph_midi_preview_refresh_voice(void)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	Uph_Track *refreshed = p->voice;

	if (refreshed && !_uph_midi_preview_track_in(uph_state.project.tracks, refreshed))
		refreshed = uph_action_track_resolve(p->voice_ref);

	if (!uph_midi_preview_is_voice_candidate(refreshed))
		refreshed = NULL;

	if (!refreshed && !p->none_chosen)
		refreshed = _uph_midi_preview_first_candidate(uph_state.project.tracks);

	if (refreshed)
		p->voice_ref = uph_action_track_ref(refreshed);

	p->voice = refreshed;
}

#pragma endregion

#pragma region Main thread control

void uph_midi_preview_note_on(uint8_t key, uint8_t velocity)
{
	if (key >= 128)
		return;

	uph_midi_preview_data.audition_velocity[key] = velocity;
	uph_midi_preview_data.audition_held[key] = true;
}

void uph_midi_preview_note_off(uint8_t key)
{
	if (key < 128)
		uph_midi_preview_data.audition_held[key] = false;
}

void uph_midi_preview_play(Uph_ResourceIndex pattern_index)
{
	uph_midi_preview_data.pattern_index = pattern_index;
	uph_midi_preview_data.playing = true;
}

void uph_midi_preview_pause(void)
{
	uph_midi_preview_data.playing = false;
}

void uph_midi_preview_stop(void)
{
	uph_midi_preview_pause();
	uph_midi_preview_seek(0.0);
}

void uph_midi_preview_seek(double beat)
{
	uph_midi_preview_data.position = fmax(0.0, beat);
}

void uph_midi_preview_set_loop(bool loop)
{
	uph_midi_preview_data.loop = loop;
}

bool uph_midi_preview_looping(void)
{
	return uph_midi_preview_data.loop;
}

bool uph_midi_preview_playing(void)
{
	return uph_midi_preview_data.playing;
}

double uph_midi_preview_position(void)
{
	return uph_midi_preview_data.position;
}

double uph_midi_preview_loop_length(const Uph_MidiPattern *pattern)
{
	const double bar = uph_time_signature_bar_length();
	return ceil(uph_calculate_pattern_length(pattern) / bar - 1e-9) * bar;
}

#pragma endregion

#pragma region Audio thread

void uph_midi_preview_begin_buffer(uint32_t frame_count, uint32_t sample_rate, float bpm, bool timeline_playing)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	p->buffer_playing = false;

	if (p->playing && timeline_playing)
		p->playing = false;

	if (!p->playing || bpm <= 0.0f || sample_rate == 0)
		return;

	const Uph_ResourceIndex pattern_index = p->pattern_index;
	if (pattern_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
	{
		p->playing = false;
		return;
	}

	const double length = uph_midi_preview_loop_length(&uph_state.project.midi_patterns[pattern_index]);
	if (length <= 0.0)
		return;

	p->buffer_position = p->position;
	double start = p->buffer_position;
	if (start >= length || start < 0.0)
		start = fmod(fmax(start, 0.0), length);

	p->buffer_range = (Uph_MidiPreviewRange){
		.pattern_index = pattern_index,
		.start_beat = start,
		.end_beat = start + uph_seconds_to_beats((double)frame_count / (double)sample_rate, bpm),
		.length = length,
		.loop = p->loop
	};
	p->buffer_playing = true;
}

bool uph_midi_preview_playback_range(const Uph_Track *track, Uph_MidiPreviewRange *range)
{
	const Uph_MidiPreviewData *p = &uph_midi_preview_data;
	if (!p->buffer_playing || !track || track != p->voice)
		return false;

	*range = p->buffer_range;
	return true;
}

void uph_midi_preview_queue_audition(Uph_Track *track, bool should_be_held[128])
{
	const Uph_MidiPreviewData *p = &uph_midi_preview_data;
	if (!track || track != p->voice)
		return;

	for (uint32_t key = 0; key < 128; key++)
	{
		if (!p->audition_held[key])
			continue;

		should_be_held[key] = true;
		if (!uph_plugin_note_active(&track->instrument, (uint8_t)key))
			uph_plugin_queue_note_event(&track->instrument, true, (uint8_t)key, 0, p->audition_velocity[key], 0);
	}
}

void uph_midi_preview_end_buffer(void)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	if (!p->buffer_playing)
		return;

	p->buffer_playing = false;

	const Uph_MidiPreviewRange *range = &p->buffer_range;
	double next = range->end_beat;
	if (next >= range->length)
	{
		if (range->loop)
			next = fmod(next, range->length);
		else
		{
			next = 0.0;
			p->playing = false;
		}
	}

	if (p->position == p->buffer_position)
		p->position = next;
}

#pragma endregion
