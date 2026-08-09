#define NAUI_JSON_WRITER_MAX_DEPTH 64

typedef struct
{
	char* buf;
	size_t buf_size;
	size_t written;
	bool is_pretty;
	bool has_error;

	bool _heap;
	uint8_t _depth;
	bool _needs_comma[NAUI_JSON_WRITER_MAX_DEPTH];
	bool _in_object[NAUI_JSON_WRITER_MAX_DEPTH];
} Naui_JsonWriter;

/* Fixed size write buffer.
 * Error if output is truncated. */
void naui_json_writer_init(Naui_JsonWriter* w, char* buf, size_t buf_size, bool is_pretty);

/* Initializes the buffer.
 * Grows as needed. */
void naui_json_writer_init_heap(Naui_JsonWriter* w, bool is_pretty);

void naui_json_writer_object_begin(Naui_JsonWriter* w);
void naui_json_writer_object_end(Naui_JsonWriter* w);
void naui_json_writer_array_begin(Naui_JsonWriter* w);
void naui_json_writer_array_end(Naui_JsonWriter* w);

/* Must be called before each value inside an object. */
void naui_json_writer_key(Naui_JsonWriter* w, const char* key);
void naui_json_writer_key_len(Naui_JsonWriter* w, const char* key, size_t len);

void naui_json_writer_string(Naui_JsonWriter* w, const char* value);
void naui_json_writer_string_len(Naui_JsonWriter* w, const char* value, size_t len);
void naui_json_writer_number(Naui_JsonWriter* w, double value);
void naui_json_writer_int(Naui_JsonWriter* w, int value);
void naui_json_writer_uint(Naui_JsonWriter* w, unsigned int value);
void naui_json_writer_int64(Naui_JsonWriter* w, int64_t value);
void naui_json_writer_uint64(Naui_JsonWriter* w, uint64_t value);
void naui_json_writer_bool(Naui_JsonWriter* w, bool value);
void naui_json_writer_null(Naui_JsonWriter* w);

/* Returns bytes written or -1 on error/truncation. */
int naui_json_writer_finish(Naui_JsonWriter* w);

/* Returns the buffer, NULL on error. */
char* naui_json_writer_finish_heap(Naui_JsonWriter* w, size_t* out_len);
