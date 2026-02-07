#pragma once
#include "Base.h"
#include <string>

namespace Naui
{
	class UUID
	{
	public:
		NAUI_API UUID();
		NAUI_API explicit UUID(const std::string& hex);

		const std::string& Str() const {return m_hex; }
		bool operator ==(const UUID& other) const { return m_hex == other.m_hex; }
		bool operator !=(const UUID& other) const { return !(*this == other); }

	private:
		std::string m_hex;
	};
}