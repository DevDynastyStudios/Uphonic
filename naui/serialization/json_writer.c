#pragma region Static Functions
static void writer_putc(Naui_JsonWriter* writer, char c)
{
	if (writer->has_error)
		return;

	if (writer->_heap)
	{
		if (writer->written + 1 >= writer->buf_size)
		{
			size_t new_cap = writer->buf_size ? writer->buf_size * 2 : 256;
			char* tmp = (char*)realloc(writer->buf, new_cap);
			if (!tmp)
			{
				writer->has_error = true;
				return;
			}

			writer->buf = tmp;
			writer->buf_size = new_cap;
		}
	}
	else if (writer->written + 1 >= writer->buf_size)
	{
		writer->has_error = true;
		return;
	}

	writer->buf[writer->written++] = c;
}

static void writer_puts(Naui_JsonWriter* writer, const char* str, size_t len)
{
	for (size_t i = 0; i < len; ++i)
	{
		writer_putc(writer, str[i]);
	}
}

static void writer_cstr(Naui_JsonWriter* writer, const char* str)
{
	while (*str)
	{
		writer_putc(writer, *str++);
	}
}

static void writer_indent(Naui_JsonWriter* writer)
{
	if (!writer->is_pretty)
		return;

	writer_putc(writer, '\n');
	for (uint8_t i = 0; i < writer->_depth; ++i)
	{
		writer_puts(writer, "    ", 4);
	}
}

static void writer_comma_and_indent(Naui_JsonWriter* writer)
{
	if (writer->_depth == 0)
		return;

	uint8_t d = writer->_depth - 1;
	if (writer->_needs_comma[d])
		writer_putc(writer, ',');

	writer->_needs_comma[d] = true;

	if (writer->is_pretty)
		writer_indent(writer);
}

static void writer_string_escaped(Naui_JsonWriter* writer, const char* str, size_t len)
{
	writer_putc(writer, '"');
	for (size_t i = 0; i < len; ++i)
	{
		unsigned char c = (unsigned char)str[i];
		switch (c)
		{
			case '"':
				writer_puts(writer, "\\\"", 2);
				break;

			case '\\':
				writer_puts(writer, "\\\\", 2);
				break;

			case '\b':
				writer_puts(writer, "\\b", 2);
				break;

			case '\f':
				writer_puts(writer, "\\f", 2);
				break;

			case '\n':
				writer_puts(writer, "\\n", 2);
				break;

			case '\r':
				writer_puts(writer, "\\r", 2);
				break;

			case '\t':
				writer_puts(writer, "\\t", 2);
				break;

			default:
				if (c < 0x20)
				{
					char esc[7];
					snprintf(esc, sizeof(esc), "\\u%04x", c);
					writer_puts(writer, esc, 6);
				}
				else
					writer_putc(writer, (char)c);

				break;
		}
	}

	writer_putc(writer, '"');
}

static void writer_push_scope(Naui_JsonWriter* writer, bool is_object)
{
	if (writer->_depth < NAUI_JSON_WRITER_MAX_DEPTH)
	{
		writer->_needs_comma[writer->_depth] = false;
		writer->_in_object[writer->_depth] = is_object;
		++writer->_depth;
	}
	else
		writer->has_error = true;
}

static void writer_pop_scope(Naui_JsonWriter* writer)
{
	if (writer->_depth > 0)
		--writer->_depth;
}
#pragma endregion

#pragma region Json Writer
void naui_json_writer_init(Naui_JsonWriter* writer, char* buf, size_t buf_size, bool is_pretty)
{
	memset(writer, 0, sizeof(*writer));
	writer->buf = buf;
	writer->buf_size = buf_size;
	writer->is_pretty = is_pretty;
	writer->_heap = false;
}

void naui_json_writer_init_heap(Naui_JsonWriter* writer, bool is_pretty)
{
	memset(writer, 0, sizeof(*writer));
	writer->is_pretty = is_pretty;
	writer->_heap = true;
}

void naui_json_writer_object_begin(Naui_JsonWriter* writer)
{
	writer_comma_and_indent(writer);
	writer_putc(writer, '{');
	writer_push_scope(writer, true);
}

void naui_json_writer_object_end(Naui_JsonWriter* writer)
{
	bool had_content = writer->_depth > 0 && writer->_needs_comma[writer->_depth - 1];
	writer_pop_scope(writer);
	if (had_content && writer->is_pretty)
		writer_indent(writer);

	writer_putc(writer, '}');
}

void naui_json_writer_array_begin(Naui_JsonWriter* writer)
{
	writer_comma_and_indent(writer);
	writer_putc(writer, '[');
	writer_push_scope(writer, false);
}

void naui_json_writer_array_end(Naui_JsonWriter* writer)
{
	bool had_content = writer->_depth > 0 && writer->_needs_comma[writer->_depth - 1];
	writer_pop_scope(writer);
	if (had_content && writer->is_pretty)
		writer_indent(writer);

	writer_putc(writer, ']');
}

void naui_json_writer_key(Naui_JsonWriter* writer, const char* key)
{
	naui_json_writer_key_len(writer, key, strlen(key));
}

