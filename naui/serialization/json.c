#pragma region Static Functions
static Naui_JsonValue* arena_value(Naui_Json* json)
{
	return (Naui_JsonValue*)naui_arena_alloc(&json->_arena, sizeof(Naui_JsonValue));
}

static Naui_JsonValue* arena_values(Naui_Json* json, size_t count)
{
	return (Naui_JsonValue*)naui_arena_alloc(&json->_arena, count * sizeof(Naui_JsonValue));
}

static const char* arena_str(Naui_Json* json, const char* src, size_t len)
{
	char* dst = (char*)naui_arena_alloc(&json->_arena, len + 1);
	if (!dst)
		return NULL;

	memcpy(dst, src, len);
	dst[len] = '\0';
	return dst;
}

static bool build_object(Naui_Json* json, Naui_JsonReader* reader, Naui_JsonValue* obj);
static bool build_array(Naui_Json* json, Naui_JsonReader* reader, Naui_JsonValue* arr)
{
	arr->type = NAUI_JSON_ARRAY;
	arr->array.items = NULL;
	arr->array.count = 0;
	arr->array.cap = 0;

	while (1)
	{
		Naui_JsonToken t = naui_json_reader_next(reader);
		if (t == NAUI_JSON_TOKEN_ARRAY_END)
			return true;

		if (t == NAUI_JSON_TOKEN_ERROR || t == NAUI_JSON_TOKEN_EOF)
			return false;

		if (arr->array.count >= arr->array.cap)
		{
			size_t new_cap = arr->array.cap ? arr->array.cap * 2 : 8;
			Naui_JsonValue* tmp = arena_values(json, new_cap);
			if (!tmp)
				return false;

			if (arr->array.items)
				memcpy(tmp, arr->array.items, arr->array.count * sizeof(Naui_JsonValue));

			arr->array.items = tmp;
			arr->array.cap = new_cap;
		}

		Naui_JsonValue* slot = &arr->array.items[arr->array.count++];
		memset(slot, 0, sizeof(*slot));

		switch (t)
		{
			case NAUI_JSON_TOKEN_NULL:
				slot->type = NAUI_JSON_NULL;
				break;

			case NAUI_JSON_TOKEN_BOOL:
				slot->type = NAUI_JSON_BOOL;
				slot->boolean = reader->boolean;
				break;

			case NAUI_JSON_TOKEN_NUMBER:
				slot->type = NAUI_JSON_NUMBER;
				slot->number = reader->number;
				break;

			case NAUI_JSON_TOKEN_STRING:
				slot->type = NAUI_JSON_STRING;
				slot->string.ptr = reader->str;
				slot->string.len = reader->len;
				break;

			case NAUI_JSON_TOKEN_OBJECT_BEGIN:
				if (!build_object(json, reader, slot))
					return false;

				break;

			case NAUI_JSON_TOKEN_ARRAY_BEGIN:
				if (!build_array(json, reader, slot))
					return false;

				break;

			default:
				return false;
		}
	}
}

