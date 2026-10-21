#define UPH_MIDI_PREVIEW_RING_CAPACITY 256
#define UPH_MIDI_PREVIEW_MAX_EVENTS 512
#define UPH_MIDI_PREVIEW_MAX_SEGMENTS 8

_Static_assert((UPH_MIDI_PREVIEW_RING_CAPACITY & (UPH_MIDI_PREVIEW_RING_CAPACITY - 1)) == 0, "must be power of two");

typedef struct
{
	uint32_t offset;
	uint8_t key;
	uint8_t velocity;
	bool note_on;
	bool playback;
}
Uph_MidiPreviewEvent;

typedef struct
{
	Uph_MidiPreviewEvent buffer[UPH_MIDI_PREVIEW_RING_CAPACITY];
	_Atomic uint32_t head;
	_Atomic uint32_t tail;
}
Uph_MidiPreviewRing;

typedef struct
{
	Uph_MidiPreviewRing ring;
	_Atomic(Uph_Track*) voice;
	_Atomic bool playing;
	_Atomic uint32_t pattern_index;
	_Atomic uint32_t restart_counter;
	_Atomic uint32_t seek_counter;
	_Atomic double seek_position;
	_Atomic double position;
	_Atomic bool loop;

	Uph_ActionTrackRef voice_ref;
	bool none_chosen;

	Uph_MidiPreviewEvent events[UPH_MIDI_PREVIEW_MAX_EVENTS];
	uint32_t event_count;
	uint32_t frame_count;
	bool buffer_active;

	Uph_Track *active_voice;
	bool audition_held[128];
	bool playback_held[128];
	bool playback_logical[128];

	Uph_Track *release_voice;
	bool release_held[128];

	uint32_t seen_restart;
	uint32_t seen_seek;
	bool was_playing;
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
	atomic_store_explicit(&p->voice, track, memory_order_release);
}

Uph_Track *uph_midi_preview_voice(void)
{
	return atomic_load_explicit(&uph_midi_preview_data.voice, memory_order_acquire);
}

void uph_midi_preview_refresh_voice(void)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	Uph_Track *voice = uph_midi_preview_voice();
	Uph_Track *refreshed = voice;

	if (refreshed && !_uph_midi_preview_track_in(uph_state.project.tracks, refreshed))
		refreshed = uph_action_track_resolve(p->voice_ref);

	if (!uph_midi_preview_is_voice_candidate(refreshed))
		refreshed = NULL;

	if (!refreshed && !p->none_chosen)
		refreshed = _uph_midi_preview_first_candidate(uph_state.project.tracks);

	if (refreshed)
		p->voice_ref = uph_action_track_ref(refreshed);

	if (refreshed != voice)
		atomic_store_explicit(&p->voice, refreshed, memory_order_release);
}

#pragma endregion

#pragma region Main thread control

static void _uph_midi_preview_push(uint8_t key, uint8_t velocity, bool note_on)
{
	Uph_MidiPreviewRing *ring = &uph_midi_preview_data.ring;
	const uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
	const uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
	if ((uint32_t)(tail - head) >= UPH_MIDI_PREVIEW_RING_CAPACITY)
		return;

	ring->buffer[tail & (UPH_MIDI_PREVIEW_RING_CAPACITY - 1)] = (Uph_MidiPreviewEvent){
		.key = key,
		.velocity = velocity,
		.note_on = note_on
	};
	atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
}

void uph_midi_preview_note_on(uint8_t key, uint8_t velocity)
{
	if (key < 128)
		_uph_midi_preview_push(key, velocity, true);
}

void uph_midi_preview_note_off(uint8_t key)
{
	if (key < 128)
		_uph_midi_preview_push(key, 0, false);
}

void uph_midi_preview_play(Uph_ResourceIndex pattern_index)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	atomic_store_explicit(&p->pattern_index, pattern_index, memory_order_relaxed);
	atomic_fetch_add_explicit(&p->restart_counter, 1, memory_order_relaxed);
	atomic_store_explicit(&p->playing, true, memory_order_release);
}

void uph_midi_preview_pause(void)
{
	atomic_store_explicit(&uph_midi_preview_data.playing, false, memory_order_release);
}

void uph_midi_preview_stop(void)
{
	uph_midi_preview_pause();
	uph_midi_preview_seek(0.0);
}

void uph_midi_preview_seek(double beat)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	beat = fmax(0.0, beat);
	atomic_store_explicit(&p->seek_position, beat, memory_order_relaxed);
	atomic_fetch_add_explicit(&p->seek_counter, 1, memory_order_release);

	if (!uph_midi_preview_playing())
		atomic_store_explicit(&p->position, beat, memory_order_relaxed);
}

