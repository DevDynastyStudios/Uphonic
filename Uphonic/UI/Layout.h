#pragma once
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <string>

class Layout
{
public:
	static bool Load(const std::string& name);
	static bool LoadDefault();
	static bool Save(const std::string& name, bool overwrite = false);
	static bool Delete(const std::string& name);

	static const std::vector<std::filesystem::path>& GetSystemLayouts();
	static const std::vector<std::filesystem::path>& GetUserLayouts();
	static const std::vector<std::filesystem::path>& GetAllLayouts();

	static bool Exists(const std::string& name);
	
	static std::filesystem::path SystemPath(const std::string& name);
	static std::filesystem::path UserPath(const std::string& name);

private:
	static void RefreshLayoutCache();
	static std::vector<std::filesystem::path> ListLayoutsIn(const std::filesystem::path& folder);
	static std::unordered_map<std::filesystem::path, bool> ParseVisibility(const std::filesystem::path& ini);


};