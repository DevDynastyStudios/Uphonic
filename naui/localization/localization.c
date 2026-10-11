#define NAUI_LOCALIZATION_NAME_SIZE 128
#define NAUI_LOCALIZATION_FOLDER_NAME "Localization"
#define NAUI_LOCALIZATION_EXTENSION ".lang"

static Naui_Language g_current_language;
static Naui_List(Naui_LanguageMeta) g_meta_cache = NULL;
static bool g_current_loaded = false;
static bool g_meta_cache_init = false;

static Naui_Language g_retired_language;
static bool g_retired_loaded = false;

#pragma region Static Helpers
static Naui_TextDirection _naui_parse_direction(const char* dir)
{
	if (dir && strcmp(dir, "rtl") == 0)
		return NAUI_TEXT_RTL;

	return NAUI_TEXT_LTR;
}

static Naui_Path _naui_localization_build_path(const char* code)
{
	Naui_Path assets_dir = naui_directory_get(NAUI_DIR_ASSETS);
	Naui_Path lang_dir = naui_path_join(assets_dir, naui_path_from_cstr(NAUI_LOCALIZATION_FOLDER_NAME));
	char filename[NAUI_LOCALIZATION_NAME_SIZE];
	snprintf(filename, sizeof(filename), "%s%s", code, NAUI_LOCALIZATION_EXTENSION);
	return naui_path_join(lang_dir, naui_path_from_cstr(filename));
}

static void _naui_localization_install(Naui_Language* incoming)
{
	if (g_retired_loaded)
		naui_localization_free(&g_retired_language);

	g_retired_loaded = g_current_loaded;
	if (g_current_loaded)
		g_retired_language = g_current_language;

	g_current_language = *incoming;
	g_current_loaded = true;
	memset(incoming, 0, sizeof(*incoming));
}
#pragma endregion

#pragma region Public API
bool naui_localization_load_file(const Naui_Path path, Naui_Language* out_language)
{
	if (!out_language)
		return false;

	/* Callers often pass an uninitialized struct, anything a file leaves out (name, region, ...) must not hold garbage. */
	memset(out_language, 0, sizeof(*out_language));

	const Naui_Path lang_path = naui_path_normalize(path);
	if (naui_path_is_empty(lang_path))
		return false;

	Naui_Json json = naui_json_parse_file(lang_path);

	if (!json.root || json.error)
	{
		naui_log(NAUI_LOG_ERROR, "Failed to parse localization file: %s (%s, line %d, column %d)", lang_path.data, json.error ? json.error : "unknown error", json.error_line, json.error_col);
		naui_json_free(&json);
		return false;
	}

	if (json.root->type != NAUI_JSON_OBJECT)
	{
		naui_log(NAUI_LOG_ERROR, "Localization file root is not an object: %s", lang_path.data);
		naui_json_free(&json);
		return false;
	}

	size_t file_size = naui_file_size(lang_path);
	naui_arena_init(&out_language->arena, file_size > 0 ? file_size : 1024);
	out_language->meta.filename = lang_path;
	out_language->meta.text_direction = NAUI_TEXT_LTR;

	uint32_t invalid_utf8_count = 0;

	NAUI_JSON_FOREACH(json.root, key, value)
	{
		if (strcmp(key->string.data, "_meta") == 0)
		{
			if (value->type != NAUI_JSON_OBJECT)
				continue;

			const Naui_JsonValue* lang_v = naui_json_object_get(value, "language");
			const Naui_JsonValue* dir_v = naui_json_object_get(value, "direction");
			const Naui_JsonValue* name_v = naui_json_object_get(value, "name");
			const Naui_JsonValue* font_v = naui_json_object_get(value, "font");
			if (lang_v && lang_v->type == NAUI_JSON_STRING)
			{
				char lang_code_buf[64];
				naui_json_copy_cstr(lang_v, lang_code_buf, sizeof(lang_code_buf));
				naui_localization_split_locale(lang_code_buf, &out_language->meta.language_code, &out_language->meta.region_code);
			}

			if (dir_v && dir_v->type == NAUI_JSON_STRING)
			{
				char dir_buf[16];
				naui_json_copy_cstr(dir_v, dir_buf, sizeof(dir_buf));
				out_language->meta.text_direction = _naui_parse_direction(dir_buf);
			}

			if (name_v && name_v->type == NAUI_JSON_STRING)
				naui_json_copy_string(name_v, &out_language->meta.display_name);

			if (font_v && font_v->type == NAUI_JSON_STRING)
				naui_json_copy_string(font_v, &out_language->meta.font);

			continue;
		}

		if (value->type != NAUI_JSON_STRING)
			continue;

		size_t val_cap = value->string.length + 1;
		char* raw_value = (char*)naui_arena_alloc(&out_language->arena, val_cap);
		if (!raw_value)
			continue;

		naui_json_copy_cstr(value, raw_value, val_cap);

		if (!naui_utf8_is_valid(raw_value, value->string.length))
		{
			if (invalid_utf8_count < 3)
				naui_log(NAUI_LOG_WARNING, "Localization file %s: the text for \"%.*s\" is not valid UTF-8", lang_path.data, (int)key->string.length, key->string.data);

			invalid_utf8_count++;
		}

		bool is_interpolated = (raw_value[0] == '$');
		char* entry_value = is_interpolated ? raw_value + 1 : raw_value;
		char* entry_key = (char*)naui_arena_alloc(&out_language->arena, key->string.length + 1);
		if (!entry_key)
			continue;

		naui_json_copy_cstr(key, entry_key, key->string.length + 1);
		Naui_LanguageEntry entry;
		entry.key = entry_key;
		entry.value = entry_value;
		entry.is_interpolated = is_interpolated;
		naui_strmap_puts(out_language->table, entry);
	}

	naui_json_free(&json);
	if (invalid_utf8_count > 0)
		naui_log(NAUI_LOG_WARNING, "Localization file %s: %u entries are not valid UTF-8. Was it saved as ANSI/Windows-1252? Re-save it as UTF-8.", lang_path.data, invalid_utf8_count);

	if (naui_string_is_empty(out_language->meta.language_code))
	{
		Naui_String stem = naui_view_to_string(naui_file_stem(&lang_path));
		naui_localization_split_locale(stem.data, &out_language->meta.language_code, &out_language->meta.region_code);
	}

	return true;
}

