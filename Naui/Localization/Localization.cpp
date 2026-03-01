#include "Localization.h"
#include "FileSystem/File.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

#define RESERVER_PADDING 32

#define JSON_META "_meta"
#define JSON_LANGUAGE "language"
#define JSON_REGION "region"
#define JSON_DIRECTION "direction"

namespace Naui
{

std::unordered_map<std::string, Localization::Entry> Localization::table;
TextDirection Localization::direction = TextDirection::LTR;
std::string Localization::languageCode = "en";
std::string Localization::regionCode = "US";
std::string Localization::currentLanguage;

static TextDirection ParseDirection(const std::string& str)
{
	return (str == "rtl" || str == "RTL") ? TextDirection::RTL : TextDirection::LTR;
}

static std::string FormatInterpolated(const std::string& pattern, std::span<const std::string> args)
{
	std::string result;
	result.reserve(pattern.size() + RESERVER_PADDING);
    const char* s = pattern.c_str();
    size_t len = pattern.size();

    for (size_t i = 0; i < len; ++i)
    {
        char c = s[i];
        if (c == '\\')
        {
            if (i + 1 < len)
            {
                char n = s[i + 1];
				if(n == '{' || n == '}' || n == '\\')
				{
					result.push_back(n);
					i++;
					continue;
				}
			}

            result.push_back('\\');
            continue;
        }

        if (c == '{')
        {
            size_t j = i + 1;
            size_t index = 0;
            bool hasDigit = false;

            while (j < len && std::isdigit(s[j]))
            {
                hasDigit = true;
                index = index * 10 + (s[j] - '0');
                j++;
            }

            if (hasDigit && j < len && s[j] == '}')
            {
                if (index < args.size())
                    result += args[index];

                i = j;
                continue;
            }

            result.push_back('{');
            continue;
        }

        result.push_back(c);
    }

    return result;
}

bool Localization::Load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    nlohmann::json json;
    try
    {
        file >> json;
    }
    catch (const std::exception& e)
    {
        return false;
    }

    table.clear();
    languageCode = "en";
    regionCode = "US";
    direction = TextDirection::LTR;

    if (json.contains(JSON_META) && json[JSON_META].is_object())
    {
        nlohmann::json& meta = json[JSON_META];
        if (meta.contains(JSON_LANGUAGE) && meta[JSON_LANGUAGE].is_string())
            languageCode = meta[JSON_LANGUAGE].get<std::string>();

        if (meta.contains(JSON_REGION) && meta[JSON_REGION].is_string())
            regionCode = meta[JSON_REGION].get<std::string>();

        if (meta.contains(JSON_DIRECTION) && meta[JSON_DIRECTION].is_string())
            direction = ParseDirection(meta[JSON_DIRECTION].get<std::string>());
        else
            direction = TextDirection::LTR;
    }

    for (auto it = json.begin(); it != json.end(); ++it)
    {
        const std::string& key = it.key();
        if (key == JSON_META)
            continue;

        if (!it.value().is_string())
            continue;

        std::string value = it.value().get<std::string>();
        Entry entry;
        if (!value.empty() && value[0] == '$')
        {
            entry.isInterpolated = true;
            value.erase(0, 1);
        }

        entry.text = std::move(value);
        table[key] = std::move(entry);
    }

    return true;
}

bool Localization::SetLanguage(const std::string& code)
{
    std::filesystem::path path = Naui::Directory::BinDirectory() / ("Language/" + code + ".lang");
    if (Localization::Load(path.string()))
    {
        currentLanguage = code;
        return true;
    }

	if(!currentLanguage.empty())
		return false;

    std::filesystem::path fallbackPath = Naui::Directory::BinDirectory() / "Language/en-US.lang";
    if (Localization::Load(fallbackPath.string()))
        currentLanguage = "en-US";

	return false;
}

const char* Localization::Get(const char* key)
{
	auto it = table.find(key);
	if(it != table.end())
		return it->second.text.c_str();

	return key;
}

std::string Localization::Format(const char* key, std::span<const std::string> args)
{
	auto it = table.find(key);
	if(it == table.end())
		return key;

	const Entry& entry = it->second;
	if(!entry.isInterpolated)
		return entry.text;

	return FormatInterpolated(entry.text, args);
}

TextDirection Localization::Direction()
{
	return direction;
}

const std::string& Localization::LanguageCode()
{
	return languageCode;
}

const std::string& Localization::RegionCode()
{
	return regionCode;
}
}