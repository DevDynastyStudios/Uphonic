typedef uint8_t Naui_JsonToken;
enum
{
	NAUI_JSON_TOKEN_NONE,
	NAUI_JSON_TOKEN_OBJECT_BEGIN,
	NAUI_JSON_TOKEN_OBJECT_END,
	NAUI_JSON_TOKEN_ARRAY_BEGIN,
	NAUI_JSON_TOKEN_ARRAY_END,
	NAUI_JSON_TOKEN_KEY,
	NAUI_JSON_TOKEN_STRING,
	NAUI_JSON_TOKEN_NUMBER,
	NAUI_JSON_TOKEN_BOOL,
	NAUI_JSON_TOKEN_NULL,
	NAUI_JSON_TOKEN_EOF,
	NAUI_JSON_TOKEN_ERROR
};

#define NAUI_JSON_READER_MAX_DEPTH 64

typedef struct
{
	Naui_JsonToken token;
	const char* str;
	size_t len;
	double number;
	bool boolean;
	const char* error;
	int error_line;
	int error_col;

	const char* _src;
	const char* _cursor;
	const char* _end;
	int _line;
	int _col;
	uint8_t _depth;
	bool _needs_comma[NAUI_JSON_READER_MAX_DEPTH];
	bool _in_object[NAUI_JSON_READER_MAX_DEPTH];
	bool _expect_key;
} Naui_JsonReader;

void naui_json_reader_init(Naui_JsonReader* reader, const char* src, size_t len);
void naui_json_reader_skip(Naui_JsonReader* reader);
int naui_json_reader_copy_str(const Naui_JsonReader* reader, char* dest, size_t dest_size);
Naui_JsonToken naui_json_reader_next(Naui_JsonReader* reader);