void naui_localization_split_locale(const char* code, Naui_String* out_lang, Naui_String* out_region)
{
	*out_lang = (Naui_String){0};
	*out_region = (Naui_String){0};

	if (!code || !code[0])
		return;

	Naui_String code_str = naui_string_from_cstr(code);
	Naui_StringView code_view = naui_string_to_view(&code_str);
	Naui_StringView parts[2];
	size_t n = naui_string_view_split(code_view, '-', parts, 2);
	if (n >= 1)
		*out_lang = naui_view_to_string(parts[0]);

	if (n >= 2)
		*out_region = naui_view_to_string(parts[1]);
}

bool naui_localization_load(const Naui_String language_code, Naui_Language* out_language)
{
	if (naui_string_is_empty(language_code) || !out_language)
		return false;

	Naui_Path path = _naui_localization_build_path(language_code.data);
	return naui_localization_load_file(path, out_language);
}

bool naui_localization_set_current(const Naui_String language_code)
{
	naui_log(NAUI_LOG_INFO, "Loading language file: %s", language_code.data);
	Naui_Language new_lang;
	bool ok = naui_localization_load(language_code, &new_lang);

	if (!ok)
	{
		bool already_en_us = !naui_string_is_empty(language_code) && strcmp(language_code.data, "en-US") == 0;
		if (already_en_us)
		{
			naui_log(NAUI_LOG_ERROR, "Failed to load language file en-US. Is the file missing?");
			return false;
		}

		naui_log(NAUI_LOG_ERROR, "Unable to load localization file for '%s'. Falling back to en-US", naui_string_is_empty(language_code) ? "" : language_code.data);
		ok = naui_localization_load(naui_string_from_cstr("en-US"), &new_lang);
		if (!ok)
		{
			Naui_Path language_folder = naui_path_join(naui_directory_get(NAUI_DIR_ASSETS), naui_path_from_cstr(NAUI_LOCALIZATION_FOLDER_NAME));
			naui_log(NAUI_LOG_ERROR, "Failed all attempts to load language file at: %s", language_folder.data);
			return false;
		}
	}

	_naui_localization_install(&new_lang);
	naui_log(NAUI_LOG_INFO, "Successfully loaded: %s-%s", g_current_language.meta.language_code.data, g_current_language.meta.region_code.data);
	return true;
}

void naui_localization_set_current_lang(Naui_Language* lang)
{
	if (!lang)
		return;

	_naui_localization_install(lang);
}

Naui_Language* naui_localization_get_current(void)
{
	if (!g_current_loaded)
		return NULL;

	return &g_current_language;
}

void naui_localization_reload_meta_cache(void)
{
	if (g_meta_cache_init)
	{
		naui_list_free(g_meta_cache);
		g_meta_cache = NULL;
	}

	g_meta_cache_init = true;
	Naui_Path assets_dir = naui_directory_get(NAUI_DIR_ASSETS);
	Naui_Path lang_dir = naui_path_join(assets_dir, naui_path_from_cstr(NAUI_LOCALIZATION_FOLDER_NAME));
	if (!naui_path_exists(lang_dir))
		return;

	Naui_List(Naui_DirEntry) entries = naui_directory_filter(lang_dir, NULL, NAUI_EXTENSIONS(NAUI_LOCALIZATION_EXTENSION));
	for (ptrdiff_t i = 0; i < naui_list_len(entries); i++)
	{
		if (entries[i].is_directory)
			continue;

		Naui_Language lang;
		if (!naui_localization_load_file(entries[i].path, &lang))
			continue;

		naui_list_push(g_meta_cache, lang.meta);
		naui_localization_free(&lang);
	}

	naui_directory_filter_free(entries);
}