void uph_midi_preview_set_loop(bool loop)
{
	atomic_store_explicit(&uph_midi_preview_data.loop, loop, memory_order_relaxed);
}

bool uph_midi_preview_looping(void)
{
	return atomic_load_explicit(&uph_midi_preview_data.loop, memory_order_relaxed);
}

bool uph_midi_preview_playing(void)
{
	return atomic_load_explicit(&uph_midi_preview_data.playing, memory_order_acquire);
}

double uph_midi_preview_position(void)
{
	return atomic_load_explicit(&uph_midi_preview_data.position, memory_order_relaxed);
}

double uph_midi_preview_loop_length(const Uph_MidiPattern *pattern)
{
	const double bar = (double)NAUI_MAX(uph_state.project.time_signature.denominator, 1u);
	return ceil(uph_calculate_pattern_length(pattern) / bar - 1e-9) * bar;
}

#pragma endregion

#pragma region Audio thread

static void _uph_midi_preview_add_event(Uph_MidiPreviewEvent event)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	if (p->event_count >= UPH_MIDI_PREVIEW_MAX_EVENTS)
		return;

	uint32_t i = p->event_count++;
	while (i > 0)
	{
		const Uph_MidiPreviewEvent *prev = &p->events[i - 1];
		const bool after = prev->offset < event.offset || (prev->offset == event.offset && (!prev->note_on || event.note_on));
		if (after)
			break;

		p->events[i] = *prev;
		i--;
	}

	p->events[i] = event;
}

static uint32_t _uph_midi_preview_offset(double beats, uint32_t sample_rate, float bpm)
{
	const Uph_MidiPreviewData *p = &uph_midi_preview_data;
	const uint32_t offset = (uint32_t)(uph_beats_to_seconds(beats, bpm) * (double)sample_rate);
	return p->frame_count > 0 && offset >= p->frame_count ? p->frame_count - 1 : offset;
}

static void _uph_midi_preview_release_logical(uint32_t offset)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	for (uint32_t key = 0; key < 128; key++)
	{
		if (!p->playback_logical[key])
			continue;

		p->playback_logical[key] = false;
		_uph_midi_preview_add_event((Uph_MidiPreviewEvent){ .offset = offset, .key = (uint8_t)key, .playback = true });
	}
}

static void _uph_midi_preview_queue_segment(const Uph_MidiPattern *pattern, double seg_start, double seg_end, double beat_base, uint32_t sample_rate, float bpm)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;

	for (uint32_t n = 0; n < (uint32_t)naui_list_len(pattern->notes); n++)
	{
		const Uph_MidiNote *note = &pattern->notes[n];
		if (note->key_number >= 128 || note->length_beats <= 0.0)
			continue;

		const double note_end = note->start_beat + note->length_beats;

		if (note_end > seg_start && note_end <= seg_end)
		{
			p->playback_logical[note->key_number] = false;
			_uph_midi_preview_add_event((Uph_MidiPreviewEvent){
				.offset = _uph_midi_preview_offset(beat_base + note_end - seg_start, sample_rate, bpm),
				.key = note->key_number,
				.playback = true
			});
		}

		if (note->start_beat >= seg_start && note->start_beat < seg_end)
		{
			if (note_end > seg_end)
				p->playback_logical[note->key_number] = true;

			_uph_midi_preview_add_event((Uph_MidiPreviewEvent){
				.offset = _uph_midi_preview_offset(beat_base + note->start_beat - seg_start, sample_rate, bpm),
				.key = note->key_number,
				.velocity = note->velocity,
				.note_on = true,
				.playback = true
			});
		}
	}
}

