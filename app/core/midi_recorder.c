#define UPH_MIDI_RECORD_RING_CAPACITY 1024
#define UPH_MIDI_RECORD_MAX_OPEN_NOTES 256
#define UPH_MIDI_RECORD_MIN_NOTE_BEATS (1.0 / 32.0)

_Static_assert((UPH_MIDI_RECORD_RING_CAPACITY & (UPH_MIDI_RECORD_RING_CAPACITY - 1)) == 0, "must be power of two");

typedef struct
{
	float time;
	uint8_t key;
	uint8_t velocity;
	bool note_on;
}
Uph_MidiRecordEvent;

typedef struct
{
	Uph_MidiRecordEvent buffer[UPH_MIDI_RECORD_RING_CAPACITY];
	_Atomic uint32_t head;
	_Atomic uint32_t tail;
}
Uph_MidiRecordRing;

typedef struct
{
	uint32_t pattern_index;
	uint32_t note_index;
	double raw_start_beat;
	uint8_t key;
}
Uph_MidiRecordOpenNote;

static Uph_MidiRecordRing uph_midi_record_ring;
static Uph_MidiRecordEvent uph_midi_record_events[UPH_MIDI_RECORD_RING_CAPACITY];
static Uph_MidiRecordOpenNote uph_midi_open_notes[UPH_MIDI_RECORD_MAX_OPEN_NOTES];
static uint32_t uph_midi_open_note_count;
static _Atomic bool uph_midi_live_held_keys[128];

void uph_midi_recorder_input(bool note_on, uint8_t key, uint8_t velocity)
{
	if (key > 127)
		return;

	if (note_on && velocity == 0)
		note_on = false;

	atomic_store_explicit(&uph_midi_live_held_keys[key], note_on, memory_order_relaxed);

	if (!uph_state.shared.song_timeline_playing)
		return;

	uint32_t tail = atomic_load_explicit(&uph_midi_record_ring.tail, memory_order_relaxed);
	uint32_t head = atomic_load_explicit(&uph_midi_record_ring.head, memory_order_acquire);
	if ((uint32_t)(tail - head) >= UPH_MIDI_RECORD_RING_CAPACITY)
		return;

	uph_midi_record_ring.buffer[tail & (UPH_MIDI_RECORD_RING_CAPACITY - 1)] = (Uph_MidiRecordEvent){
		.time = naui_time(),
		.key = key,
		.velocity = velocity,
		.note_on = note_on
	};
	atomic_store_explicit(&uph_midi_record_ring.tail, tail + 1, memory_order_release);
}

bool uph_midi_recorder_key_held(uint8_t key)
{
	return key < 128 && atomic_load_explicit(&uph_midi_live_held_keys[key], memory_order_relaxed);
}

static uint32_t uph_midi_record_ring_drain(Uph_MidiRecordEvent *out, uint32_t max_events)
{
	uint32_t head = atomic_load_explicit(&uph_midi_record_ring.head, memory_order_relaxed);
	uint32_t tail = atomic_load_explicit(&uph_midi_record_ring.tail, memory_order_acquire);

	uint32_t count = tail - head;
	if (count > max_events)
		count = max_events;

	if (out)
	{
		for (uint32_t i = 0; i < count; i++)
			out[i] = uph_midi_record_ring.buffer[(head + i) & (UPH_MIDI_RECORD_RING_CAPACITY - 1)];
	}

	atomic_store_explicit(&uph_midi_record_ring.head, head + count, memory_order_release);
	return count;
}

static void uph_midi_record_close_note(uint32_t open_index, double end_beat)
{
	Uph_MidiRecordOpenNote open = uph_midi_open_notes[open_index];
	uph_midi_open_notes[open_index] = uph_midi_open_notes[--uph_midi_open_note_count];

	Uph_Project *project = &uph_state.project;
	if (open.pattern_index >= naui_list_len(project->midi_patterns))
		return;

	Uph_MidiPattern *pattern = &project->midi_patterns[open.pattern_index];
	if (open.note_index >= naui_list_len(pattern->notes))
		return;

	const double length = end_beat - open.raw_start_beat;
	pattern->notes[open.note_index].length_beats = length > UPH_MIDI_RECORD_MIN_NOTE_BEATS ? length : UPH_MIDI_RECORD_MIN_NOTE_BEATS;
}

