#define UPH_KEY_BITSET_BITS_PER_WORD (sizeof(uint64_t) * 8)
#define UPH_KEY_BITSET_WORD_COUNT ((UPH_NOTE_MAP_KEY_COUNT + UPH_KEY_BITSET_BITS_PER_WORD - 1) / UPH_KEY_BITSET_BITS_PER_WORD)

typedef uint64_t Uph_KeyBitset[UPH_KEY_BITSET_WORD_COUNT];

static inline bool uph_key_bitset_get(const Uph_KeyBitset set, uint8_t key)
{
	return (set[key / UPH_KEY_BITSET_BITS_PER_WORD] >> (key % UPH_KEY_BITSET_BITS_PER_WORD)) & 1u;
}

static inline void uph_key_bitset_set(Uph_KeyBitset set, uint8_t key, bool pressed)
{
	const uint64_t mask = (uint64_t)1u << (key % UPH_KEY_BITSET_BITS_PER_WORD);
	uint64_t *word = &set[key / UPH_KEY_BITSET_BITS_PER_WORD];
	*word = pressed ? (*word | mask) : (*word & ~mask);
}

static inline void uph_key_bitset_clear(Uph_KeyBitset set)
{
	for (uint32_t i = 0; i < UPH_KEY_BITSET_WORD_COUNT; i++)
		set[i] = 0;
}

static inline void uph_key_bitset_copy(Uph_KeyBitset dst, const Uph_KeyBitset src)
{
	for (uint32_t i = 0; i < UPH_KEY_BITSET_WORD_COUNT; i++)
		dst[i] = src[i];
}

static inline bool uph_key_bitset_was_pressed(const Uph_KeyBitset prev, const Uph_KeyBitset current, uint8_t key)
{
	return !uph_key_bitset_get(prev, key) && uph_key_bitset_get(current, key);
}

static inline bool uph_key_bitset_was_released(const Uph_KeyBitset prev, const Uph_KeyBitset current, uint8_t key)
{
	return uph_key_bitset_get(prev, key) && !uph_key_bitset_get(current, key);
}

typedef struct
{
	float key_length;
	float black_key_length_ratio;
	float black_key_height_ratio;
	float white_key_border_alpha;
	float white_key_border_thick;
	float black_key_border_alpha;
	float black_key_border_thick;
	float octave_border_alpha;
	float octave_border_thick;
}
Uph_PianoStyle;

typedef struct
{
	Uph_PianoStyle style;
	Uph_KeyBitset external_keys;
	int16_t mouse_key;
	bool mouse_active;
}
Uph_Piano;

typedef struct
{
	Leaf_BoundingBox box;
	float lane_height;
	float scroll_y;
}
Uph_PianoCanvas;

typedef struct
{
	float top;
	float bottom;
	float width;
	bool accidental;
}
Uph_PianoKeyRect;

typedef struct
{
	int16_t pressed;
	int16_t released;
}
Uph_PianoEvents;

void uph_piano_init(Uph_Piano *piano);

static inline Uph_PianoStyle uph_piano_default_style(void)
{
	return (Uph_PianoStyle) {
		.key_length = 120.0f,
		.black_key_length_ratio = 0.55f,
		.black_key_height_ratio = 0.55f,
		.white_key_border_alpha = 120.0f,
		.white_key_border_thick = 0.8f,
		.black_key_border_alpha = 255.0f,
		.black_key_border_thick = 1.0f,
		.octave_border_alpha = 220.0f,
		.octave_border_thick = 1.2f
	};
}

static inline float uph_piano_width(const Uph_Piano *piano)
{
	return NAUI_DPI(piano->style.key_length);
}

bool uph_piano_key_pressed(const Uph_Piano *piano, uint8_t key);
void uph_piano_set_external_key(Uph_Piano *piano, uint8_t key, bool pressed);
bool uph_piano_key_rect(const Uph_Piano *piano, const Uph_NoteMap *map, uint8_t key, const Uph_PianoCanvas *canvas, Uph_PianoKeyRect *out);

// Key under a screen position, or -1. Accidental keys win over the natural keys behind them.
int32_t uph_piano_hit_test(const Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas, float mouse_x, float mouse_y);
Uph_PianoEvents uph_piano_update(Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas, bool hovered);
Uph_PianoEvents uph_piano_release_mouse(Uph_Piano *piano);
void uph_piano_render(const Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas);
