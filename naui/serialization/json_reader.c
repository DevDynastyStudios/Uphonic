#pragma region Static Functions
static bool is_whitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

static void advance(Naui_JsonReader* r)
{
	if (r->_cursor >= r->_end)
		return;

	if (*r->_cursor == '\n')
	{
		++r->_line;
		r->_col = 1;
	}
	else
		++r->_col;

	++r->_cursor;
}

static char peek(const Naui_JsonReader* r)
{
	if (r->_cursor >= r->_end)
		return '\0';

	return *r->_cursor;
}

static void skip_whitespace(Naui_JsonReader* r)
{
	while (r->_cursor < r->_end && is_whitespace(*r->_cursor))
	{
		advance(r);
	}
}

static Naui_JsonToken set_error(Naui_JsonReader* r, const char* msg)
{
	r->token = NAUI_JSON_TOKEN_ERROR;
	r->error = msg;
	r->error_line = r->_line;
	r->error_col = r->_col;
	return NAUI_JSON_TOKEN_ERROR;
}

static Naui_JsonToken parse_string(Naui_JsonReader* r, Naui_JsonToken token_type)
{
	advance(r);
	r->str = r->_cursor;

	while (r->_cursor < r->_end)
	{
		char c = *r->_cursor;
		if (c == '"')
		{
			r->len = (size_t)(r->_cursor - r->str);
			r->token = token_type;
			advance(r);
			return token_type;
		}

		if (c == '\\')
		{
			advance(r);
			if (r->_cursor >= r->_end)
				return set_error(r, "unexpected end of input in string escape");

			char esc = *r->_cursor;
			if (esc != '"' && esc != '\\' && esc != '/' && esc != 'b' && esc != 'f' && esc != 'n' && esc != 'r' && esc != 't' && esc != 'u')
				return set_error(r, "invalid escape sequence");
		}
		else if ((unsigned char)c < 0x20)
			return set_error(r, "unescaped control character in string");

		advance(r);
	}

	return set_error(r, "unterminated string");
}

static Naui_JsonToken parse_number(Naui_JsonReader* r)
{
	const char* start = r->_cursor;

	if (peek(r) == '-')
		advance(r);

	if (!is_digit(peek(r)))
		return set_error(r, "invalid number");

	while (is_digit(peek(r)))
	{
		advance(r);
	}

	if (peek(r) == '.')
	{
		advance(r);
		if (!is_digit(peek(r)))
			return set_error(r, "invalid number: expected digit after '.'");

		while (is_digit(peek(r)))
		{
			advance(r);
		}
	}

	if (peek(r) == 'e' || peek(r) == 'E')
	{
		advance(r);
		if (peek(r) == '+' || peek(r) == '-')
			advance(r);

		if (!is_digit(peek(r)))
			return set_error(r, "invalid number: expected digit in exponent");

		while (is_digit(peek(r)))
		{
			advance(r);
		}
	}

	r->str = start;
	r->len = (size_t)(r->_cursor - start);
	r->number = strtod(start, NULL);
	r->token = NAUI_JSON_TOKEN_NUMBER;
	return NAUI_JSON_TOKEN_NUMBER;
}

static Naui_JsonToken parse_literal(Naui_JsonReader* r, const char* word, size_t word_len, Naui_JsonToken token_type, bool bool_value)
{
	if ((size_t)(r->_end - r->_cursor) < word_len)
		return set_error(r, "unexpected end of input");

	if (memcmp(r->_cursor, word, word_len) != 0)
		return set_error(r, "invalid literal");

	if (r->_cursor + word_len < r->_end)
	{
		char next = r->_cursor[word_len];
		if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_')
			return set_error(r, "invalid literal");
	}

	r->str = r->_cursor;
	r->len = word_len;
	r->boolean = bool_value;
	r->token = token_type;

	for (size_t i = 0; i < word_len; ++i)
	{
		advance(r);
	}

	return token_type;
}

static void reader_push_scope(Naui_JsonReader* r, bool is_object)
{
	if (r->_depth < NAUI_JSON_READER_MAX_DEPTH)
	{
		r->_needs_comma[r->_depth] = false;
		r->_in_object[r->_depth] = is_object;
		++r->_depth;
	}
}

