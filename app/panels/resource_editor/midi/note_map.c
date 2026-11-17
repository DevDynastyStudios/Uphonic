#define UPH_NOTES_PER_OCTAVE 12

#define UPH_NOTE_OCTAVE_OFFSET (0)
#define UPH_NOTE_MAP_KEY_COUNT 128
#define UPH_NOTE_NAME_MAX 16

typedef uint8_t Uph_NoteKind;
enum
{
	UPH_NOTE_KIND_NATURAL,
	UPH_NOTE_KIND_ACCIDENTAL
};

typedef uint8_t Uph_NoteHighlight;
enum
{
	UPH_NOTE_HIGHLIGHT_NONE,
	UPH_NOTE_HIGHLIGHT_SCALE,
	UPH_NOTE_HIGHLIGHT_ROOT
};

typedef struct
{
	char name[UPH_NOTE_NAME_MAX];
	Uph_NoteKind kind;
	Uph_NoteHighlight highlight;
	bool show_label;
}
Uph_NoteDef;

typedef struct
{
	Uph_NoteDef notes[UPH_NOTE_MAP_KEY_COUNT];
	uint8_t lowest;
	uint8_t highest;
	bool scale_active;
}
Uph_NoteMap;

static inline bool uph_note_map_in_range(const Uph_NoteMap *map, int32_t key)
{
	return key >= (int32_t)map->lowest && key <= (int32_t)map->highest;
}

static inline int32_t uph_note_map_clamp_key(const Uph_NoteMap *map, int32_t key)
{
	return NAUI_CLAMP(key, (int32_t)map->lowest, (int32_t)map->highest);
}

// Number of lanes the editor scrolls through.
static inline uint32_t uph_note_map_row_count(const Uph_NoteMap *map)
{
	return (uint32_t)map->highest - (uint32_t)map->lowest + 1u;
}

// Lanes are stacked from the top. The highest note is row 0.
static inline int32_t uph_note_map_row(const Uph_NoteMap *map, int32_t key)
{
	return (int32_t)map->highest - key;
}

static inline int32_t uph_note_map_key_at_row(const Uph_NoteMap *map, int32_t row)
{
	return (int32_t)map->highest - row;
}

static inline Uph_NoteKind uph_note_map_kind_at(const Uph_NoteMap *map, int32_t key)
{
	while (key >= UPH_NOTE_MAP_KEY_COUNT)
	{
		key -= UPH_NOTES_PER_OCTAVE;
	}

	while (key < 0)
	{
		key += UPH_NOTES_PER_OCTAVE;
	}

	return map->notes[key].kind;
}

static const char *const uph__note_pitch_class_names[UPH_NOTES_PER_OCTAVE] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

static const Uph_NoteKind uph__note_pitch_class_kinds[UPH_NOTES_PER_OCTAVE] =
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
		snprintf(def->name, sizeof(def->name), "%s%d", uph__note_pitch_class_names[pitch_class], octave);
		def->kind = uph__note_pitch_class_kinds[pitch_class];
		def->highlight = UPH_NOTE_HIGHLIGHT_NONE;
		def->show_label = pitch_class == 0;
	}
}

static void uph__note_map_clear_highlights(Uph_NoteMap *map)
{
	for (uint32_t key = 0; key < UPH_NOTE_MAP_KEY_COUNT; key++)
		map->notes[key].highlight = UPH_NOTE_HIGHLIGHT_NONE;
}

#pragma region Scales

const char *uph_note_pitch_class_name(uint8_t pitch_class)
{
	return uph__note_pitch_class_names[pitch_class % UPH_NOTES_PER_OCTAVE];
}

static bool uph__scale_contains(uint16_t mask, uint8_t root, uint8_t key)
{
	const uint32_t degree = ((uint32_t)key + UPH_NOTES_PER_OCTAVE - root % UPH_NOTES_PER_OCTAVE) % UPH_NOTES_PER_OCTAVE;
	return (mask >> degree) & 1u;
}

void uph_note_map_apply_scale(Uph_NoteMap *map, uint16_t mask, uint8_t root)
{
	uph__note_map_clear_highlights(map);
	map->scale_active = true;

	for (uint32_t key = 0; key < UPH_NOTE_MAP_KEY_COUNT; key++)
	{
		const bool is_root = key % UPH_NOTES_PER_OCTAVE == root % UPH_NOTES_PER_OCTAVE;
		map->notes[key].show_label = is_root;

		if (!uph__scale_contains(mask, root, (uint8_t)key))
			continue;

		map->notes[key].highlight = is_root ? UPH_NOTE_HIGHLIGHT_ROOT : UPH_NOTE_HIGHLIGHT_SCALE;
	}
}

#pragma endregion
