#define NAUI_LOCALIZATION_NAME_SIZE 128
#define NAUI_LOCALIZATION_FOLDER_NAME "Localization"
#define NAUI_LOCALIZATION_EXTENSION ".lang"

static Naui_Language g_current_language;
static Naui_List(Naui_LanguageMeta) g_meta_cache = NULL;
static bool g_current_loaded = false;
static bool g_meta_cache_init = false;

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

bool naui_localization_load_file(const Naui_Path path, Naui_Language* out_language)
{
	if (naui_path_is_empty(path) || !out_language)
		return false;

	out_language->table = NULL;
	out_language->meta.language_code = (Naui_String){0};
	Naui_Json json = naui_json_parse_file(path);

	if (!json.root || json.error)
	{
		naui_log(NAUI_LOG_ERROR, "Failed to parse localization file: %s", path.data);
		naui_json_free(&json);
		return false;
	}

	if (json.root->type != NAUI_JSON_OBJECT)
	{
		naui_log(NAUI_LOG_ERROR, "Localization file root is not an object: %s", path.data);
		naui_json_free(&json);
		return false;
	}

	size_t file_size = naui_file_size(path);
	naui_arena_init(&out_language->arena, file_size > 0 ? file_size : 1024);
	out_language->meta.filename = path;
	out_language->meta.text_direction = NAUI_TEXT_LTR;

	NAUI_JSON_FOREACH(json.root, key, value)
	{
		if (strcmp(key->string.ptr, "_meta") == 0)
		{
			if (value->type != NAUI_JSON_OBJECT)
				continue;

			const Naui_JsonValue* lang_v = naui_json_object_get(value, "language");
			const Naui_JsonValue* dir_v = naui_json_object_get(value, "direction");
			const Naui_JsonValue* name_v = naui_json_object_get(value, "name");
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

			continue;
		}

		if (value->type != NAUI_JSON_STRING)
			continue;

		size_t val_cap = value->string.len + 1;
		char* raw_value = (char*)naui_arena_alloc(&out_language->arena, val_cap);
		if (!raw_value)
			continue;

		naui_json_copy_cstr(value, raw_value, val_cap);
		bool is_interpolated = (raw_value[0] == '$');
		char* entry_value = is_interpolated ? raw_value + 1 : raw_value;

		char* entry_key = (char*)naui_arena_alloc(&out_language->arena, key->string.len + 1);
		if (!entry_key)
			continue;

		naui_json_copy_cstr(key, entry_key, key->string.len + 1);
		Naui_LanguageEntry entry;
		entry.key = entry_key;
		entry.value = entry_value;
		entry.is_interpolated = is_interpolated;
		naui_strmap_puts(out_language->table, entry);
	}

	naui_json_free(&json);
	if (naui_string_is_empty(out_language->meta.language_code))
	{
		Naui_String stem = naui_view_to_string(naui_file_stem(&path));
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

void naui_localization_set_current(const Naui_String language_code)
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
			return;
		}

		naui_log(NAUI_LOG_ERROR, "Unable to load localization file for '%s'. Falling back to en-US", naui_string_is_empty(language_code) ? "" : language_code.data);
		ok = naui_localization_load(naui_string_from_cstr("en-US"), &new_lang);
		if (!ok)
		{
			Naui_String language_folder = naui_string_from_cstr(naui_directory_get(NAUI_DIR_ASSETS).data);
			naui_string_append(&language_folder, naui_string_from_cstr(NAUI_LOCALIZATION_FOLDER_NAME));
			naui_log(NAUI_LOG_ERROR, "Failed all attempts to load language file at: %s", language_folder);
			return;
		}
	}

	if (g_current_loaded)
		naui_localization_free(&g_current_language);

	g_current_language = new_lang;
	g_current_loaded = true;
	naui_log(NAUI_LOG_INFO, "Successfully loaded: %s-%s", new_lang.meta.language_code.data, new_lang.meta.region_code.data);
}

void naui_localization_set_current_lang(Naui_Language* lang)
{
	if (!lang)
		return;

	if (g_current_loaded)
		naui_localization_free(&g_current_language);

	g_current_language = *lang;
	g_current_loaded = true;
	memset(lang, 0, sizeof(*lang));
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
	for (ptrdiff_t i = 0; i < naui_list_len(entries); ++i)
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
	naui_string_builder_reserve(builder, src_len);
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
						naui_string_builder_append_view(builder, val_view);
						matched = true;
						break;
					}
				}

				if (!matched)
				{
					Naui_StringView placeholder_view = { (char*)(src + i), (size_t)(close - src + 1) - i };
					naui_string_builder_append_view(builder, placeholder_view);
				}

				i = (size_t)(close - src) + 1;
				continue;
			}
		}

		naui_string_builder_append_char(builder, src[i]);
		++i;
	}

	size_t final_len = (size_t)naui_string_builder_len(builder);
	char* result = (char*)malloc(final_len + 1);
	if (result)
	{
		memcpy(result, builder, final_len);
		result[final_len] = '\0';
	}

	naui_string_builder_free(builder);
	return result;
}

void naui_localization_free(Naui_Language* language)
{
	if (!language)
		return;

	naui_strmap_free(language->table);
	naui_arena_free(&language->arena);
}