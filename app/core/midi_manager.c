typedef struct
{
	size_t note_index;
	uint32_t start_tick;
	uint8_t channel;
	uint8_t key;
} Uph_MidiOpenNote;

static uint32_t uph_midi_beats_to_ticks(const double beats)
{
	return beats > 0.0 ? (uint32_t)llround(beats * UPH_MIDI_PPQ) : 0;
}

Uph_MidiPattern uph_midi_convert_to_pattern(const Naui_Path midi_path)
{
	Uph_MidiPattern pattern = {0};
	cmidi_file_t* file = cmidi_file_load(midi_path.data);
	if (!file)
	{
		naui_log(NAUI_LOG_WARNING, "Failed to load MIDI file: %s", midi_path.data);
		return pattern;
	}

	const cmidi_sched_event_t* events = cmidi_file_events(file);
	const int event_count = cmidi_file_event_count(file);
	const uint16_t file_ticks_per_beat = cmidi_file_ticks_per_beat(file);
	const double ticks_per_beat = file_ticks_per_beat > 0 ? (double)file_ticks_per_beat : (double)UPH_MIDI_PPQ;
	Uph_MidiOpenNote* open = malloc(sizeof(Uph_MidiOpenNote) * (size_t)(event_count > 0 ? event_count : 1));
	size_t open_count = 0;
	uint32_t last_tick = 0;

	for (int i = 0; open && i < event_count; i++)
	{
		const cmidi_sched_event_t* event = &events[i];
		const bool is_on = event->type == CMIDI_SCHED_NOTE_ON && event->data1 > 0;
		const bool is_off = event->type == CMIDI_SCHED_NOTE_OFF || (event->type == CMIDI_SCHED_NOTE_ON && event->data1 == 0);
		last_tick = event->tick > last_tick ? event->tick : last_tick;

		if (is_on)
		{
			open[open_count++] = (Uph_MidiOpenNote){
				.note_index = naui_list_len(pattern.notes),
				.start_tick = event->tick,
				.channel = event->channel,
				.key = event->data0
			};

			naui_list_push(pattern.notes, ((Uph_MidiNote){
				.start_beat = (double)event->tick / ticks_per_beat,
				.length_beats = 0.0,
				.key_number = event->data0,
				.velocity = event->data1
			}));

		} else if(is_off)
		{
			for (size_t k = 0; k < open_count; k++)
			{
				if (open[k].channel != event->channel || open[k].key != event->data0)
					continue;

				const uint32_t length = event->tick > open[k].start_tick ? event->tick - open[k].start_tick : 1;
				pattern.notes[open[k].note_index].length_beats = (double)length / ticks_per_beat;
				memmove(&open[k], &open[k + 1], (open_count - k - 1) * sizeof(Uph_MidiOpenNote));
				open_count--;
				break;
			}
		}
	}

	for (size_t k = 0; k < open_count; k++)
	{
		const uint32_t length = last_tick > open[k].start_tick ? last_tick - open[k].start_tick : 1;
		pattern.notes[open[k].note_index].length_beats = (double)length / ticks_per_beat;
	}

	free(open);
	cmidi_file_free(file);
	return pattern;
} 

cmidi_file_t* uph_midi_convert_to_midi(const Uph_MidiPattern* pattern)
{
	if (!pattern)
		return NULL;

	cmidi_file_t* file = cmidi_file_create(UPH_MIDI_PPQ);
	if (!file)
		return NULL;

	const double bpm = uph_state.project.bpm > 0.0f ? (double)uph_state.project.bpm : 120.0;
	cmidi_file_set_format(file, 0);
	cmidi_file_set_tempo(file, (uint32_t)llround(60000000.0 / bpm));

	const size_t count = naui_list_len(pattern->notes);
	if (count == 0)
		return file;

	uint32_t* starts = malloc(sizeof(uint32_t) * count);
	uint32_t* ends = malloc(sizeof(uint32_t) * count);
	if (!starts || !ends)
	{
		free(starts);
		free(ends);
		cmidi_file_free(file);
		return NULL;
	}

	for (size_t i = 0; i < count; i++)
	{
		const Uph_MidiNote* note = &pattern->notes[i];
		starts[i] = uph_midi_beats_to_ticks(note->start_beat);
		ends[i] = uph_midi_beats_to_ticks(note->start_beat + note->length_beats);
		if (ends[i] <= starts[i])
			ends[i] = starts[i] + 1;
	}

	for (size_t i = 0; i < count; i++)
	{
		for (size_t j = 0; j < count; j++)
		{
			if (i != j && pattern->notes[i].key_number == pattern->notes[j].key_number && starts[j] == ends[i] && ends[i] - starts[i] > 1)
			{
				ends[i]--;
				break;
			}
		}
	}

	for (size_t i = 0; i < count; i++)
	{
		const Uph_MidiNote* note = &pattern->notes[i];
		if (note->key_number > 127)
			continue;

		const uint8_t velocity = note->velocity < 1 ? 1 : (note->velocity > 127 ? 127 : note->velocity);
		cmidi_file_add_note(file, 0, starts[i], note->key_number, velocity, ends[i] - starts[i]);
	}

	free(starts);
	free(ends);
	return file;
}