Naui_List(Naui_LanguageMeta) naui_localization_get_languages(void)
{
	if (!g_meta_cache_init)
		naui_localization_reload_meta_cache();

	return g_meta_cache;
}

const char* naui_localization_get(const Naui_Language* language, const char* key)
{
	if (!language || !key)
		return key;

	Naui_LanguageEntry* table = (Naui_LanguageEntry*)language->table;
	ptrdiff_t idx = naui_strmap_get_index(table, key);
	if (idx < 0)
		return key;

	return language->table[idx].value;
}

char* naui_localization_format(const Naui_Language* language, const char* key, const char** args, int arg_count)
{
	if (!language || !key)
		return NULL;

	Naui_LanguageEntry* table = (Naui_LanguageEntry*)language->table;
	ptrdiff_t idx = naui_strmap_get_index(table, key);
	if (idx < 0)
		return NULL;

	const Naui_LanguageEntry* entry = &language->table[idx];

	if (!entry->is_interpolated)
		return NULL;

	const char* src = entry->value;
	size_t src_len = strlen(src);
	Naui_StringBuilder builder = NULL;
	naui_string_builder_reserve(&builder, src_len);
	size_t i = 0;
	while (i < src_len)
	{
		if (src[i] == '{')
		{
			const char* close = strchr(src + i, '}');
			if (close)
			{
				size_t name_len = (size_t)(close - (src + i + 1));
				bool matched = false;

				for (int a = 0; a < arg_count; a += 2)
				{
					const char* name = args[a];
					if (name && strlen(name) == name_len && memcmp(name, src + i + 1, name_len) == 0)
					{
						Naui_StringView val_view = { (char*)args[a + 1], strlen(args[a + 1]) };
						naui_string_builder_append_view(&builder, val_view);
						matched = true;
						break;
					}
				}

				if (!matched)
				{
					Naui_StringView placeholder_view = { (char*)(src + i), (size_t)(close - src + 1) - i };
					naui_string_builder_append_view(&builder, placeholder_view);
				}

				i = (size_t)(close - src) + 1;
				continue;
			}
		}

		naui_string_builder_append_char(&builder, src[i]);
		i++;
	}

	size_t final_len = (size_t)naui_string_builder_len(builder);
	char* result = (char*)malloc(final_len + 1);
	if (result)
	{
		if (final_len > 0)
			memcpy(result, builder, final_len);

		result[final_len] = '\0';
	}

	naui_string_builder_free(&builder);
	return result;
}

void naui_localization_free(Naui_Language* language)
{
	if (!language)
		return;

	naui_strmap_free(language->table);
	naui_arena_free(&language->arena);
}

void naui_localization_check_font(uint8_t font_index)
{
	if (!g_current_loaded)
		return;

	if (!naui_font_is_loaded(font_index))
	{
		naui_log(NAUI_LOG_WARNING, "Font %u is not loaded, can't check it against the language", (unsigned)font_index);
		return;
	}

	enum { MAX_REPORTED = 12 };
	uint32_t missing[MAX_REPORTED];
	uint32_t missing_distinct = 0;
	uint32_t missing_total = 0;
	const ptrdiff_t entry_count = naui_strmap_len(g_current_language.table);
	for (ptrdiff_t e = 0; e < entry_count; e++)
	{
		const char* text = g_current_language.table[e].value;
		const size_t length = strlen(text);

		size_t i = 0;
		while (i < length)
		{
			uint32_t cp = 0;
			size_t n = naui_utf8_next(text + i, length - i, &cp);
			if (n == 0)
			{
				i++;
				continue;
			}

			i += n;
			if (cp < 0x20 || cp == 0x7F || naui_font_has_codepoint(font_index, cp))
				continue;

			missing_total++;
			bool known = false;
			for (uint32_t k = 0; k < missing_distinct; k++)
			{
				if (missing[k] == cp)
				{
					known = true;
					break;
				}
			}

			if (!known && missing_distinct < MAX_REPORTED)
				missing[missing_distinct++] = cp;
		}
	}

	if (missing_total == 0)
		return;

	char list[MAX_REPORTED * 9 + 1];
	size_t w = 0;
	for (uint32_t k = 0; k < missing_distinct; k++)
	{
		w += (size_t)snprintf(list + w, sizeof(list) - w, "U+%04X ", (unsigned)missing[k]);
	}

	naui_log(NAUI_LOG_WARNING, "%u characters in %s-%s have no glyph in font %u and will show as '?'. Examples: %s. Load a font that covers this script.", missing_total, g_current_language.meta.language_code.data, g_current_language.meta.region_code.data, (unsigned)font_index, list);
}
#pragma endregion