bool uph_file_signature(const Naui_Path path, Uph_FileSignature *out_signature)
{
	Naui_FileHandle handle = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&handle, path, NAUI_FILE_READ))
		return false;

	uint8_t buffer[64 * 1024];
	uint64_t hash = NAUI_FNV_OFFSET;
	uint64_t size = 0;

	size_t bytes_read;
	while ((bytes_read = naui_file_read(&handle, buffer, sizeof(buffer))) > 0)
	{
		for (size_t i = 0; i < bytes_read; i++)
		{
			hash ^= buffer[i];
			hash *= NAUI_FNV_PRIME;
		}
		size += bytes_read;
	}

	naui_file_close(&handle);
	out_signature->size = size;
	out_signature->hash = hash;
	return true;
}

bool uph_files_identical(const Naui_Path a, const Naui_Path b)
{
	Naui_FileHandle handle_a = NAUI_FILE_HANDLE_INIT;
	Naui_FileHandle handle_b = NAUI_FILE_HANDLE_INIT;

	if (!naui_file_open(&handle_a, a, NAUI_FILE_READ))
		return false;

	if (!naui_file_open(&handle_b, b, NAUI_FILE_READ))
	{
		naui_file_close(&handle_a);
		return false;
	}

	uint8_t buffer_a[16 * 1024];
	uint8_t buffer_b[16 * 1024];
	bool identical = true;

	for (;;)
	{
		const size_t read_a = naui_file_read(&handle_a, buffer_a, sizeof(buffer_a));
		const size_t read_b = naui_file_read(&handle_b, buffer_b, sizeof(buffer_b));

		if (read_a != read_b || memcmp(buffer_a, buffer_b, read_a) != 0)
		{
			identical = false;
			break;
		}

		if (read_a == 0)
			break;
	}

	naui_file_close(&handle_a);
	naui_file_close(&handle_b);
	return identical;
}