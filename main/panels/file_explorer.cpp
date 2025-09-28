#include "panel_manager.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

struct UphFileExplorer
{
    fs::path current_dir = fs::current_path();
};

static UphFileExplorer explorer_data {};

static void uph_file_explorer_render(UphPanel* panel)
{
    ImGui::TextUnformatted(explorer_data.current_dir.string().c_str());
    ImGui::Separator();

    // Up one directory
    if (explorer_data.current_dir.has_parent_path()) {
        if (ImGui::Button("..")) {
            explorer_data.current_dir = explorer_data.current_dir.parent_path();
        }
    }

    ImGui::Separator();

    // List entries
    for (auto& entry : fs::directory_iterator(explorer_data.current_dir)) {
        const auto& path = entry.path();
        std::string name = path.filename().string();

        if (entry.is_directory()) {
            if (ImGui::Selectable((name + "/").c_str(), false)) {
                explorer_data.current_dir = path;
            }
        } else {
            if (ImGui::Selectable(name.c_str(), false)) {
                // TODO: handle file selection (open, load, etc.)
            }
        }
    }

}

UPH_REGISTER_PANEL("File Explorer", ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, uph_file_explorer_render);