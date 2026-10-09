#define UPH_NOTE_MAP_KEY_COUNT 128
#define UPH_NOTE_NAME_MAX 16
#define UPH_NOTES_PER_OCTAVE 12

// MIDI note 60 (C4)
#define UPH_NOTE_OCTAVE_OFFSET (-1)

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
}
Uph_NoteMap;

void uph_note_map_init_default(Uph_NoteMap *map);
void uph_note_map_set_range(Uph_NoteMap *map, uint8_t lowest, uint8_t highest);
void uph_note_map_set_kind(Uph_NoteMap *map, uint8_t key, Uph_NoteKind kind);
void uph_note_map_set_name(Uph_NoteMap *map, uint8_t key, const char *name);
void uph_note_map_set_show_label(Uph_NoteMap *map, uint8_t key, bool show_label);
void uph_note_map_set_highlight(Uph_NoteMap *map, uint8_t key, Uph_NoteHighlight highlight);
void uph_note_map_clear_highlights(Uph_NoteMap *map);

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
