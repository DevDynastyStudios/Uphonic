#pragma once

#include <string>
#include <vector>
#include <filesystem>

class Layout
{
public:
	static bool Load(const std::string& name);
	static bool LoadDefault();
	static bool Save(const std::string& name, bool overwrite = true);
	static bool Delete(const std::string& name);
	static bool Exists(const std::string& name);

	static std::filesystem::path SystemPath(const std::string& name);
	static std::filesystem::path UserPath(const std::string& name);

	static const std::vector<std::filesystem::path>& GetSystemLayouts();
	static const std::vector<std::filesystem::path>& GetUserLayouts();
	static const std::vector<std::filesystem::path>& GetAllLayouts();

	static void RefreshLayoutCache();
};