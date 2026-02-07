#include "UUID.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace Naui
{
	static std::string GenerateHex128()
	{
		static std::random_device randDevice;
		static std::mt19937_64 engine(randDevice());
		static std::uniform_int_distribution<uint64_t> dist;

		uint64_t a = dist(engine);
		uint64_t b = dist(engine);

		std::stringstream ss;
		ss << std::hex << std::setfill('0') << std::setw(16) << a << std::setw(16) << b;
		return ss.str();
	}

	UUID::UUID() : m_hex(GenerateHex128()) {}
	UUID::UUID(const std::string& hex) : m_hex(hex) {}
}