static void uph_midi_record_close_pattern_notes(uint32_t pattern_index, double end_beat)
{
	for (uint32_t i = 0; i < uph_midi_open_note_count; )
	{
		if (uph_midi_open_notes[i].pattern_index == pattern_index)
			uph_midi_record_close_note(i, end_beat);
		else
			i++;
	}
}

static void uph_midi_record_drop_pattern_notes(uint32_t pattern_index)
{
	for (uint32_t i = 0; i < uph_midi_open_note_count; )
	{
		if (uph_midi_open_notes[i].pattern_index == pattern_index)
			uph_midi_open_notes[i] = uph_midi_open_notes[--uph_midi_open_note_count];
		else
			i++;
	}
}

static void uph_midi_record_note_on(
	Uph_MidiPattern *pattern,
	uint32_t pattern_index,
	const Uph_TimelineBlock *block,
	double beat,
	uint8_t key,
	uint8_t velocity
)
{
	for (uint32_t i = 0; i < uph_midi_open_note_count; i++)
	{
		if (uph_midi_open_notes[i].pattern_index == pattern_index && uph_midi_open_notes[i].key == key)
		{
			uph_midi_record_close_note(i, beat - block->start_beat);
			break;
		}
	}

	double start_beat = beat;
	if (uph_state.settings.midi.record_quantize)
		start_beat = uph_snap_beat_round(beat, (Uph_SnapResolution)uph_state.settings.midi.record_quantize_grid);

	start_beat -= block->start_beat;
	if (start_beat < 0.0)
		start_beat = 0.0;

	const uint32_t note_index = (uint32_t)naui_list_len(pattern->notes);
	naui_list_push(pattern->notes, ((Uph_MidiNote){
		.start_beat = start_beat,
		.length_beats = UPH_MIDI_RECORD_MIN_NOTE_BEATS,
		.key_number = key,
		.velocity = velocity
	}));

	if (uph_midi_open_note_count < UPH_MIDI_RECORD_MAX_OPEN_NOTES)
	{
		uph_midi_open_notes[uph_midi_open_note_count++] = (Uph_MidiRecordOpenNote){
			.pattern_index = pattern_index,
			.note_index = note_index,
			.raw_start_beat = beat - block->start_beat,
			.key = key
		};
	}
}

static void uph_midi_record_note_off(uint32_t pattern_index, const Uph_TimelineBlock *block, double beat, uint8_t key)
{
	for (uint32_t i = 0; i < uph_midi_open_note_count; i++)
	{
		if (uph_midi_open_notes[i].pattern_index == pattern_index && uph_midi_open_notes[i].key == key)
		{
			uph_midi_record_close_note(i, beat - block->start_beat);
			return;
		}
	}
}

static void uph_midi_record_begin_take(Uph_Track *track, double start_beat)
{
	uph_resources_add_pattern();

	const Uph_ResourceIndex pattern_index = (Uph_ResourceIndex)naui_list_len(uph_state.project.midi_patterns) - 1;
	uph_midi_record_drop_pattern_notes(pattern_index);

	Uph_TimelineBlock block = {
		.type = UPH_RESOURCE_PATTERN,
		.start_beat = start_beat,
		.length_beats = (double)uph_state.project.time_signature.denominator,
		.resource_index = pattern_index
	};

	naui_list_push(track->blocks, block);
	track->armed_block = &track->blocks[naui_list_len(track->blocks) - 1];
}

void uph_midi_recorder_finish_take(Uph_Track *track)
{
	if (track->armed_block == UPH_INVALID_TIMELINE_BLOCK)
		return;

	Uph_Project *project = &uph_state.project;

	if (uph_track_armed_block_in_range(track))
	{
		Uph_TimelineBlock *block = track->armed_block;
		const uint32_t pattern_index = block->resource_index;

		if (pattern_index < naui_list_len(project->midi_patterns))
		{
			const double end_beat = uph_state.shared.song_timeline_playhead_position - block->start_beat;
			uph_midi_record_close_pattern_notes(pattern_index, end_beat);

			const double pattern_length = uph_calculate_pattern_length(&project->midi_patterns[pattern_index]);
			if (pattern_length > block->length_beats)
				block->length_beats = pattern_length;
		}
	}

	track->armed_block = UPH_INVALID_TIMELINE_BLOCK;
}