static bool build_object(Naui_Json* json, Naui_JsonReader* reader, Naui_JsonValue* obj)
{
	obj->type = NAUI_JSON_OBJECT;
	obj->object.pairs = NULL;
	obj->object.count = 0;
	obj->object.cap = 0;

	while (1)
	{
		Naui_JsonToken t = naui_json_reader_next(reader);
		if (t == NAUI_JSON_TOKEN_OBJECT_END)
			return true;

		if (t == NAUI_JSON_TOKEN_ERROR || t == NAUI_JSON_TOKEN_EOF)
			return false;

		if (t != NAUI_JSON_TOKEN_KEY)
			return false;

		if (obj->object.count + 2 > obj->object.cap)
		{
			size_t new_cap = obj->object.cap ? obj->object.cap * 2 : 16;
			Naui_JsonValue* tmp = arena_values(json, new_cap);
			if (!tmp)
				return false;

			if (obj->object.pairs)
				memcpy(tmp, obj->object.pairs, obj->object.count * sizeof(Naui_JsonValue));

			obj->object.pairs = tmp;
			obj->object.cap = new_cap;
		}

		Naui_JsonValue* key_slot = &obj->object.pairs[obj->object.count++];
		key_slot->type = NAUI_JSON_STRING;
		key_slot->string.ptr = reader->str;
		key_slot->string.len = reader->len;

		Naui_JsonValue* val_slot = &obj->object.pairs[obj->object.count++];
		memset(val_slot, 0, sizeof(*val_slot));
		t = naui_json_reader_next(reader);
		
		switch (t)
		{
			case NAUI_JSON_TOKEN_NULL:
				val_slot->type = NAUI_JSON_NULL;
				break;

			case NAUI_JSON_TOKEN_BOOL:
				val_slot->type = NAUI_JSON_BOOL;
				val_slot->boolean = reader->boolean;
				break;

			case NAUI_JSON_TOKEN_NUMBER:
				val_slot->type = NAUI_JSON_NUMBER;
				val_slot->number = reader->number;
				break;

			case NAUI_JSON_TOKEN_STRING:
				val_slot->type = NAUI_JSON_STRING;
				val_slot->string.ptr = reader->str;
				val_slot->string.len = reader->len;
				break;

			case NAUI_JSON_TOKEN_OBJECT_BEGIN:
				if (!build_object(json, reader, val_slot))
					return false;

				break;

			case NAUI_JSON_TOKEN_ARRAY_BEGIN:
				if (!build_array(json, reader, val_slot))
					return false;

				break;

			default:
				return false;
		}
	}
}

static void write_value(Naui_JsonWriter* writer, const Naui_JsonValue* value);

static void write_object(Naui_JsonWriter* writer, const Naui_JsonValue* value)
{
	naui_json_writer_object_begin(writer);
	for (size_t i = 0; i + 1 < value->object.count; i += 2)
	{
		const Naui_JsonValue* key = &value->object.pairs[i];
		const Naui_JsonValue* val = &value->object.pairs[i + 1];
		naui_json_writer_key_len(writer, key->string.ptr, key->string.len);
		write_value(writer, val);
	}

	naui_json_writer_object_end(writer);
}

static void write_array(Naui_JsonWriter* writer, const Naui_JsonValue* value)
{
	naui_json_writer_array_begin(writer);
	for (size_t i = 0; i < value->array.count; ++i)
	{
		write_value(writer, &value->array.items[i]);
	}

	naui_json_writer_array_end(writer);
}

static void write_value(Naui_JsonWriter* writer, const Naui_JsonValue* value)
{
	switch (value->type)
	{
		case NAUI_JSON_NULL:
			naui_json_writer_null(writer);
			break;

		case NAUI_JSON_BOOL:
			naui_json_writer_bool(writer, value->boolean);
			break;

		case NAUI_JSON_NUMBER:
			naui_json_writer_number(writer, value->number);
			break;

		case NAUI_JSON_STRING:
			naui_json_writer_string_len(writer, value->string.ptr, value->string.len);
			break;

		case NAUI_JSON_ARRAY:
			write_array(writer, value);
			break;

		case NAUI_JSON_OBJECT:
			write_object(writer, value);
			break;
	}
}

static Naui_JsonValue* object_slot(Naui_Json* json, Naui_JsonValue* obj, const char* key)
{
	if (!obj || obj->type != NAUI_JSON_OBJECT)
		return NULL;

	size_t key_len = strlen(key);
	for (size_t i = 0; i + 1 < obj->object.count; i += 2)
	{
		if (obj->object.pairs[i].string.len == key_len &&
		    memcmp(obj->object.pairs[i].string.ptr, key, key_len) == 0)
			return &obj->object.pairs[i + 1];
	}

	if (obj->object.count + 2 > obj->object.cap)
	{
		size_t new_cap = obj->object.cap ? obj->object.cap * 2 : 8;
		Naui_JsonValue* tmp = arena_values(json, new_cap);
		if (!tmp)
			return NULL;

		if (obj->object.pairs)
			memcpy(tmp, obj->object.pairs, obj->object.count * sizeof(Naui_JsonValue));

		obj->object.pairs = tmp;
		obj->object.cap = new_cap;
	}

	Naui_JsonValue* key_slot = &obj->object.pairs[obj->object.count++];
	key_slot->type = NAUI_JSON_STRING;
	key_slot->string.ptr = arena_str(json, key, key_len);
	key_slot->string.len = key_len;

	Naui_JsonValue* val_slot = &obj->object.pairs[obj->object.count++];
	memset(val_slot, 0, sizeof(*val_slot));
	return val_slot;
}