void naui_json_writer_key_len(Naui_JsonWriter* writer, const char* key, size_t len)
{
	writer_comma_and_indent(writer);
	if (writer->_depth > 0)
		writer->_needs_comma[writer->_depth - 1] = false;

	writer_string_escaped(writer, key, len);
	writer_putc(writer, ':');
	if (writer->is_pretty)
		writer_putc(writer, ' ');
}

void naui_json_writer_string(Naui_JsonWriter* writer, const char* value)
{
	naui_json_writer_string_len(writer, value, strlen(value));
}

void naui_json_writer_string_len(Naui_JsonWriter* writer, const char* value, size_t len)
{
	if (writer->_depth == 0 || writer->_in_object[writer->_depth - 1])
	{
		if (writer->_depth > 0)
			writer->_needs_comma[writer->_depth - 1] = true;
	}
	else
		writer_comma_and_indent(writer);

	writer_string_escaped(writer, value, len);
}

void naui_json_writer_number(Naui_JsonWriter* writer, double value)
{
	if (writer->_depth > 0 && !writer->_in_object[writer->_depth - 1])
		writer_comma_and_indent(writer);
	else if (writer->_depth > 0)
		writer->_needs_comma[writer->_depth - 1] = true;

	char buf[64];
	if (value == (double)(long long)value && !isinf(value) && !isnan(value))
		snprintf(buf, sizeof(buf), "%.0f", value);
	else
		snprintf(buf, sizeof(buf), "%.15g", value);

	writer_cstr(writer, buf);
}

void naui_json_writer_int(Naui_JsonWriter* writer, int value)
{
	if (writer->_depth > 0 && !writer->_in_object[writer->_depth - 1])
		writer_comma_and_indent(writer);
	else if (writer->_depth > 0)
		writer->_needs_comma[writer->_depth - 1] = true;

	char buf[32];
	snprintf(buf, sizeof(buf), "%d", value);
	writer_cstr(writer, buf);
}

void naui_json_writer_uint(Naui_JsonWriter* writer, unsigned int value)
{
	if (writer->_depth > 0 && !writer->_in_object[writer->_depth - 1])
		writer_comma_and_indent(writer);
	else if (writer->_depth > 0)
		writer->_needs_comma[writer->_depth - 1] = true;

	char buf[32];
	snprintf(buf, sizeof(buf), "%u", value);
	writer_cstr(writer, buf);
}

void naui_json_writer_int64(Naui_JsonWriter* writer, int64_t value)
{
	if (writer->_depth > 0 && !writer->_in_object[writer->_depth - 1])
		writer_comma_and_indent(writer);
	else if (writer->_depth > 0)
		writer->_needs_comma[writer->_depth - 1] = true;

	char buf[32];
	snprintf(buf, sizeof(buf), "%lld", (long long)value);
	writer_cstr(writer, buf);
}

void naui_json_writer_uint64(Naui_JsonWriter* writer, uint64_t value)
{
	if (writer->_depth > 0 && !writer->_in_object[writer->_depth - 1])
		writer_comma_and_indent(writer);
	else if (writer->_depth > 0)
		writer->_needs_comma[writer->_depth - 1] = true;

	char buf[32];
	snprintf(buf, sizeof(buf), "%llu", (unsigned long long)value);
	writer_cstr(writer, buf);
}

void naui_json_writer_bool(Naui_JsonWriter* writer, bool value)
{
	if (writer->_depth > 0 && !writer->_in_object[writer->_depth - 1])
		writer_comma_and_indent(writer);
	else if (writer->_depth > 0)
		writer->_needs_comma[writer->_depth - 1] = true;

	writer_cstr(writer, value ? "true" : "false");
}

void naui_json_writer_null(Naui_JsonWriter* writer)
{
	if (writer->_depth > 0 && !writer->_in_object[writer->_depth - 1])
		writer_comma_and_indent(writer);
	else if (writer->_depth > 0)
		writer->_needs_comma[writer->_depth - 1] = true;

	writer_cstr(writer, "null");
}

int naui_json_writer_finish(Naui_JsonWriter* writer)
{
	if (writer->has_error)
		return -1;

	if (writer->buf && writer->buf_size > 0)
	{
		size_t null_pos = writer->written < writer->buf_size ? writer->written : writer->buf_size - 1;
		writer->buf[null_pos] = '\0';
	}

	if (writer->is_pretty && writer->written > 0)
		writer_putc(writer, '\n');

	return (int)writer->written;
}

char* naui_json_writer_finish_heap(Naui_JsonWriter* writer, size_t* out_len)
{
	if (writer->has_error || !writer->_heap)
	{
		if (out_len)
			*out_len = 0;
			
		return NULL;
	}

	writer_putc(writer, '\0');
	if (writer->has_error)
	{
		free(writer->buf);
		if (out_len) *out_len = 0;
		return NULL;
	}

	if (out_len)
		*out_len = writer->written - 1;

	char* result = writer->buf;
	writer->buf = NULL;
	return result;
}
#pragma endregion
