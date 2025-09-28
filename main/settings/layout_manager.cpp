#include "layout_manager.h"
#include "../panels/panel_manager.h"
#include <imgui.h>
#include <fstream>
#include <filesystem>
#include <ini.h>

#define LAYOUT_IDENTIFIER "UphPanels"
#define INI_EXTENSION ".ini"

// Yes, I know about the flag bug with already existing ini files
void uph_save_layout(const char* name) {
    std::string filename = std::string(name) + INI_EXTENSION;
    ImGui::SaveIniSettingsToDisk(filename.c_str());
    std::ofstream out(filename, std::ios::app);
    out << "\n[" << LAYOUT_IDENTIFIER << "]\n";

    for (auto& panel : panels()) {
        if (panel.window_flags & ImGuiWindowFlags_NoSavedSettings)
            continue;

        out << panel.title << "=" << (panel.is_visible ? "1" : "0") << "\n";
    }
}

bool uph_load_layout(const char* name) {
    std::string filename = std::string(name) + INI_EXTENSION;
    if (!std::filesystem::exists(filename))
        return false;

    ImGui::LoadIniSettingsFromDisk(filename.c_str());

    mINI::INIFile file(filename);
    mINI::INIStructure ini;
    file.read(ini);
        
    auto& section = ini[LAYOUT_IDENTIFIER];
    for(UphPanel& panel : panels())
    {
        if(!section.has(panel.title))
            continue;

        panel.is_visible = (section[panel.title] == "1");
    }

    return true;
}

void uph_remove_layout(const char* name) 
{
    std::string filename = std::string(name) + INI_EXTENSION;
    std::remove(filename.c_str());
}

std::vector<std::string> uph_list_layouts(const char* directory) 
{
    std::vector<std::string> layouts;
    namespace fs = std::filesystem;

    if (!fs::exists(directory)) 
        return layouts;

    for (auto& entry : fs::directory_iterator(directory)) {
        if(!entry.is_regular_file())
            continue;

        auto path = entry.path();
        if (path.extension() == INI_EXTENSION)
            layouts.push_back(path.stem().string());

    }
    return layouts;
}
