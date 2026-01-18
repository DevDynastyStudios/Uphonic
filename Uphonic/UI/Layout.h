#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class Layout
{
public:
	static bool Load(const std::string& name);
	static bool LoadDefault();
	static bool Save(const std::string& name, bool overwrite = false);
	static bool Delete(const std::string& name);

	static const std::vector<std::string>& GetSystemLayouts();
	static const std::vector<std::string>& GetUserLayouts();
	static const std::vector<std::string>& GetAllLayouts();

	static bool ExistsSystem(const std::string& name);
	static bool ExistsUser(const std::string& name);
	
	static std::string SystemPath(const std::string& name);
	static std::string UserPath(const std::string& name);

private:
	static void RefreshLayoutCache();
	static std::vector<std::string> ListLayoutsIn(const char* folder);
	static std::unordered_map<std::string, bool> ParseVisibility(const std::string& ini);


};