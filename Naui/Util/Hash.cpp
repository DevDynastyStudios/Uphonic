#include "Hash.h"

namespace Naui
{
	static constexpr uint64_t FNV_OFFSET = 1469598103934665603ull;
	static constexpr uint64_t FNV_PRIME = 1099511628211ull;

	uint64_t Hash::FNV1a(std::string_view str)
	{
		uint64_t hash = FNV_OFFSET;
		for(unsigned char c : str)
		{
			hash ^= c;
			hash *= FNV_PRIME;
		}

		return hash;
	}

	uint64_t Hash::FNV1a(const void* data, size_t size)
	{
		uint64_t hash = FNV_OFFSET;
		const unsigned char* bytes = static_cast<const unsigned char*>(data);

		for(size_t i = 0; i < size; ++i)
		{
			hash ^= bytes[i];
			hash *= FNV_PRIME;
		}

		return hash;
	}

	uint64_t Hash::Combine(uint64_t h1, uint64_t h2)
	{
		return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
	}
}