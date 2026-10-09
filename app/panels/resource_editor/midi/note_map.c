static const char *const _uph_note_pitch_class_names[UPH_NOTES_PER_OCTAVE] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

static const Uph_NoteKind _uph_note_pitch_class_kinds[UPH_NOTES_PER_OCTAVE] =
{
	UPH_NOTE_KIND_NATURAL, UPH_NOTE_KIND_ACCIDENTAL,	// C  C#
	UPH_NOTE_KIND_NATURAL, UPH_NOTE_KIND_ACCIDENTAL,	// D  D#
	UPH_NOTE_KIND_NATURAL,								// E
	UPH_NOTE_KIND_NATURAL, UPH_NOTE_KIND_ACCIDENTAL,	// F  F#
	UPH_NOTE_KIND_NATURAL, UPH_NOTE_KIND_ACCIDENTAL,	// G  G#
	UPH_NOTE_KIND_NATURAL, UPH_NOTE_KIND_ACCIDENTAL,	// A  A#
	UPH_NOTE_KIND_NATURAL								// B
};

void uph_note_map_init_default(Uph_NoteMap *map)
{
	memset(map, 0, sizeof(*map));
	map->lowest = 0;
	map->highest = UPH_NOTE_MAP_KEY_COUNT - 1;

	for (int32_t key = 0; key < UPH_NOTE_MAP_KEY_COUNT; key++)
	{
		const int32_t pitch_class = key % UPH_NOTES_PER_OCTAVE;
		const int32_t octave = key / UPH_NOTES_PER_OCTAVE + UPH_NOTE_OCTAVE_OFFSET;

		Uph_NoteDef *def = &map->notes[key];
		snprintf(def->name, sizeof(def->name), "%s%d", _uph_note_pitch_class_names[pitch_class], octave);
		def->kind = _uph_note_pitch_class_kinds[pitch_class];
		def->highlight = UPH_NOTE_HIGHLIGHT_NONE;
		def->show_label = pitch_class == 0;
	}
}

void uph_note_map_set_range(Uph_NoteMap *map, uint8_t lowest, uint8_t highest)
{
	if (highest >= UPH_NOTE_MAP_KEY_COUNT)
		highest = UPH_NOTE_MAP_KEY_COUNT - 1;
		
	if (lowest > highest)
		lowest = highest;

	map->lowest = lowest;
	map->highest = highest;
}

void uph_note_map_set_kind(Uph_NoteMap *map, uint8_t key, Uph_NoteKind kind)
{
	if (key < UPH_NOTE_MAP_KEY_COUNT)
		map->notes[key].kind = kind;
}

void uph_note_map_set_name(Uph_NoteMap *map, uint8_t key, const char *name)
{
	if (key < UPH_NOTE_MAP_KEY_COUNT)
		snprintf(map->notes[key].name, sizeof(map->notes[key].name), "%s", name ? name : "");
}

void uph_note_map_set_show_label(Uph_NoteMap *map, uint8_t key, bool show_label)
{
	if (key < UPH_NOTE_MAP_KEY_COUNT)
		map->notes[key].show_label = show_label;
}

void uph_note_map_set_highlight(Uph_NoteMap *map, uint8_t key, Uph_NoteHighlight highlight)
{
	if (key < UPH_NOTE_MAP_KEY_COUNT)
		map->notes[key].highlight = highlight;
}

void uph_note_map_clear_highlights(Uph_NoteMap *map)
{
	for (uint32_t key = 0; key < UPH_NOTE_MAP_KEY_COUNT; key++)
		map->notes[key].highlight = UPH_NOTE_HIGHLIGHT_NONE;
}