static Naui_JsonValue* array_slot(Naui_Json* json, Naui_JsonValue* arr)
{
	if (!arr || arr->type != NAUI_JSON_ARRAY)
		return NULL;

	if (arr->array.count >= arr->array.cap)
	{
		size_t new_cap = arr->array.cap ? arr->array.cap * 2 : 8;
		Naui_JsonValue* tmp = arena_values(json, new_cap);
		if (!tmp)
			return NULL;

		if (arr->array.items)
			memcpy(tmp, arr->array.items, arr->array.count * sizeof(Naui_JsonValue));

		arr->array.items = tmp;
		arr->array.cap = new_cap;
	}

	Naui_JsonValue* slot = &arr->array.items[arr->array.count++];
	memset(slot, 0, sizeof(*slot));
	return slot;
}

#define OBJECT_SET(json, obj, key, setup) \
	do { Naui_JsonValue* s = object_slot(json, obj, key); if (s) { setup; } } while(0)

#define ARRAY_PUSH(json, arr, setup) \
	do { Naui_JsonValue* s = array_slot(json, arr); if (s) { setup; } } while(0)

#pragma endregion

#pragma region Json Parser
Naui_Json naui_json_parse(const char* src, size_t len)
{
	Naui_Json result;
	memset(&result, 0, sizeof(result));

	Naui_JsonReader reader;
	naui_json_reader_init(&reader, src, len);

	Naui_JsonToken t = naui_json_reader_next(&reader);
	if (t == NAUI_JSON_TOKEN_EOF)
	{
		result.error = "empty input";
		return result;
	}

	if (t == NAUI_JSON_TOKEN_ERROR)
	{
		result.error = reader.error;
		result.error_line = reader.error_line;
		result.error_col = reader.error_col;
		return result;
	}

	Naui_JsonValue* root = arena_value(&result);
	if (!root)
	{
		result.error = "out of memory";
		return result;
	}

	bool ok = false;
	switch (t)
	{
		case NAUI_JSON_TOKEN_OBJECT_BEGIN:
			ok = build_object(&result, &reader, root);
			break;

		case NAUI_JSON_TOKEN_ARRAY_BEGIN:
			ok = build_array(&result, &reader, root);
			break;

		case NAUI_JSON_TOKEN_STRING:
			root->type = NAUI_JSON_STRING;
			root->string.ptr = reader.str;
			root->string.len = reader.len;
			ok = true;
			break;

		case NAUI_JSON_TOKEN_NUMBER:
			root->type = NAUI_JSON_NUMBER;
			root->number = reader.number;
			ok = true;
			break;

		case NAUI_JSON_TOKEN_BOOL:
			root->type = NAUI_JSON_BOOL;
			root->boolean = reader.boolean;
			ok = true;
			break;

		case NAUI_JSON_TOKEN_NULL:
			root->type = NAUI_JSON_NULL;
			ok = true;
			break;

		default:
			break;
	}

	if (!ok)
	{
		naui_json_free(&result);
		result.error = reader.error ? reader.error : "parse error";
		result.error_line = reader.error_line;
		result.error_col = reader.error_col;
		return result;
	}

	result.root = root;
	return result;
}

Naui_Json naui_json_parse_file(const Naui_Path path)
{
	Naui_Json result;
	memset(&result, 0, sizeof(result));

	size_t len;
	char* src = naui_file_read_all(path, &len);
	if (!src)
	{
		result.error = "failed to read file";
		return result;
	}

	result = naui_json_parse(src, len);
	if (result.root && !result.error)
		result._file_src = src;
	else
		free(src);

	return result;
}

void naui_json_free(Naui_Json* result)
{
	naui_arena_free(&result->_arena);
	free(result->_file_src);
	memset(result, 0, sizeof(*result));
}

