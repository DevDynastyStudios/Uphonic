typedef struct
{
	Uph_ResourceIndex pattern_index;
	double start_beat;
	double end_beat;
	double length;
	double loop_start;
	double loop_end;
	bool loop;
	bool repeat;
	bool extend;
}
Uph_MidiPreviewRange;

typedef struct
{
	Uph_Track *voice;
	Uph_ActionTrackRef voice_ref;
	bool none_chosen;
	_Atomic bool voice_monitoring;

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
Uph_MidiPreviewState;

static Uph_MidiPreviewState _uph_midi_preview_state = { .loop = true };

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
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	state->none_chosen = track == NULL;
	state->voice_ref = uph_action_track_ref(track);
	state->voice = track;
}

Uph_Track *uph_midi_preview_voice(void)
{
	return _uph_midi_preview_state.voice;
}

void uph_midi_preview_set_voice_monitoring(bool monitoring)
{
	atomic_store_explicit(&_uph_midi_preview_state.voice_monitoring, monitoring, memory_order_relaxed);
}

bool uph_midi_preview_voice_monitoring(void)
{
	return atomic_load_explicit(&_uph_midi_preview_state.voice_monitoring, memory_order_relaxed) && _uph_midi_preview_state.voice;
}

void uph_midi_preview_refresh_voice(void)
{
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	Uph_Track *refreshed = state->voice;

	if (refreshed && !_uph_midi_preview_track_in(uph_state.project.tracks, refreshed))
		refreshed = uph_action_track_resolve(state->voice_ref);

	if (!uph_midi_preview_is_voice_candidate(refreshed))
		refreshed = NULL;

	if (!refreshed && !state->none_chosen)
		refreshed = _uph_midi_preview_first_candidate(uph_state.project.tracks);

	if (refreshed)
		state->voice_ref = uph_action_track_ref(refreshed);

	state->voice = refreshed;
}

#pragma endregion

#pragma region Main thread control

void uph_midi_preview_note_on(uint8_t key, uint8_t velocity)
{
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	if (key >= 128)
		return;

	state->audition_velocity[key] = velocity;
	state->audition_held[key] = true;
}

void uph_midi_preview_note_off(uint8_t key)
{
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	if (key < 128)
		state->audition_held[key] = false;
}

void uph_midi_preview_play(Uph_ResourceIndex pattern_index)
{
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	state->pattern_index = pattern_index;
	state->playing = true;
}

void uph_midi_preview_pause(void)
{
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	state->playing = false;
}

void uph_midi_preview_seek(double beat)
{
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	state->position = fmax(0.0, beat);
}

void uph_midi_preview_stop(void)
{
	uph_midi_preview_pause();
	uph_midi_preview_seek(0.0);
}

void uph_midi_preview_set_loop(bool loop)
{
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	state->loop = loop;
}

bool uph_midi_preview_looping(void)
{
	return _uph_midi_preview_state.loop;
}

bool uph_midi_preview_playing(void)
{
	return _uph_midi_preview_state.playing;
}

double uph_midi_preview_position(void)
{
	return _uph_midi_preview_state.position;
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
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	state->buffer_playing = false;

	if (state->playing && timeline_playing)
		state->playing = false;

	if (!state->playing || bpm <= 0.0f || sample_rate == 0)
		return;

	const Uph_ResourceIndex pattern_index = state->pattern_index;
	if (pattern_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns))
	{
		state->playing = false;
		return;
	}

	const Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[pattern_index];
	const double length = uph_midi_preview_loop_length(pattern);
	if (length <= 0.0)
		return;

	const bool extend = uph_midi_recorder_pattern_armed();
	const bool loop = state->loop && !extend;
	const bool region = loop && uph_loop_region_valid(&pattern->loop_region);
	const double loop_start = region ? pattern->loop_region.start : 0.0;
	const double loop_end = region ? pattern->loop_region.end : length;

	state->buffer_position = state->position;
	double start = fmax(state->buffer_position, 0.0);
	if (region)
	{
		if (start >= loop_end && start >= length)
			start = loop_start;
	}
	else if (start >= length && !extend)
		start = fmod(start, length);

	double end = start + uph_seconds_to_beats((double)frame_count / (double)sample_rate, bpm);
	if (region && start < loop_end)
		end = fmin(end, loop_end);

	state->buffer_range = (Uph_MidiPreviewRange){
		.pattern_index = pattern_index,
		.start_beat = start,
		.end_beat = end,
		.length = length,
		.loop_start = loop_start,
		.loop_end = start < loop_end ? loop_end : length,
		.loop = loop,
		.repeat = loop && !region,
		.extend = extend
	};
	state->buffer_playing = true;
}

bool uph_midi_preview_playback_range(const Uph_Track *track, Uph_MidiPreviewRange *range)
{
	const Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	if (!state->buffer_playing || !track || track != state->voice)
		return false;

	*range = state->buffer_range;
	return true;
}

bool uph_midi_preview_buffer_range(Uph_MidiPreviewRange *range)
{
	const Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	if (!state->buffer_playing)
		return false;

	*range = state->buffer_range;
	return true;
}

void uph_midi_preview_queue_audition(Uph_Track *track, bool should_be_held[128])
{
	const Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	if (!track || track != state->voice)
		return;

	for (uint32_t key = 0; key < 128; key++)
	{
		if (!state->audition_held[key])
			continue;

		should_be_held[key] = true;
		if (!uph_plugin_note_active(&track->instrument, (uint8_t)key))
			uph_plugin_queue_note_event(&track->instrument, true, (uint8_t)key, 0, state->audition_velocity[key], 0);
	}
}

void uph_midi_preview_end_buffer(void)
{
	Uph_MidiPreviewState *state = &_uph_midi_preview_state;
	if (!state->buffer_playing)
		return;

	state->buffer_playing = false;

	const Uph_MidiPreviewRange *range = &state->buffer_range;
	double next = range->end_beat;
	if (next >= range->loop_end && !range->extend)
	{
		if (range->repeat)
			next = fmod(next, range->length);
		else if (range->loop)
			next = range->loop_start;
		else
		{
			next = 0.0;
			state->playing = false;
		}
	}

	if (state->position == state->buffer_position)
		state->position = next;
}

#pragma endregion
