#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#pragma comment(lib, "advapi32.lib")

typedef BOOLEAN (WINAPI* RtlGenRandom_t)(PVOID, ULONG);

static uint64_t platform_random64(void)
{
	static RtlGenRandom_t fn = NULL;
	if (!fn)
	{
		HMODULE mod = LoadLibraryA("advapi32.dll");
		if (mod)
			fn = (RtlGenRandom_t)GetProcAddress(mod, "SystemFunction036");
	}

	uint64_t v = 0;
	if (fn)
	{
		fn(&v, sizeof(v));
	}
	else
	{
		static uint64_t counter = 0;
		v = ((uint64_t)GetTickCount64() << 32) ^ ++counter;
	}

	return v;
}

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
