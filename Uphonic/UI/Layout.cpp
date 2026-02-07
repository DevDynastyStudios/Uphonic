#include "Layout.h"
#include "Naui.h"
#include "Naui/FileSystem/File.h"
#include <string>
#include <fstream>
#include <sstream>
#include <imgui.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

static const std::filesystem::path SYSTEM_DIR = std::filesystem::weakly_canonical(Naui::Directory::BinDirectory() / "Layouts");
static const std::filesystem::path USER_DIR = std::filesystem::weakly_canonical(Naui::Directory::AppDataDirectory() / "Uphonic/Layouts");

static std::vector<std::filesystem::path> cachedSystemLayouts;
static std::vector<std::filesystem::path> cachedUserLayouts;
static std::vector<std::filesystem::path> cachedAllLayouts;
static bool layoutCacheDirty = true;

std::filesystem::path Layout::SystemPath(const std::string& name)
{
	return SYSTEM_DIR / (name + ".json");
}

std::filesystem::path Layout::UserPath(const std::string& name)
{
	return USER_DIR / (name + ".json");
}

bool Layout::Exists(const std::string& name)
{
	return fs::exists(UserPath(name));
}

bool Layout::Load(const std::string& name)
{
	std::filesystem::path sys = SystemPath(name);
	std::filesystem::path usr = UserPath(name);
	std::filesystem::path path;

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
		if (panel->GetWindowFlags() & ImGuiWindowFlags_NoSavedSettings)
			continue;

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
	fs::create_directories(USER_DIR);
	fs::path path = USER_DIR / (name + ".json");

	if (!overwrite && fs::exists(path))
		return false;

	const char *data = ImGui::SaveIniSettingsToMemory();

	nlohmann::json j;
	for (auto &[id, panel] : Naui::GetAllPanels())
	{
		if (panel->GetWindowFlags() & ImGuiTableFlags_NoSavedSettings)
			continue;

		j["panels"][panel->GetTitle()]["isOpen"] = panel->IsOpen();
	}
	j["data"] = data;

	std::ofstream file(path);
	file << j.dump(4);

	return layoutCacheDirty = true;
}

bool Layout::Delete(const std::string& name)
{
	bool deleted = false;
	std::filesystem::path user = UserPath(name);
	if (fs::exists(user))
	{
		std::error_code ec;
		fs::remove(user, ec);
		if (!ec)
			deleted = layoutCacheDirty = true;
	}

	return deleted;
}

const std::vector<std::filesystem::path>& Layout::GetSystemLayouts()
{
	if (layoutCacheDirty)
		RefreshLayoutCache();

	return cachedSystemLayouts;
}

const std::vector<std::filesystem::path>& Layout::GetUserLayouts()
{
	if (layoutCacheDirty)
		RefreshLayoutCache();

	return cachedUserLayouts;
}

const std::vector<std::filesystem::path>& Layout::GetAllLayouts()
{
	if (layoutCacheDirty)
	{
		cachedAllLayouts = GetSystemLayouts();
		auto usr = GetUserLayouts();
		cachedAllLayouts.insert(cachedAllLayouts.end(), usr.begin(), usr.end());
	}

	return cachedAllLayouts;
}

std::vector<std::filesystem::path> Layout::ListLayoutsIn(const std::filesystem::path& folderPath)
{
	std::vector<std::filesystem::path> result;

	if (!fs::exists(folderPath))
		return result;

	for (auto& entry : fs::directory_iterator(folderPath))
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