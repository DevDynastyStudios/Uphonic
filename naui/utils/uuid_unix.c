#if !defined(_WIN32) || !defined(_WIN64)

#if defined(__linux__)
#	include <sys/random.h>

static uint64_t platform_random64(void)
{
	uint64_t v = 0;

	if (getrandom(&v, sizeof(v), 0) == sizeof(v))
		return v;

	FILE* f = fopen("/dev/urandom", "rb");
	if (f)
	{
		fread(&v, sizeof(v), 1, f);
		fclose(f);
	}

	return v;
}

#else
/* macOS / BSD */
#	include <stdlib.h>

static uint64_t platform_random64(void)
{
	uint64_t v = 0;
	arc4random_buf(&v, sizeof(v));
	return v;
}

#endif

Naui_UUID naui_uuid_generate(void)
{
	Naui_UUID id;
	do
	{
		id = platform_random64();
	} while (id == NAUI_UUID_NULL);

	return id;
}
#endif
