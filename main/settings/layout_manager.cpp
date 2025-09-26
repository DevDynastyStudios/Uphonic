#include "layout_manager.h"
#include <imgui.h>
#include <fstream>
#include <sstream>

// Save current layout under a name
void uph_save_layout(const char* name) 
{
    ImGui::SaveIniSettingsToDisk(name);
}

bool uph_load_layout(const char* name) {
    ImGui::LoadIniSettingsFromDisk(name);
    return true;
}

void uph_remove_layout(const char* name) 
{
    std::remove(name);
}

// FIX THIS
std::vector<std::string> uph_list_layouts(const std::string& filename) 
{
    std::ifstream in(filename);
    std::vector<std::string> names;
    std::string line;

    return names;
}