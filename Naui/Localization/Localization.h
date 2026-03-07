#pragma warning(push)
#pragma warning(disable : 4251)
#pragma once
#include "Base.h"
#include <string>
#include <unordered_map>
#include <span>
#include <vector>

namespace Naui
{
enum class TextDirection
{
	LTR, RTL
};

struct LanguageEntry
{
	std::string code;
	std::string displayName;
};

class NAUI_API Localization
{
public:
	static bool Load(const std::string& path);
	static bool SetLanguage(const std::string& code);
	static const char* Get(const char* key);
	static std::string Format(const char* key, std::span<const std::string> args);

	static TextDirection Direction();
	static const std::string& LanguageCode();
	static const std::string& RegionCode();
	static const std::string& CurrentLanguage();

	// Returns all available language entries found in the Language directory.
	// Cached on every SetLanguage call.
	static const std::vector<LanguageEntry>& GetLanguages();

private:
	static void ScanLanguages();

	struct Entry
	{
		std::string text;
		bool isInterpolated = false;
	};

	static std::unordered_map<std::string, Entry> table;
	static TextDirection direction;
	static std::string languageCode;
	static std::string regionCode;
	static std::string currentLanguage;
	static std::vector<LanguageEntry> languages;
};

inline const char* TR(const char* key)
{
	return Localization::Get(key);
}

inline std::string TR(const char* key, std::vector<std::string> args)
{
	return Localization::Format(key, args);
}

}
#pragma warning(pop)