Naui_JsonValue* naui_json_array_get(const Naui_JsonValue* array, size_t index)
{
	if (!array || array->type != NAUI_JSON_ARRAY)
		return NULL;

	if (index >= array->array.count)
		return NULL;

	return &array->array.items[index];
}

Naui_JsonValue* naui_json_object_get(const Naui_JsonValue* object, const char* key)
{
	if (!object || object->type != NAUI_JSON_OBJECT || !key)
		return NULL;

	size_t key_len = strlen(key);
	for (size_t i = 0; i + 1 < object->object.count; i += 2)
	{
		const Naui_JsonValue* k = &object->object.pairs[i];
		if (k->string.len == key_len && memcmp(k->string.ptr, key, key_len) == 0)
			return (Naui_JsonValue*)&object->object.pairs[i + 1];
	}

	return NULL;
}

bool naui_json_is_null(const Naui_JsonValue* value)
{
	return !value || value->type == NAUI_JSON_NULL;
}

bool naui_json_get_bool(const Naui_JsonValue* value, bool default_value)
{
	if (!value || value->type != NAUI_JSON_BOOL)
		return default_value;

	return value->boolean;
}

double naui_json_get_number(const Naui_JsonValue* value, double default_value)
{
	if (!value || value->type != NAUI_JSON_NUMBER)
		return default_value;

	return value->number;
}

int naui_json_get_int(const Naui_JsonValue* value, int default_value)
{
	if (!value || value->type != NAUI_JSON_NUMBER)
		return default_value;

	return (int)value->number;
}

const char* naui_json_get_string(const Naui_JsonValue* value, const char* default_value)
{
	if (!value || value->type != NAUI_JSON_STRING)
		return default_value;

	return value->string.ptr;
}

int naui_json_copy_string(const Naui_JsonValue* value, Naui_String* out_string)
{
    if (!value || value->type != NAUI_JSON_STRING)
        return -1;

    Naui_JsonReader tmp;
    naui_json_reader_init(&tmp, value->string.ptr, value->string.len);
    tmp.token = NAUI_JSON_TOKEN_STRING;
    tmp.str = value->string.ptr;
    tmp.len = value->string.len;
    out_string->length = (size_t)naui_json_reader_copy_str(&tmp, out_string->data, NAUI_STRING_MAX_SIZE);
    out_string->data[NAUI_STRING_MAX_SIZE - 1] = '\0';
    return (int)out_string->length;
}

int naui_json_copy_cstr(const Naui_JsonValue* value, char* dest, size_t dest_size)
{
    if (!value || value->type != NAUI_JSON_STRING || !dest || dest_size == 0)
        return -1;

    Naui_JsonReader tmp;
    naui_json_reader_init(&tmp, value->string.ptr, value->string.len);
    tmp.token = NAUI_JSON_TOKEN_STRING;
    tmp.str = value->string.ptr;
    tmp.len = value->string.len;
    int written = naui_json_reader_copy_str(&tmp, dest, dest_size);
    dest[dest_size - 1] = '\0';
    return written;
}

Naui_Json naui_json_result_create(void)
{
	Naui_Json json;
	memset(&json, 0, sizeof(json));
	return json;
}

Naui_JsonValue* naui_json_object(Naui_Json* json)
{
	Naui_JsonValue* value = arena_value(json);
	if (!value)
		return NULL;

	value->type = NAUI_JSON_OBJECT;
	json->root = value;
	return value;
}

Naui_JsonValue* naui_json_array(Naui_Json* json)
{
	Naui_JsonValue* value = arena_value(json);
	if (!value)
		return NULL;

	value->type = NAUI_JSON_ARRAY;
	json->root = value;
	return value;
}

void naui_json_set_null(Naui_Json* json, Naui_JsonValue* obj, const char* key)
{
	OBJECT_SET(json, obj, key, s->type = NAUI_JSON_NULL);
}

void naui_json_set_bool(Naui_Json* json, Naui_JsonValue* obj, const char* key, bool value)
{
	OBJECT_SET(json, obj, key, s->type = NAUI_JSON_BOOL; s->boolean = value);
}

