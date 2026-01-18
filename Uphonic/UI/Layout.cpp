#include "Layout.h"
#include "Naui.h"
#include "Naui/FileSystem/File.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <imgui.h>

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
	fs::path rel  = fs::path(SYSTEM_DIR) / (name + ".ini");
	return (base / rel).string();
}

std::string Layout::UserPath(const std::string& name)
{
	fs::path base = Naui::Directory::BinDirectory();
	fs::path rel  = fs::path(USER_DIR) / (name + ".ini");
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

	ImGui::LoadIniSettingsFromDisk(path.c_str());
	std::ifstream in(path);
	if (!in)
		return true;

	std::string ini((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	for (auto& [id, panelPtr] : Naui::GetAllPanels())
	{
		const std::string& title = panelPtr->GetTitle();
		std::string tag = "[Window][" + title + "]";

		size_t pos = ini.find(tag);
		if (pos == std::string::npos)
		{
			panelPtr->SetOpen(false);
			continue;
		}

		size_t nextSection = ini.find("[Window][", pos + tag.length());
		if (nextSection == std::string::npos)
			nextSection = ini.length();

		size_t collapsedPos = ini.find("Collapsed=", pos);
		if (collapsedPos == std::string::npos || collapsedPos >= nextSection)
		{
			panelPtr->SetOpen(true);
			continue;
		}

		char value = ini[collapsedPos + strlen("Collapsed=")];
		panelPtr->SetOpen(value == '0');
	}

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
    fs::path path = dir / (name + ".ini");

    if (!overwrite && fs::exists(path))
        return false;

    // 1. Let ImGui write the ini normally
    ImGui::SaveIniSettingsToDisk(path.string().c_str());

    // 2. Load the ini text
    std::ifstream in(path);
    if (!in)
        return false;

    std::string ini((std::istreambuf_iterator<char>(in)),
                    std::istreambuf_iterator<char>());
    in.close();

    // 3. Build a map: window title -> panel*
    std::unordered_map<std::string, Naui::Panel*> panelMap;
    panelMap.reserve(Naui::GetAllPanels().size());
    for (auto& [id, panelPtr] : Naui::GetAllPanels())
        panelMap.emplace(panelPtr->GetTitle(), panelPtr);

    // 4. Rewrite ini with panel-driven Collapsed flags
    std::ostringstream outIni;
    std::istringstream iss(ini);
    std::string line;
    std::string currentName;
    Naui::Panel* currentPanel = nullptr;
    bool inWindow     = false;
    bool sawCollapsed = false;

    auto flush_missing_collapsed = [&]()
    {
        if (inWindow && currentPanel && !sawCollapsed)
        {
            bool open = currentPanel->IsOpen();
            outIni << "Collapsed=" << (open ? "0" : "1") << "\n";
        }
    };

    while (std::getline(iss, line))
    {
        // Start of a new window block
        if (line.rfind("[Window][", 0) == 0)
        {
            // Finish previous window block if needed
            flush_missing_collapsed();

            inWindow     = true;
            sawCollapsed = false;

            size_t close = line.find(']', 8);
            currentName  = line.substr(8, close - 8);

            auto it = panelMap.find(currentName);
            currentPanel = (it != panelMap.end()) ? it->second : nullptr;

            outIni << line << "\n";
            continue;
        }

        // Inside a window block, Collapsed= line
        if (inWindow && line.rfind("Collapsed=", 0) == 0)
        {
            sawCollapsed = true;

            if (currentPanel)
            {
                bool open = currentPanel->IsOpen();
                outIni << "Collapsed=" << (open ? "0" : "1") << "\n";
            }
            else
            {
                // Not one of our panels → leave as ImGui wrote it
                outIni << line << "\n";
            }

            continue;
        }

        // Any other line
        outIni << line << "\n";
    }

    // End of file: flush last window block if needed
    flush_missing_collapsed();

    // 5. Write modified ini back to disk
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;

    std::string finalIni = outIni.str();
    out.write(finalIni.data(), finalIni.size());

    layoutCacheDirty = true;
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

		if (entry.path().extension() == ".ini")
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