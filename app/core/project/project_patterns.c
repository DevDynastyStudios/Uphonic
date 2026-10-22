static const char *s_midi_extensions[] = { ".mid", ".midi", ".rmi", ".kar", NULL };

typedef struct
{
	int64_t start_tick;
	int64_t length_ticks;
	uint8_t key;
	uint8_t velocity;
}
Uph_MidiNoteKey;

static int _uph_midi_note_key_compare(const void *a, const void *b)
{
	const Uph_MidiNoteKey *left = a;
	const Uph_MidiNoteKey *right = b;

	if (left->start_tick != right->start_tick)
		return left->start_tick < right->start_tick ? -1 : 1;

	if (left->key != right->key)
		return left->key < right->key ? -1 : 1;

	if (left->velocity != right->velocity)
		return left->velocity < right->velocity ? -1 : 1;

	if (left->length_ticks != right->length_ticks)
		return left->length_ticks < right->length_ticks ? -1 : 1;
		
	return 0;
}

static Uph_MidiNoteKey *_uph_midi_pattern_keys(const Uph_MidiPattern *pattern)
{
	const size_t count = naui_list_len(pattern->notes);
	Uph_MidiNoteKey *keys = malloc(sizeof(Uph_MidiNoteKey) * (count > 0 ? count : 1));
	if (!keys)
		return NULL;

	for (size_t i = 0; i < count; i++)
	{
		const Uph_MidiNote *note = &pattern->notes[i];
		keys[i] = (Uph_MidiNoteKey){
			.start_tick = (int64_t)llround(note->start_beat * UPH_MIDI_PPQ),
			.length_ticks = (int64_t)llround(note->length_beats * UPH_MIDI_PPQ),
			.key = note->key_number,
			.velocity = note->velocity
		};
	}

	qsort(keys, count, sizeof(Uph_MidiNoteKey), _uph_midi_note_key_compare);
	return keys;
}

static bool _uph_midi_note_keys_equal(const Uph_MidiNoteKey *a, const Uph_MidiNoteKey *b, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		const int64_t length_difference = a[i].length_ticks - b[i].length_ticks;
		if (a[i].start_tick != b[i].start_tick || a[i].key != b[i].key || a[i].velocity != b[i].velocity || length_difference > 1 || length_difference < -1)
			return false;
	}

	return true;
}

static int32_t _uph_project_find_duplicate_pattern(const Uph_Project *project, const Uph_MidiPattern *incoming)
{
	const size_t count = naui_list_len(incoming->notes);
	if (count == 0)
		return -1;

	Uph_MidiNoteKey *incoming_keys = NULL;
	int32_t found = -1;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(project->midi_patterns) && found < 0; i++)
	{
		if (naui_list_len(project->midi_patterns[i].notes) != count)
			continue;

		if (!incoming_keys && !(incoming_keys = _uph_midi_pattern_keys(incoming)))
			break;

		Uph_MidiNoteKey *existing_keys = _uph_midi_pattern_keys(&project->midi_patterns[i]);
		if (existing_keys && _uph_midi_note_keys_equal(incoming_keys, existing_keys, count))
			found = (int32_t)i;

		free(existing_keys);
	}

	free(incoming_keys);
	return found;
}

bool uph_project_is_midi_extension(const Naui_StringView extension)
{
	for (const char **midi = s_midi_extensions; *midi; midi++)
	{
		if (naui_string_view_equals_cstr(extension, *midi, false))
			return true;
	}

	return false;
}

bool uph_project_add_pattern_file(Uph_Project *project, const Naui_Path file_path)
{
	Uph_MidiPattern pattern = uph_midi_convert_to_pattern(file_path);
	if (naui_list_len(pattern.notes) == 0)
	{
		naui_log(NAUI_LOG_WARNING, "Failed to load MIDI Pattern (%s)", file_path.data);
		naui_list_free(pattern.notes);
		return false;
	}

	const int32_t duplicate = _uph_project_find_duplicate_pattern(project, &pattern);
	if (duplicate >= 0)
	{
		naui_log(NAUI_LOG_INFO, "Skipping MIDI file, identical pattern already in project: %s", project->midi_patterns[duplicate].name.data);
		naui_list_free(pattern.notes);
		uph_state.shared.selected_resource.index = (uint32_t)duplicate;
		uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
		return true;
	}

	naui_string_copy_view(&pattern.name, naui_file_stem(&file_path));
	naui_list_push(project->midi_patterns, pattern);
	uph_state.shared.selected_resource.index = naui_list_len(project->midi_patterns) - 1;
	uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
	naui_log(NAUI_LOG_INFO, "Added MIDI Pattern (%s)", file_path.data);
	return true;
}

void uph_project_adopt_pattern_files(Uph_Project *project, const Naui_Path project_folder)
{
	const Naui_Path patterns_dir = naui_path_join(project_folder, NAUI_PATH(UPH_IO_FOLDER_PATTERNS));
	Naui_List(Naui_DirEntry) files = naui_directory_filter(patterns_dir, NULL, s_midi_extensions);
	if (!files)
		return;

	qsort(files, naui_list_len(files), sizeof(Naui_DirEntry), uph_project_dir_entry_compare);
	Naui_List(Naui_String) defined = uph_io_defined_pattern_files(patterns_dir);

	for (size_t i = 0; i < naui_list_len(files); i++)
	{
		const Naui_Path file = files[i].path;
		if (files[i].is_directory || uph_project_file_name_is_listed(defined, file))
			continue;

		Uph_MidiPattern pattern = uph_midi_convert_to_pattern(file);
		if (naui_list_len(pattern.notes) == 0)
		{
			naui_log(NAUI_LOG_WARNING, "Deleting MIDI file that failed to load: %s", file.data);
			naui_list_free(pattern.notes);
			naui_file_delete(file);
			continue;
		}

		if (_uph_project_find_duplicate_pattern(project, &pattern) >= 0)
		{
			naui_log(NAUI_LOG_INFO, "Not adding MIDI file, an identical pattern is already in the project: %s", file.data);
			naui_list_free(pattern.notes);
			continue;
		}

		naui_string_copy_view(&pattern.name, naui_file_stem(&file));
		naui_list_push(project->midi_patterns, pattern);
		naui_log(NAUI_LOG_INFO, "Added MIDI pattern found in the project folder: %s", file.data);

		char canonical_name[64];
		snprintf(canonical_name, sizeof(canonical_name), "pattern_%04zu.mid", naui_list_len(project->midi_patterns) - 1);
		const Naui_Path canonical = naui_path_join(patterns_dir, naui_path_from_cstr(canonical_name));
		if (strcmp(canonical.data, file.data) != 0 && !naui_path_exists(canonical) && !uph_project_file_name_is_listed(defined, canonical))
			naui_file_rename(file, canonical);
	}

	naui_list_free(defined);
	naui_directory_filter_free(files);
}