static void uph_midi_record_tracks(
	Naui_List(Uph_Track) tracks,
	uint32_t event_count,
	float callback_time,
	double buffer_start_beat,
	double buffer_end_beat,
	float bpm
)
{
	Uph_Project *project = &uph_state.project;

	for (uint64_t i = 0; i < naui_list_len(tracks); i++)
	{
		Uph_Track *track = &tracks[i];
		uph_midi_record_tracks(track->subtracks, event_count, callback_time, buffer_start_beat, buffer_end_beat, bpm);

		if (track->type != UPH_RESOURCE_PATTERN)
			continue;

		if (!(track->state & UPH_TRACK_ARMED))
		{
			uph_midi_recorder_finish_take(track);
			continue;
		}

		if (track->armed_block == UPH_INVALID_TIMELINE_BLOCK)
		{
			uph_midi_record_begin_take(track, buffer_start_beat);
			if (track->armed_block == UPH_INVALID_TIMELINE_BLOCK)
				continue;
		}

		if (!uph_track_armed_block_in_range(track))
		{
			track->armed_block = UPH_INVALID_TIMELINE_BLOCK;
			continue;
		}

		Uph_TimelineBlock *block = track->armed_block;
		const uint32_t pattern_index = block->resource_index;

		if (pattern_index >= naui_list_len(project->midi_patterns))
		{
			track->armed_block = UPH_INVALID_TIMELINE_BLOCK;
			continue;
		}

		Uph_MidiPattern *pattern = &project->midi_patterns[pattern_index];

		for (uint32_t e = 0; e < event_count; e++)
		{
			const Uph_MidiRecordEvent *event = &uph_midi_record_events[e];

			double seconds_ago = callback_time - event->time;
			if (seconds_ago < 0.0)
				seconds_ago = 0.0;

			double beat = buffer_start_beat - uph_seconds_to_beats(seconds_ago, bpm);
			if (beat < block->start_beat)
				beat = block->start_beat;

			if (event->note_on)
				uph_midi_record_note_on(pattern, pattern_index, block, beat, event->key, event->velocity);
			else
				uph_midi_record_note_off(pattern_index, block, beat, event->key);
		}

		for (uint32_t o = 0; o < uph_midi_open_note_count; o++)
		{
			const Uph_MidiRecordOpenNote *open = &uph_midi_open_notes[o];
			if (open->pattern_index != pattern_index || open->note_index >= naui_list_len(pattern->notes))
				continue;

			const double length = (buffer_end_beat - block->start_beat) - open->raw_start_beat;
			pattern->notes[open->note_index].length_beats = length > UPH_MIDI_RECORD_MIN_NOTE_BEATS ? length : UPH_MIDI_RECORD_MIN_NOTE_BEATS;
		}

		block->length_beats = NAUI_MAX(
            (double)uph_state.project.time_signature.denominator,
            buffer_end_beat - block->start_beat
        );
	}
}

void uph_midi_recorder_process(double buffer_start_beat, uint32_t frame_count, uint32_t sample_rate, float bpm, bool playing)
{
	if (!playing || bpm <= 0.0f)
	{
		uph_midi_record_ring_drain(NULL, UPH_MIDI_RECORD_RING_CAPACITY);
		return;
	}

	const float callback_time = naui_time();
	const uint32_t event_count = uph_midi_record_ring_drain(uph_midi_record_events, UPH_MIDI_RECORD_RING_CAPACITY);
	const double buffer_end_beat = buffer_start_beat + uph_seconds_to_beats((double)frame_count / (double)sample_rate, bpm);

	uph_midi_record_tracks(uph_state.project.tracks, event_count, callback_time, buffer_start_beat, buffer_end_beat, bpm);
}