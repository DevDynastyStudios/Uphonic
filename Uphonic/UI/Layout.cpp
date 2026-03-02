#include "Layout.h"
#include "Naui.h"
#include "Naui/FileSystem/File.h"
#include <string>
#include <fstream>
#include <sstream>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <iostream>

namespace fs = std::filesystem;

static const fs::path SYSTEM_DIR = fs::weakly_canonical(Naui::Directory::BinDirectory() / "Layouts");
static const fs::path USER_DIR   = fs::weakly_canonical(Naui::Directory::AppDataDirectory() / "Uphonic/Layouts");

static std::vector<fs::path> cachedSystemLayouts;
static std::vector<fs::path> cachedUserLayouts;
static std::vector<fs::path> cachedAllLayouts;
static bool layoutCacheDirty = true;

fs::path Layout::SystemPath(const std::string& name) { return SYSTEM_DIR / (name + ".json"); }
fs::path Layout::UserPath(const std::string& name)   { return USER_DIR   / (name + ".json"); }

bool Layout::Exists(const std::string& name) { return fs::exists(UserPath(name)); }

static fs::path ResolveLayoutPath(const std::string& name)
{
    fs::path sys = Layout::SystemPath(name);
    fs::path usr = Layout::UserPath(name);
    if (fs::exists(sys)) return sys;
    if (fs::exists(usr)) return usr;
    return {};
}

// Assigns stable layout IDs to any panel that doesn't yet have one.
// Panels created via AddPanel<T>() will already have IDs; this covers edge cases.
static void EnsureLayoutIDsAssigned()
{
    auto& counters = Naui::GetPanelTypeCounters();

    for (auto& [uid, panel] : Naui::GetAllPanels())
    {
        if (panel->GetWindowFlags() & ImGuiWindowFlags_NoSavedSettings)
            continue;
        if (!panel->GetLayoutID().empty())
            continue;

        const std::string& type = panel->GetTypeName();
        panel->SetLayoutID(type + "_" + std::to_string(counters[type]++));
    }
}

static nlohmann::json BuildPanelsJson()
{
    nlohmann::json panels;

    for (auto& [uid, panel] : Naui::GetAllPanels())
    {
        if (panel->GetWindowFlags() & ImGuiWindowFlags_NoSavedSettings)
            continue;

        panels[panel->GetLayoutID()] =
        {
            { "type",   panel->GetTypeName() },
            { "isOpen", panel->IsOpen()      }
        };
    }

    return panels;
}

// Destroys all panels and recreates them from the saved layout with their stable IDs.
// Panels are given "###layoutID" titles so ImGui can match them when the ini is loaded.
// Returns the number of panels successfully created
static int RecreatePanelsFromLayout(const nlohmann::json& panelsJson)
{
    Naui::ResetPanelTypeCounters();
    Naui::DestroyAllPanels();

    int loaded = 0;

    for (auto& [layoutID, entry] : panelsJson.items())
    {
        if (!entry.is_object())
        {
            std::cerr << "Layout::Load - skipping malformed entry for layoutID: " << layoutID << "\n";
            continue;
        }

        if (!entry.contains("type") || !entry["type"].is_string())
        {
            std::cerr << "Layout::Load - missing or invalid 'type' for layoutID: " << layoutID << "\n";
            continue;
        }

        if (!entry.contains("isOpen") || !entry["isOpen"].is_boolean())
        {
            std::cerr << "Layout::Load - missing or invalid 'isOpen' for layoutID: " << layoutID << "\n";
            continue;
        }

        const std::string type   = Naui::NormalizeTypeName(entry["type"].get<std::string>().c_str());
        const bool        isOpen = entry["isOpen"].get<bool>();

        Naui::Panel* panel = Naui::CreatePanelByType(type, layoutID);
        if (!panel)
        {
            std::cerr << "Layout::Load - unknown panel type: " << type << "\n";
            continue;
        }

        panel->SetOpen(isOpen);
        loaded++;
    }

    return loaded;
}

