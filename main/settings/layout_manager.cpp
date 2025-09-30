#include "layout_manager.h"
#include "../panels/panel_manager.h"
#include <imgui.h>
#include <fstream>
#include <filesystem>
#include <ini.h>

#define LAYOUT_IDENTIFIER "UphPanels"
#define IMMUTABLE_IDENTIFIER "Immutable"
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

		if(panel.is_visible)
			out << panel.title << "=1\n";
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
		{
			panel.is_visible = false;
            continue;
		}

        panel.is_visible = (section[panel.title] == "1");
    }

    return true;
}

bool uph_remove_layout(const char* name) 
{
    if (uph_layout_has_header(name, IMMUTABLE_IDENTIFIER))
        return false;

    std::string filename = std::string(name) + INI_EXTENSION;
	if (std::filesystem::exists(filename))
    {
        std::filesystem::remove(filename);
        return true;
    }
	
    return false;
}

bool uph_layout_has_header(const char* name, const char* header)
{
    std::string filename = std::string(name) + INI_EXTENSION;
    if (!std::filesystem::exists(filename))
        return false;

    mINI::INIFile file(filename);
    mINI::INIStructure ini;
    if (!file.read(ini))
        return false;

    return ini.has(header);
}

bool uph_layout_is_immutable(const char* name)
{
	return uph_layout_has_header(name, IMMUTABLE_IDENTIFIER);
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
