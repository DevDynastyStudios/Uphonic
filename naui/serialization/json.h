#define NAUI_JSON_FOREACH(node, k, v) \
    for (size_t _i = 0; (node) && (((node)->type == NAUI_JSON_ARRAY  && _i < (node)->array.count) || ((node)->type == NAUI_JSON_OBJECT && _i + 1 < (node)->object.count)); _i += (node)->type == NAUI_JSON_OBJECT ? 2 : 1) \
    for (const Naui_JsonValue* k = (node)->type == NAUI_JSON_OBJECT ? &(node)->object.pairs[_i] : NULL, * v = (node)->type == NAUI_JSON_OBJECT ? &(node)->object.pairs[_i + 1] : &(node)->array.items[_i]; v; k = NULL, v = NULL)

typedef uint8_t Naui_JsonType;
enum
{
	NAUI_JSON_NULL,
	NAUI_JSON_BOOL,
	NAUI_JSON_NUMBER,
	NAUI_JSON_STRING,
	NAUI_JSON_ARRAY,
	NAUI_JSON_OBJECT
};

typedef struct Naui_JsonValue Naui_JsonValue;
struct Naui_JsonValue
{
	Naui_JsonType type;
	union
	{
		bool boolean;
		double number;
		struct
		{
			const char* ptr;
			size_t len;
		} string;

		struct
		{
			Naui_JsonValue* items;
			size_t count;
			size_t cap;
		} array;

		struct
		{
			Naui_JsonValue* pairs;
			size_t count;
			size_t cap;
		} object;
	};
};

typedef struct
{
	Naui_JsonValue* root;
	const char* error;
	int error_line;
	int error_col;

	Naui_Arena _arena;
	char* _file_src;
} Naui_Json;

Naui_Json naui_json_parse(const char* src, size_t len);
Naui_Json naui_json_parse_file(const Naui_Path path);
void naui_json_free(Naui_Json* result);

Naui_JsonValue* naui_json_array_get(const Naui_JsonValue* array, size_t index);
Naui_JsonValue* naui_json_object_get(const Naui_JsonValue* object, const char* key);

bool naui_json_is_null(const Naui_JsonValue* v);
bool naui_json_get_bool(const Naui_JsonValue* v, bool default_value);
double naui_json_get_number(const Naui_JsonValue* v, double default_value);
int naui_json_get_int(const Naui_JsonValue* v, int default_value);
const char* naui_json_get_string(const Naui_JsonValue* v, const char* default_value);


/* Copies token to Naui_String.
 * Returns bytes written or -1 on type mismatch. */
int naui_json_copy_string(const Naui_JsonValue* value, Naui_String* out_string);

/*
 * Copy a string to destination. Null-terminated and with escape sequences resolved.
 * Returns bytes written or -1 on type mismatch.
 */
int naui_json_copy_cstr(const Naui_JsonValue* v, char* dest, size_t dest_size);

Naui_Json naui_json_result_create(void);
Naui_JsonValue* naui_json_object(Naui_Json* json);
Naui_JsonValue* naui_json_array(Naui_Json* json);

void naui_json_set_null(Naui_Json* json, Naui_JsonValue* obj, const char* key);
void naui_json_set_bool(Naui_Json* json, Naui_JsonValue* obj, const char* key, bool value);
void naui_json_set_number(Naui_Json* json, Naui_JsonValue* obj, const char* key, double value);
void naui_json_set_int(Naui_Json* json, Naui_JsonValue* obj, const char* key, int value);
void naui_json_set_string(Naui_Json* json, Naui_JsonValue* obj, const char* key, const char* value);
Naui_JsonValue* naui_json_set_object(Naui_Json* json, Naui_JsonValue* obj, const char* key);
Naui_JsonValue* naui_json_set_array(Naui_Json* json, Naui_JsonValue* obj, const char* key);

void naui_json_push_null(Naui_Json* json, Naui_JsonValue* arr);
void naui_json_push_bool(Naui_Json* json, Naui_JsonValue* arr, bool value);
void naui_json_push_number(Naui_Json* json, Naui_JsonValue* arr, double value);
void naui_json_push_int(Naui_Json* json, Naui_JsonValue* arr, int value);
void naui_json_push_string(Naui_Json* json, Naui_JsonValue* arr, const char* value);
Naui_JsonValue* naui_json_push_object(Naui_Json* json, Naui_JsonValue* arr);
Naui_JsonValue* naui_json_push_array(Naui_Json* json, Naui_JsonValue* arr);

/*
 * Serialize to a fixed buffer. Returns bytes written or -1.
 * Pass NULL to measure the required size first.
 */
int naui_json_write(const Naui_JsonValue* root, char* dest, size_t dest_size, bool pretty);
bool naui_json_write_file(const Naui_JsonValue* root, const Naui_Path path, bool pretty);