void naui_json_set_number(Naui_Json* json, Naui_JsonValue* obj, const char* key, double value)
{
	OBJECT_SET(json, obj, key, s->type = NAUI_JSON_NUMBER; s->number = value);
}

void naui_json_set_int(Naui_Json* json, Naui_JsonValue* obj, const char* key, int value)
{
	naui_json_set_number(json, obj, key, (double)value);
}

void naui_json_set_string(Naui_Json* json, Naui_JsonValue* obj, const char* key, const char* value)
{
	size_t len = strlen(value);
	OBJECT_SET(json, obj, key,
		s->type = NAUI_JSON_STRING;
		s->string.ptr = arena_str(json, value, len);
		s->string.len = len
	);
}

Naui_JsonValue* naui_json_set_object(Naui_Json* json, Naui_JsonValue* obj, const char* key)
{
	Naui_JsonValue* s = object_slot(json, obj, key);
	if (!s)
		return NULL;

	s->type = NAUI_JSON_OBJECT;
	return s;
}

Naui_JsonValue* naui_json_set_array(Naui_Json* json, Naui_JsonValue* obj, const char* key)
{
	Naui_JsonValue* s = object_slot(json, obj, key);
	if (!s)
		return NULL;

	s->type = NAUI_JSON_ARRAY;
	return s;
}

void naui_json_push_null(Naui_Json* json, Naui_JsonValue* arr)
{
	ARRAY_PUSH(json, arr, s->type = NAUI_JSON_NULL);
}

void naui_json_push_bool(Naui_Json* json, Naui_JsonValue* arr, bool value)
{
	ARRAY_PUSH(json, arr, s->type = NAUI_JSON_BOOL; s->boolean = value);
}

void naui_json_push_number(Naui_Json* json, Naui_JsonValue* arr, double value)
{
	ARRAY_PUSH(json, arr, s->type = NAUI_JSON_NUMBER; s->number = value);
}

void naui_json_push_int(Naui_Json* json, Naui_JsonValue* arr, int value)
{
	naui_json_push_number(json, arr, (double)value);
}

void naui_json_push_string(Naui_Json* json, Naui_JsonValue* arr, const char* value)
{
	size_t len = strlen(value);
	ARRAY_PUSH(json, arr,
		s->type = NAUI_JSON_STRING;
		s->string.ptr = arena_str(json, value, len);
		s->string.len = len
	);
}

Naui_JsonValue* naui_json_push_object(Naui_Json* json, Naui_JsonValue* arr)
{
	Naui_JsonValue* slot = array_slot(json, arr);
	if (!slot)
		return NULL;

	slot->type = NAUI_JSON_OBJECT;
	return slot;
}

Naui_JsonValue* naui_json_push_array(Naui_Json* json, Naui_JsonValue* arr)
{
	Naui_JsonValue* slot = array_slot(json, arr);
	if (!slot)
		return NULL;

	slot->type = NAUI_JSON_ARRAY;
	return slot;
}

int naui_json_write(const Naui_JsonValue* root, char* dest, size_t dest_size, bool is_pretty)
{
	if (!root)
		return -1;

	if (!dest)
	{
		Naui_JsonWriter writer;
		naui_json_writer_init_heap(&writer, is_pretty);
		write_value(&writer, root);
		size_t len;
		char* buf = naui_json_writer_finish_heap(&writer, &len);
		if (!buf)
			return -1;

		free(buf);
		return (int)len;
	}

	Naui_JsonWriter writer;
	naui_json_writer_init(&writer, dest, dest_size, is_pretty);
	write_value(&writer, root);
	return naui_json_writer_finish(&writer);
}

bool naui_json_write_file(const Naui_JsonValue* root, const Naui_Path path, bool is_pretty)
{
	if (!root)
		return false;

	Naui_JsonWriter writer;
	naui_json_writer_init_heap(&writer, is_pretty);
	write_value(&writer, root);

	size_t len;
	char* buf = naui_json_writer_finish_heap(&writer, &len);
	if (!buf)
		return false;

	bool ok = naui_file_write_all(path, buf, len);
	free(buf);
	return ok;
}
#pragma endregion