// Fallback — creates one instance of every registered panel type, all closed.
// Panels still exist so they remain accessible via the menu bar.
static void CreateFallbackPanels()
{
    Naui::ResetPanelTypeCounters();
    Naui::DestroyAllPanels();

    for (auto& [typeName, factory] : Naui::GetPanelFactories())
    {
        auto& counters             = Naui::GetPanelTypeCounters();
        const int idx              = counters[typeName]++;
        const std::string layoutID = typeName + "_" + std::to_string(idx);

        Naui::Panel* panel = Naui::CreatePanelByType(typeName, layoutID);
        if (!panel)
            continue;

        panel->SetOpen(false);
        std::cout << "Layout::Load - created fallback panel: " << typeName << "\n";
    }
}

bool Layout::Load(const std::string& name)
{
    const fs::path path = ResolveLayoutPath(name);
    if (path.empty())
        return false;

    nlohmann::json j;
    std::ifstream file(path);
    file >> j;

    // Step 1 — Recreate panels from file; fall back if none loaded successfully
    if (j.contains("panels"))
    {
        const int loaded = RecreatePanelsFromLayout(j["panels"]);
        if (loaded == 0)
        {
            std::cerr << "Layout::Load - no panels loaded from '" << name << "', falling back to defaults\n";
            CreateFallbackPanels();
            return false;
        }
    }
    else
    {
        CreateFallbackPanels();
        return false;
    }

    // Step 2 — Feed ImGui its ini data; windows are matched by ###layoutID
    if (j.contains("data"))
        ImGui::LoadIniSettingsFromMemory(j["data"].get<std::string>().c_str());

    return true;
}

bool Layout::LoadDefault()
{
    return Load("Default");
}

bool Layout::Save(const std::string& name, bool overwrite)
{
    fs::create_directories(USER_DIR);
    const fs::path path = USER_DIR / (name + ".json");

    if (!overwrite && fs::exists(path))
        return false;

    EnsureLayoutIDsAssigned();
    nlohmann::json j;
    j["panels"] = BuildPanelsJson();
    j["data"]   = ImGui::SaveIniSettingsToMemory();

    std::ofstream file(path);
    file << j.dump(4);

    return layoutCacheDirty = true;
}

bool Layout::Delete(const std::string& name)
{
    bool deleted = false;
    const fs::path user = UserPath(name);

    if (fs::exists(user))
    {
        std::error_code ec;
        fs::remove(user, ec);
        if (!ec)
            deleted = layoutCacheDirty = true;
    }

    return deleted;
}

static std::vector<fs::path> ListLayoutsIn(const fs::path& folderPath)
{
    std::vector<fs::path> result;
    if (!fs::exists(folderPath))
        return result;

    for (auto& entry : fs::directory_iterator(folderPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            result.push_back(entry.path().stem().string());
    }

    return result;
}

void Layout::RefreshLayoutCache()
{
    cachedSystemLayouts = ListLayoutsIn(SYSTEM_DIR);
    cachedUserLayouts   = ListLayoutsIn(USER_DIR);
    layoutCacheDirty    = false;
}

const std::vector<fs::path>& Layout::GetSystemLayouts()
{
    if (layoutCacheDirty) RefreshLayoutCache();
    return cachedSystemLayouts;
}

const std::vector<fs::path>& Layout::GetUserLayouts()
{
    if (layoutCacheDirty) RefreshLayoutCache();
    return cachedUserLayouts;
}

const std::vector<fs::path>& Layout::GetAllLayouts()
{
    if (layoutCacheDirty)
    {
        cachedAllLayouts = GetSystemLayouts();
        auto usr = GetUserLayouts();
        cachedAllLayouts.insert(cachedAllLayouts.end(), usr.begin(), usr.end());
    }

    return cachedAllLayouts;
}