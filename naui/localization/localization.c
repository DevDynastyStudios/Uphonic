#define NAUI_LOCALIZATION_NAME_SIZE 128

static Naui_Language g_current_language;
static Naui_List(Naui_LanguageMeta) g_meta_cache = NULL;
static bool g_current_loaded = false;
static bool g_meta_cache_init = false;

static char* naui_strdup_(const char* s)
{
	if (!s)
		return NULL;

	size_t len = strlen(s) + 1;
	char* copy = (char*)malloc(len);
	if (copy)
		memcpy(copy, s, len);

	return copy;
}

static Naui_TextDirection naui_parse_direction_(const char* dir)
{
	if (dir && strcmp(dir, "rtl") == 0)
		return NAUI_TEXT_RTL;

	return NAUI_TEXT_LTR;
}

static void naui_split_locale_(const char* code, char** out_lang, char** out_region)
{
	*out_lang = NULL;
	*out_region = NULL;

	if (!code)
		return;

	const char* dash = strchr(code, '-');
	if (!dash)
	{
		*out_lang = naui_strdup_(code);
		return;
	}

	size_t lang_len = (size_t)(dash - code);
	char* lang = (char*)malloc(lang_len + 1);
	if (lang)
	{
		memcpy(lang, code, lang_len);
		lang[lang_len] = '\0';
	}

	*out_lang = lang;
	*out_region = naui_strdup_(dash + 1);
}

static void naui_meta_free_(Naui_LanguageMeta* meta)
{
	if (!meta)
		return;

	free(meta->filename);
	free(meta->language_code);
	free(meta->region_code);
	free(meta->display_name);
	memset(meta, 0, sizeof(*meta));
}

static void naui_meta_copy_(Naui_LanguageMeta* dst, const Naui_LanguageMeta* src)
{
	dst->filename = naui_strdup_(src->filename);
	dst->language_code = naui_strdup_(src->language_code);
	dst->region_code = naui_strdup_(src->region_code);
	dst->display_name = naui_strdup_(src->display_name);
	dst->text_direction = src->text_direction;
}

static Naui_Path naui_localization_build_path_(const char* code)
{
	Naui_Path bin_dir = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, naui_path_from_cstr("Language"));

	char filename[NAUI_LOCALIZATION_NAME_SIZE];
	snprintf(filename, sizeof(filename), "%s.lang", code);

	Naui_Path result = naui_path_join(lang_dir, naui_path_from_cstr(filename));
	return result;
}

bool naui_localization_load_file(const char* path, Naui_Language* out_language)
{
	if (!path || !out_language)
		return false;

	memset(out_language, 0, sizeof(*out_language));

	Naui_Path json_path = naui_path_from_cstr(path);
	Naui_Json json = naui_json_parse_file(json_path);

	if (!json.root || json.error)
	{
		naui_json_free(&json);
		return false;
	}

	if (json.root->type != NAUI_JSON_OBJECT)
	{
		naui_json_free(&json);
		return false;
	}

	out_language->table = NULL;
	out_language->meta.filename = naui_strdup_(path);
	out_language->meta.language_code = NULL;
	out_language->meta.region_code = NULL;
	out_language->meta.display_name = NULL;
	out_language->meta.text_direction = NAUI_TEXT_LTR;

	NAUI_JSON_FOREACH(json.root, key, value)
	{
		char key_buf[256];
		size_t key_len = key->string.len;
		if (key_len >= sizeof(key_buf))
			key_len = sizeof(key_buf) - 1;

		memcpy(key_buf, key->string.ptr, key_len);
		key_buf[key_len] = '\0';

		if (strcmp(key_buf, "_meta") == 0)
		{
			if (value->type != NAUI_JSON_OBJECT)
				continue;
		
			const Naui_JsonValue* lang_v = naui_json_object_get(value, "language");
			const Naui_JsonValue* dir_v = naui_json_object_get(value, "direction");
			const Naui_JsonValue* name_v = naui_json_object_get(value, "name");
		
			if (lang_v && lang_v->type == NAUI_JSON_STRING)
			{
				char lang_code_buf[64];
				naui_json_copy_string(lang_v, lang_code_buf, sizeof(lang_code_buf));
				free(out_language->meta.language_code);
				free(out_language->meta.region_code);
				naui_split_locale_(lang_code_buf, &out_language->meta.language_code, &out_language->meta.region_code);
			}
		
			if (dir_v && dir_v->type == NAUI_JSON_STRING)
			{
				char dir_buf[16];
				naui_json_copy_string(dir_v, dir_buf, sizeof(dir_buf));
				out_language->meta.text_direction = naui_parse_direction_(dir_buf);
			}
		
			if (name_v && name_v->type == NAUI_JSON_STRING)
			{
				char name_buf[256];
				naui_json_copy_string(name_v, name_buf, sizeof(name_buf));
				free(out_language->meta.display_name);
				out_language->meta.display_name = naui_strdup_(name_buf);
			}
		
			continue;
		}

		if (value->type != NAUI_JSON_STRING)
			continue;

		size_t val_len = value->string.len;
		bool is_interpolated = (val_len > 0 && value->string.ptr[0] == '$');
		const char* val_start = value->string.ptr;
		size_t copy_len = val_len;

		if (is_interpolated)
		{
			val_start += 1;
			copy_len -= 1;
		}

		char* entry_value = (char*)malloc(copy_len + 1);
		if (!entry_value)
			continue;

		memcpy(entry_value, val_start, copy_len);
		entry_value[copy_len] = '\0';
		char* entry_key = (char*)malloc(key_len + 1);
		if (!entry_key)
		{
			free(entry_value);
			continue;
		}

		memcpy(entry_key, key_buf, key_len + 1);
		Naui_LanguageEntry entry;
		entry.key = entry_key;
		entry.value = entry_value;
		entry.is_interpolated = is_interpolated;
		naui_strmap_puts(out_language->table, entry);
	}

	naui_json_free(&json);
	if (!out_language->meta.language_code)
	{
		const char* base = strrchr(path, '/');
		if (!base)
			base = strrchr(path, '\\');
		base = base ? base + 1 : path;

		char derived[64];
		size_t i = 0;
		while (base[i] && base[i] != '.' && i < sizeof(derived) - 1)
		{
			derived[i] = base[i];
			++i;
		}

		derived[i] = '\0';
		naui_split_locale_(derived, &out_language->meta.language_code, &out_language->meta.region_code);
	}

	return true;
}

