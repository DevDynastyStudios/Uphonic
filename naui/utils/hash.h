#define NAUI_FNV_OFFSET 14695981039346656037ull
#define NAUI_FNV_PRIME 1099511628211ull

static inline uint64_t naui_hash_str(const char* str)
{
	uint64_t hash = NAUI_FNV_OFFSET;
	while (*str)
	{
		hash ^= (uint8_t)*str++;
		hash *= NAUI_FNV_PRIME;
	}

	return hash;
}

static inline uint64_t naui_hash_bytes(const void* data, size_t size)
{
	uint64_t hash = NAUI_FNV_OFFSET;
	const unsigned char* bytes = (const unsigned char*)data;
	for (size_t i = 0; i < size; ++i)
	{
		hash ^= bytes[i];
		hash *= NAUI_FNV_PRIME;
	}

	return hash;
}

static inline uint64_t naui_hash_combine(uint64_t h1, uint64_t h2)
{
	return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
}
