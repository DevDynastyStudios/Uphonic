#pragma once
#include <cstdint>
#include <string_view>

namespace Naui
{
	class Hash
	{
	public:
		static uint64_t FNV1a(std::string_view str);
		static uint64_t FNV1a(const void* data, size_t size);

		static uint64_t Combine(uint64_t h1, uint64_t h2);
	};
}