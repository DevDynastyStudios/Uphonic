#define NAUI_TR(key) naui_localization_get(naui_localization_get_current(), key)

typedef uint8_t Naui_TextDirection;
enum
{
	NAUI_TEXT_LTR,
	NAUI_TEXT_RTL
};

typedef struct
{
	char* filename;
	char* language_code;
	char* region_code;
	char* display_name;
	Naui_TextDirection text_direction;
} Naui_LanguageMeta;

typedef struct
{
	char* key;
	char* value;
	bool is_interpolated;
} Naui_LanguageEntry;

typedef struct
{
	Naui_Map(Naui_LanguageEntry) table;
	Naui_LanguageMeta meta;
} Naui_Language;

/* Set the active language by code. Scans the language subfolder file names.
 * Attempts fall back to en-US if the requested code fails. */
void naui_localization_set_current(const Naui_String language_code);

void naui_localization_set_current_lang(Naui_Language* language);

/* Returns the currently active language, or NULL if none loaded. */
Naui_Language* naui_localization_get_current(void);

/* Rebuild the metadata cache from all .lang files in the language subfolder. */
void naui_localization_reload_meta_cache(void);

/* Return the cached list of available languages.
 * Valid until the next naui_localization_reload_meta_cache() call. */
Naui_List(Naui_LanguageMeta) naui_localization_get_languages(void);

/* Load a language by code into out_language.
 * Returns false if the file cannot be found or parsed. */
bool naui_localization_load(const Naui_String language_code, Naui_Language* out_language);

/* Load directly from a file stored in the Localization folder */
bool naui_localization_load_file(const Naui_Path path, Naui_Language* out_language);

/* Look up a key. Returns the key itself if not found (never NULL). */
const char* naui_localization_get(const Naui_Language* language, const char* key);

/*
 * Look up and format an interpolated string.
 * Returns NULL if key not found or not interpolated.
 */
char* naui_localization_format(const Naui_Language* language, const char* key, const char** args, int arg_count);

/* Free all memory owned by a Naui_Language. */
void naui_localization_free(Naui_Language* language);