static void reader_pop_scope(Naui_JsonReader* r)
{
	if (r->_depth > 0)
		--r->_depth;
}

/* Returns false and sets error if a comma was expected but not found. */
static bool consume_comma_if_needed(Naui_JsonReader* r)
{
	if (r->_depth == 0)
		return true;

	uint8_t d = r->_depth - 1;
	if (!r->_needs_comma[d])
		return true;

	skip_whitespace(r);
	if (peek(r) != ',')
		return false;

	advance(r);
	return true;
}

static void mark_needs_comma(Naui_JsonReader* r)
{
	if (r->_depth > 0)
		r->_needs_comma[r->_depth - 1] = true;
}
#pragma endregion

#pragma region Json Reader
void naui_json_reader_init(Naui_JsonReader* reader, const char* src, size_t len)
{
	memset(reader, 0, sizeof(*reader));
	reader->_src = src;
	reader->_cursor = src;
	reader->_end = src + len;
	reader->_line = 1;
	reader->_col = 1;
	reader->token = NAUI_JSON_TOKEN_NONE;
}

void naui_json_reader_skip(Naui_JsonReader* reader)
{
	Naui_JsonToken t = reader->token;
	if (t != NAUI_JSON_TOKEN_OBJECT_BEGIN && t != NAUI_JSON_TOKEN_ARRAY_BEGIN)
	{
		naui_json_reader_next(reader);
		return;
	}

	int depth = 1;
	while (depth > 0 && reader->token != NAUI_JSON_TOKEN_ERROR && reader->token != NAUI_JSON_TOKEN_EOF)
	{
		naui_json_reader_next(reader);

		if (reader->token == NAUI_JSON_TOKEN_OBJECT_BEGIN || reader->token == NAUI_JSON_TOKEN_ARRAY_BEGIN)
			++depth;
		else if (reader->token == NAUI_JSON_TOKEN_OBJECT_END || reader->token == NAUI_JSON_TOKEN_ARRAY_END)
			--depth;
	}
}

int naui_json_reader_copy_str(const Naui_JsonReader* reader, char* dest, size_t dest_size)
{
	if (!dest || dest_size == 0)
		return -1;

	if (reader->token != NAUI_JSON_TOKEN_KEY && reader->token != NAUI_JSON_TOKEN_STRING)
	{
		dest[0] = '\0';
		return -1;
	}

	const char* src = reader->str;
	const char* end = reader->str + reader->len;
	size_t written = 0;

	while (src < end && written < dest_size - 1)
	{
		if (*src != '\\')
		{
			dest[written++] = *src++;
			continue;
		}

		++src;
		if (src >= end)
			break;

		switch (*src)
		{
			case '"': dest[written++] = '"';
				break;

			case '\\': dest[written++] = '\\';
				break;

			case '/': dest[written++] = '/';
				break;

			case 'b': dest[written++] = '\b';
				break;

			case 'f': dest[written++] = '\f';
				break;

			case 'n': dest[written++] = '\n';
				break;

			case 'r': dest[written++] = '\r';
				break;

			case 't': dest[written++] = '\t';
				break;

			case 'u':
			{
				if (src + 4 >= end)
				{
					src = end;
					continue;
				}

				unsigned int cp = 0;
				for (int i = 1; i <= 4; ++i)
				{
					char h = src[i];
					cp <<= 4;
					if (h >= '0' && h <= '9')
					{
						cp |= (unsigned)(h - '0');
					}
					else if (h >= 'a' && h <= 'f')
					{
						cp |= (unsigned)(h - 'a' + 10);
					}
					else if (h >= 'A' && h <= 'F')
					{
						cp |= (unsigned)(h - 'A' + 10);
					}
				}

				src += 4;
				if (cp < 0x80 && written < dest_size - 1)
				{
					dest[written++] = (char)cp;
				}
				else if (cp < 0x800 && written + 1 < dest_size - 1)
				{
					dest[written++] = (char)(0xC0 | (cp >> 6));
					dest[written++] = (char)(0x80 | (cp & 0x3F));
				}
				else if (written + 2 < dest_size - 1)
				{
					dest[written++] = (char)(0xE0 | (cp >> 12));
					dest[written++] = (char)(0x80 | ((cp >> 6) & 0x3F));
					dest[written++] = (char)(0x80 | (cp & 0x3F));
				}

				break;
			}
			default:
				dest[written++] = *src;
				break;
		}

		++src;
	}

	dest[written] = '\0';
	return (int)written;
}

