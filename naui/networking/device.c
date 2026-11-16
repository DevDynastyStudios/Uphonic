Naui_DeviceId naui_device_id_generate(void)
{
	Naui_DeviceId id;
	do
	{
		for (size_t i = 0; i < NAUI_DEVICE_ID_SIZE; i += sizeof(uint64_t))
		{
			uint64_t part = naui_uuid_generate();
			memcpy(id.bytes + i, &part, sizeof(part));
		}
	} while (naui_device_id_is_null(id));

	return id;
}

bool naui_device_id_is_null(Naui_DeviceId id)
{
	uint8_t any = 0;
	for (size_t i = 0; i < NAUI_DEVICE_ID_SIZE; ++i)
	{
		any |= id.bytes[i];
	}

	return any == 0;
}

bool naui_device_id_equal(Naui_DeviceId a, Naui_DeviceId b)
{
	return memcmp(a.bytes, b.bytes, NAUI_DEVICE_ID_SIZE) == 0;
}

uint64_t naui_device_id_hash(Naui_DeviceId id)
{
	return naui_hash_bytes(id.bytes, NAUI_DEVICE_ID_SIZE);
}

void naui_device_id_format(Naui_DeviceId id, char* buf)
{
	static const char digits[] = "0123456789abcdef";
	for (size_t i = 0; i < NAUI_DEVICE_ID_SIZE; ++i)
	{
		buf[i * 2 + 0] = digits[id.bytes[i] >> 4];
		buf[i * 2 + 1] = digits[id.bytes[i] & 0x0f];
	}

	buf[NAUI_DEVICE_ID_SIZE * 2] = 0;
}

static int device_hex_value(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

bool naui_device_id_parse(Naui_DeviceId* out, const char* text)
{
	if (!out || !text || strlen(text) != NAUI_DEVICE_ID_SIZE * 2)
		return false;

	Naui_DeviceId id;
	for (size_t i = 0; i < NAUI_DEVICE_ID_SIZE; ++i)
	{
		int hi = device_hex_value(text[i * 2 + 0]);
		int lo = device_hex_value(text[i * 2 + 1]);
		if (hi < 0 || lo < 0)
			return false;

		id.bytes[i] = (uint8_t)((hi << 4) | lo);
	}

	*out = id;
	return true;
}

void naui_device_describe_local(Naui_Device* device, const char* name_override)
{
#if defined(NAUI_WINDOWS)
	device->platform = NAUI_PLATFORM_WINDOWS;
#elif defined(NAUI_LINUX)
	device->platform = NAUI_PLATFORM_LINUX;
#elif defined(NAUI_MACOS)
	device->platform = NAUI_PLATFORM_MACOS;
#else
	device->platform = NAUI_PLATFORM_UNKNOWN;
#endif

#if defined(__x86_64__) || defined(_M_X64)
	device->arch = NAUI_ARCH_X86_64;
#elif defined(__i386__) || defined(_M_IX86)
	device->arch = NAUI_ARCH_X86;
#elif defined(__aarch64__) || defined(_M_ARM64)
	device->arch = NAUI_ARCH_ARM64;
#elif defined(__arm__) || defined(_M_ARM)
	device->arch = NAUI_ARCH_ARM;
#else
	device->arch = NAUI_ARCH_UNKNOWN;
#endif

	char hostname[NAUI_DEVICE_NAME_MAX] = { 0 };
	const char* name = name_override;
	if (!name)
	{
#if defined(_WIN32) || defined(_WIN64)
		DWORD size = (DWORD)sizeof(hostname);
		if (!GetComputerNameA(hostname, &size))
			hostname[0] = 0;
#else
		if (gethostname(hostname, sizeof(hostname) - 1) != 0)
			hostname[0] = 0;
#endif
		name = hostname[0] ? hostname : "device";
	}

	size_t len = strlen(name);
	if (len >= NAUI_DEVICE_NAME_MAX)
		len = NAUI_DEVICE_NAME_MAX - 1;

	memcpy(device->name, name, len);
	device->name[len] = 0;
}