static void _uph_midi_preview_advance_playback(uint32_t frame_count, uint32_t sample_rate, float bpm, bool timeline_playing)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;

	bool playing = atomic_load_explicit(&p->playing, memory_order_acquire);
	if (playing && timeline_playing)
	{
		atomic_store_explicit(&p->playing, false, memory_order_release);
		playing = false;
	}

	const uint32_t restart = atomic_load_explicit(&p->restart_counter, memory_order_relaxed);
	if (restart != p->seen_restart)
	{
		p->seen_restart = restart;
		_uph_midi_preview_release_logical(0);
	}

	const uint32_t seek = atomic_load_explicit(&p->seek_counter, memory_order_acquire);
	if (seek != p->seen_seek)
	{
		p->seen_seek = seek;
		atomic_store_explicit(&p->position, atomic_load_explicit(&p->seek_position, memory_order_relaxed), memory_order_relaxed);
		_uph_midi_preview_release_logical(0);
	}

	if (!playing)
	{
		if (p->was_playing)
			_uph_midi_preview_release_logical(0);

		p->was_playing = false;
		return;
	}

	p->was_playing = true;

	const uint32_t pattern_index = atomic_load_explicit(&p->pattern_index, memory_order_relaxed);
	if (pattern_index >= (uint32_t)naui_list_len(uph_state.project.midi_patterns) || bpm <= 0.0f || sample_rate == 0)
		return;

	const Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[pattern_index];
	const double length = uph_midi_preview_loop_length(pattern);
	if (length <= 0.0)
		return;

	const bool loop = atomic_load_explicit(&p->loop, memory_order_relaxed);
	double position = atomic_load_explicit(&p->position, memory_order_relaxed);
	if (position >= length || position < 0.0)
		position = fmod(fmax(position, 0.0), length);

	double remaining = uph_seconds_to_beats((double)frame_count / (double)sample_rate, bpm);
	double beat_base = 0.0;

	for (uint32_t segment = 0; segment < UPH_MIDI_PREVIEW_MAX_SEGMENTS && remaining > 0.0; segment++)
	{
		const double seg_end = fmin(position + remaining, length);
		_uph_midi_preview_queue_segment(pattern, position, seg_end, beat_base, sample_rate, bpm);

		beat_base += seg_end - position;
		remaining -= seg_end - position;
		position = seg_end;

		if (position >= length)
		{
			_uph_midi_preview_release_logical(_uph_midi_preview_offset(beat_base, sample_rate, bpm));
			position = 0.0;

			if (!loop)
			{
				atomic_store_explicit(&p->playing, false, memory_order_release);
				p->was_playing = false;
				break;
			}
		}
	}

	atomic_store_explicit(&p->position, position, memory_order_relaxed);
}

void uph_midi_preview_begin_buffer(uint32_t frame_count, uint32_t sample_rate, float bpm, bool timeline_playing)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	p->event_count = 0;
	p->frame_count = frame_count;
	p->buffer_active = true;

	Uph_Track *voice = uph_midi_preview_voice();
	if (voice != p->active_voice)
	{
		bool any_held = false;
		for (uint32_t key = 0; key < 128; key++)
		{
			p->release_held[key] = p->audition_held[key] || p->playback_held[key];
			any_held |= p->release_held[key];
		}

		if (any_held)
			p->release_voice = p->active_voice;

		memset(p->audition_held, 0, sizeof(p->audition_held));
		memset(p->playback_held, 0, sizeof(p->playback_held));
		memset(p->playback_logical, 0, sizeof(p->playback_logical));
		p->active_voice = voice;
	}

	Uph_MidiPreviewRing *ring = &p->ring;
	const uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
	const uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
	for (uint32_t i = head; i != tail; i++)
		_uph_midi_preview_add_event(ring->buffer[i & (UPH_MIDI_PREVIEW_RING_CAPACITY - 1)]);
	atomic_store_explicit(&ring->head, tail, memory_order_release);

	_uph_midi_preview_advance_playback(frame_count, sample_rate, bpm, timeline_playing);
}

void uph_midi_preview_render_track(Uph_Track *track)
{
	Uph_MidiPreviewData *p = &uph_midi_preview_data;
	if (!p->buffer_active || !track)
		return;

	if (track == p->release_voice && track != p->active_voice)
	{
		for (uint32_t key = 0; key < 128; key++)
		{
			if (p->release_held[key] && uph_plugin_note_active(&track->instrument, (uint8_t)key))
				uph_plugin_queue_note_event(&track->instrument, false, (uint8_t)key, 0, 0, 0);
		}

		p->release_voice = NULL;
	}

	if (track != p->active_voice)
		return;

	for (uint32_t i = 0; i < p->event_count; i++)
	{
		const Uph_MidiPreviewEvent *event = &p->events[i];
		bool *own = event->playback ? p->playback_held : p->audition_held;
		const bool *other = event->playback ? p->audition_held : p->playback_held;

		if (event->note_on)
		{
			if (uph_plugin_note_active(&track->instrument, event->key))
				uph_plugin_queue_note_event(&track->instrument, false, event->key, 0, 0, event->offset);

			uph_plugin_queue_note_event(&track->instrument, true, event->key, 0, event->velocity, event->offset);
			own[event->key] = true;
		}
		else if (own[event->key])
		{
			own[event->key] = false;
			if (!other[event->key])
				uph_plugin_queue_note_event(&track->instrument, false, event->key, 0, 0, event->offset);
		}
	}

	p->event_count = 0;
}

void uph_midi_preview_end_buffer(void)
{
	uph_midi_preview_data.buffer_active = false;
	uph_midi_preview_data.event_count = 0;
}

bool uph_midi_preview_key_held(const Uph_Track *track, uint8_t key)
{
	const Uph_MidiPreviewData *p = &uph_midi_preview_data;
	return key < 128 && track && track == p->active_voice && (p->audition_held[key] || p->playback_held[key]);
}

#pragma endregion