Naui_JsonToken naui_json_reader_next(Naui_JsonReader* reader)
{
	Naui_JsonReader* r = reader;
	if (r->token == NAUI_JSON_TOKEN_ERROR || r->token == NAUI_JSON_TOKEN_EOF)
		return r->token;

	skip_whitespace(r);
	if (r->_cursor >= r->_end)
	{
		r->token = NAUI_JSON_TOKEN_EOF;
		return NAUI_JSON_TOKEN_EOF;
	}

	char c = peek(r);
	if (c == '}')
	{
		if (r->_depth == 0 || !r->_in_object[r->_depth - 1])
			return set_error(r, "unexpected '}'");

		advance(r);
		reader_pop_scope(r);
		r->_expect_key = r->_depth > 0 && r->_in_object[r->_depth - 1];
		r->token = NAUI_JSON_TOKEN_OBJECT_END;
		return NAUI_JSON_TOKEN_OBJECT_END;
	}

	if (c == ']')
	{
		if (r->_depth == 0 || r->_in_object[r->_depth - 1])
			return set_error(r, "unexpected ']'");

		advance(r);
		reader_pop_scope(r);
		r->_expect_key = r->_depth > 0 && r->_in_object[r->_depth - 1];
		r->token = NAUI_JSON_TOKEN_ARRAY_END;
		return NAUI_JSON_TOKEN_ARRAY_END;
	}

	if (r->_depth > 0 && r->_in_object[r->_depth - 1] && r->_expect_key)
	{
		if (!consume_comma_if_needed(r))
			return set_error(r, "expected ',' between object members");

		skip_whitespace(r);
		if (peek(r) != '"')
			return set_error(r, "expected string key");

		Naui_JsonToken t = parse_string(r, NAUI_JSON_TOKEN_KEY);
		if (t == NAUI_JSON_TOKEN_ERROR)
			return t;

		skip_whitespace(r);
		if (peek(r) != ':')
			return set_error(r, "expected ':' after key");

		advance(r);
		if (r->_depth > 0)
			r->_needs_comma[r->_depth - 1] = false;

		r->_expect_key = false;
		return NAUI_JSON_TOKEN_KEY;
	}

	if (!r->_expect_key)
	{
		if (!consume_comma_if_needed(r))
			return set_error(r, "expected ','");

		skip_whitespace(r);
		c = peek(r);
	}

	mark_needs_comma(r);
	if (r->_depth > 0 && r->_in_object[r->_depth - 1])
		r->_expect_key = true;

	switch (c)
	{
		case '{':
		{
			advance(r);
			reader_push_scope(r, true);
			r->_needs_comma[r->_depth - 1] = false;
			r->_expect_key = true;
			r->token = NAUI_JSON_TOKEN_OBJECT_BEGIN;
			return NAUI_JSON_TOKEN_OBJECT_BEGIN;
		}
		case '[':
		{
			advance(r);
			reader_push_scope(r, false);
			r->_needs_comma[r->_depth - 1] = false;
			r->_expect_key = false;
			r->token = NAUI_JSON_TOKEN_ARRAY_BEGIN;
			return NAUI_JSON_TOKEN_ARRAY_BEGIN;
		}
		case '"':
			return parse_string(r, NAUI_JSON_TOKEN_STRING);

		case 't':
			return parse_literal(r, "true", 4, NAUI_JSON_TOKEN_BOOL, true);

		case 'f':
			return parse_literal(r, "false", 5, NAUI_JSON_TOKEN_BOOL, false);

		case 'n':
			return parse_literal(r, "null", 4, NAUI_JSON_TOKEN_NULL, false);

		default:
			if (c == '-' || is_digit(c))
				return parse_number(r);

			return set_error(r, "unexpected character");
	}
}
#pragma endregion
