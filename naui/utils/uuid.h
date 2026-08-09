typedef uint64_t Naui_UUID;

#define NAUI_UUID_NULL ((Naui_UUID)0)
#define NAUI_UUID_STR_SIZE 17

Naui_UUID naui_uuid_generate(void);

/* Writes hex characters + null into buf (must be >= NAUI_UUID_STR_SIZE). */
static inline void naui_uuid_format(Naui_UUID id, char* buf)
{
	snprintf(buf, NAUI_UUID_STR_SIZE, "%016llx", (unsigned long long)id);
}

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4996)
#endif
/* Parse a hex string produced by naui_uuid_format.
 * Returns NAUI_UUID_NULL on fail. */
static inline Naui_UUID naui_uuid_parse(const char* str)
{
	if (!str)
		return NAUI_UUID_NULL;

	unsigned long long v = 0;
	if (sscanf(str, "%016llx", &v) != 1)
		return NAUI_UUID_NULL;

	return (Naui_UUID)v;
}

#ifdef _MSC_VER
#	pragma warning(pop)
#endif