bool naui_localization_load(const char* language_code, Naui_Language* out_language)
{
	if (!language_code || !out_language)
		return false;

	Naui_Path path = naui_localization_build_path_(language_code);
	bool ok = naui_localization_load_file(path.data, out_language);
	return ok;
}

void naui_localization_set_current(const char* language_code)
{
	Naui_Language new_lang;
	bool ok = naui_localization_load(language_code, &new_lang);

	if (!ok && (!language_code || strcmp(language_code, "en-US") != 0))
		ok = naui_localization_load("en-US", &new_lang);

	if (!ok)
		return;

	if (g_current_loaded)
		naui_localization_free(&g_current_language);

	g_current_language = new_lang;
	g_current_loaded = true;
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
		for (ptrdiff_t i = 0; i < naui_list_len(g_meta_cache); ++i)
		{
			naui_meta_free_(&g_meta_cache[i]);
		}

		naui_list_free(g_meta_cache);
		g_meta_cache = NULL;
	}

	g_meta_cache_init = true;
	Naui_Path bin_dir = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, naui_path_from_cstr("Language"));

	if (!naui_path_exists(lang_dir))
		return;

	const char* extensions[] = { ".lang", NULL };
	Naui_List(Naui_DirEntry) entries = naui_directory_filter(lang_dir, NULL, extensions, 1);
	for (ptrdiff_t i = 0; i < naui_list_len(entries); ++i)
	{
		if (entries[i].is_directory)
			continue;

		const char* full_path = entries[i].path.data;

		Naui_Language lang;
		if (!naui_localization_load_file(full_path, &lang))
			continue;

		Naui_LanguageMeta meta;
		naui_meta_copy_(&meta, &lang.meta);
		free(meta.filename);
		meta.filename = naui_strdup_(full_path);
		naui_list_push(g_meta_cache, meta);
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
	size_t out_len = 0;
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
						out_len += strlen(args[a + 1]);
						matched = true;
						break;
					}
				}

				if (!matched)
					out_len += (size_t)(close - src + 1) - i;

				i = (size_t)(close - src) + 1;
				continue;
			}
		}

		++out_len;
		++i;
	}

	char* result = (char*)malloc(out_len + 1);
	if (!result)
		return NULL;

	size_t out_pos = 0;
	i = 0;
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
						const char* val = args[a + 1];
						size_t val_len = strlen(val);
						memcpy(result + out_pos, val, val_len);
						out_pos += val_len;
						matched = true;
						break;
					}
				}

				if (!matched)
				{
					size_t span = (size_t)(close - src + 1) - i;
					memcpy(result + out_pos, src + i, span);
					out_pos += span;
				}

				i = (size_t)(close - src) + 1;
				continue;
			}
		}

		result[out_pos++] = src[i];
		++i;
	}

	result[out_pos] = '\0';
	return result;
}

void naui_localization_free(Naui_Language* language)
{
	if (!language)
		return;

	for (ptrdiff_t i = 0; i < naui_strmap_len(language->table); ++i)
	{
		free(language->table[i].key);
		free(language->table[i].value);
	}

	naui_strmap_free(language->table);
	language->table = NULL;
	naui_meta_free_(&language->meta);
}