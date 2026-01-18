#include "Layout.h"
#include "Naui.h"
#include "Naui/FileSystem/File.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include <iostream>

namespace fs = std::filesystem;

static const char* SYSTEM_DIR = "Layouts/System";
static const char* USER_DIR   = "Layouts/User";

static std::vector<std::string> cachedSystemLayouts;
static std::vector<std::string> cachedUserLayouts;
static std::vector<std::string> cachedAllLayouts;
static bool layoutCacheDirty = true;

std::string Layout::SystemPath(const std::string& name)
{
	fs::path base = Naui::Directory::BinDirectory();
	fs::path rel  = fs::path(SYSTEM_DIR) / (name + ".json");
	return (base / rel).string();
}

std::string Layout::UserPath(const std::string& name)
{
	fs::path base = Naui::Directory::BinDirectory();
	fs::path rel  = fs::path(USER_DIR) / (name + ".json");
	return (base / rel).string();
}

bool Layout::ExistsSystem(const std::string& name)
{
	return fs::exists(SystemPath(name));
}

bool Layout::ExistsUser(const std::string& name)
{
	return fs::exists(UserPath(name));
}

bool Layout::Load(const std::string& name)
{
	std::string sys = SystemPath(name);
	std::string usr = UserPath(name);
	std::string path;

	if (fs::exists(sys)) 
		path = sys;
	else if (fs::exists(usr)) 
		path = usr;
	else 
		return false;

	nlohmann::json j;
	std::ifstream file(path);
	file >> j;

	for (auto& [id, panel] : Naui::GetAllPanels())
	{
		panel->SetOpen(j["panels"][panel->GetTitle()]["isOpen"].get<bool>());
	}

    std::string iniData = j["data"].get<std::string>();
    ImGui::LoadIniSettingsFromMemory(iniData.c_str());

	return true;
}

bool Layout::LoadDefault()
{
	return Load("Default");
}

bool Layout::Save(const std::string& name, bool overwrite)
{
    fs::path base = Naui::Directory::BinDirectory();
    fs::path dir  = base / USER_DIR;
    fs::create_directories(dir);
    fs::path path = dir / (name + ".json");

    if (!overwrite && fs::exists(path))
        return false;

    const char *data = ImGui::SaveIniSettingsToMemory();

	nlohmann::json j;
	for (auto &[id, panel] : Naui::GetAllPanels())
	{
		j["panels"][panel->GetTitle()]["isOpen"] = panel->IsOpen();
	}
	j["data"] = data;

	std::ofstream file(path);
	file << j.dump(4);

    return true;
}

bool Layout::Delete(const std::string& name)
{
	bool deleted = false;
	std::string user = UserPath(name);
	if (fs::exists(user))
	{
		std::error_code ec;
		fs::remove(user, ec);
		if (!ec)
			deleted = layoutCacheDirty = true;
	}

	return deleted;
}

const std::vector<std::string>& Layout::GetSystemLayouts()
{
	if (layoutCacheDirty)
		RefreshLayoutCache();

	return cachedSystemLayouts;
}

const std::vector<std::string>& Layout::GetUserLayouts()
{
	if (layoutCacheDirty)
		RefreshLayoutCache();

	return cachedUserLayouts;
}

const std::vector<std::string>& Layout::GetAllLayouts()
{
	if (layoutCacheDirty)
	{
		cachedAllLayouts = GetSystemLayouts();
		auto usr = GetUserLayouts();
		cachedAllLayouts.insert(cachedAllLayouts.end(), usr.begin(), usr.end());
	}

	return cachedAllLayouts;
}

std::vector<std::string> Layout::ListLayoutsIn(const char* folder)
{
	std::vector<std::string> result;
	fs::path base = Naui::Directory::BinDirectory();
	fs::path dir  = base / folder;

	if (!fs::exists(dir))
		return result;

	for (auto& entry : fs::directory_iterator(dir))
	{
		if (!entry.is_regular_file())
			continue;

		if (entry.path().extension() == ".json")
			result.push_back(entry.path().stem().string());
	}

	return result;
}

void Layout::RefreshLayoutCache()
{
	cachedSystemLayouts = ListLayoutsIn(SYSTEM_DIR);
	cachedUserLayouts   = ListLayoutsIn(USER_DIR);
	layoutCacheDirty = false;
}