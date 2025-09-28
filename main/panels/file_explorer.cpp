#include "panel_manager.h"
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

struct UphFileExplorer
{
    fs::path current_dir = fs::current_path();
    fs::path selected_dir = fs::current_path();
    bool show_file_list = false; // toggle for secondary panel
};

static UphFileExplorer explorer_data {};

// Format file size in KB/MB
static std::string format_size(uintmax_t size)
{
    if (size < 1024) return std::to_string(size) + " B";
    double kb = size / 1024.0;
    if (kb < 1024) return std::to_string((int)kb) + " KB";
    double mb = kb / 1024.0;
    if (mb < 1024) return std::to_string((int)mb) + " MB";
    double gb = mb / 1024.0;
    return std::to_string((int)gb) + " GB";
}

// Format last write time
static std::string format_time(const fs::file_time_type& ftime)
{
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm tm = *std::localtime(&tt);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    return buf;
}

static void render_directory_tree(const fs::path& dir, fs::path& selected_dir)
{
    std::vector<fs::directory_entry> entries;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_directory())
            entries.push_back(entry);
    }
    std::sort(entries.begin(), entries.end(),
              [](auto& a, auto& b){ return a.path().filename() < b.path().filename(); });

    for (auto& entry : entries) {
        const auto& path = entry.path();
        std::string name = path.filename().string();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (path == selected_dir)
            flags |= ImGuiTreeNodeFlags_Selected;

        bool open = ImGui::TreeNodeEx(name.c_str(), flags);
        if (ImGui::IsItemClicked()) {
            selected_dir = path;
        }

        if (open) {
            render_directory_tree(path, selected_dir);
            ImGui::TreePop();
        }
    }
}

static void render_file_list(const fs::path& dir)
{
    if (!fs::exists(dir)) return;

    // Collect entries
    std::vector<fs::directory_entry> dirs, files;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) dirs.push_back(entry);
        else files.push_back(entry);
    }
    auto sorter = [](auto& a, auto& b){ return a.path().filename() < b.path().filename(); };
    std::sort(dirs.begin(), dirs.end(), sorter);
    std::sort(files.begin(), files.end(), sorter);

    ImGui::Columns(3, "filelist");
    ImGui::Text("Name"); ImGui::NextColumn();
    ImGui::Text("Size"); ImGui::NextColumn();
    ImGui::Text("Modified"); ImGui::NextColumn();
    ImGui::Separator();

    auto render_entry = [&](fs::directory_entry& entry) {
        const auto& path = entry.path();
        std::string name = path.filename().string();

        // Name column
        if (ImGui::Selectable(name.c_str(), false,
                              ImGuiSelectableFlags_SpanAllColumns)) {
            // Single click
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            // Double-click: open file/folder
            if (entry.is_directory())
                explorer_data.selected_dir = path;
            else {
                // TODO: hook into your "open file" logic
            }
        }
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Open")) {
                if (entry.is_directory())
                    explorer_data.selected_dir = path;
                else {
                    // TODO: open file
                }
            }
            if (ImGui::MenuItem("Delete")) {
                // TODO: delete logic
            }
            ImGui::EndPopup();
        }
        ImGui::NextColumn();

        // Size column
        if (entry.is_directory()) {
            ImGui::TextUnformatted("-");
        } else {
            try {
                auto sz = fs::file_size(path);
                ImGui::TextUnformatted(format_size(sz).c_str());
            } catch (...) {
                ImGui::TextUnformatted("?");
            }
        }
        ImGui::NextColumn();

        // Modified column
        try {
            auto ftime = fs::last_write_time(path);
            ImGui::TextUnformatted(format_time(ftime).c_str());
        } catch (...) {
            ImGui::TextUnformatted("?");
        }
        ImGui::NextColumn();
    };

    for (auto& d : dirs)  render_entry(d);
    for (auto& f : files) render_entry(f);

    ImGui::Columns(1);
}

static void uph_file_explorer_render(UphPanel* panel)
{
    // Toolbar row
    if (ImGui::Button(explorer_data.show_file_list ? "📂 Hide Files" : "📂 Show Files")) {
        explorer_data.show_file_list = !explorer_data.show_file_list;
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(explorer_data.selected_dir.string().c_str());

    ImGui::Separator();

    if (explorer_data.show_file_list) {
        ImGui::Columns(2, "explorer", true);

        // Left: directory tree
        if (ImGui::BeginChild("DirTree", ImVec2(0,0), true)) {
            render_directory_tree(explorer_data.current_dir.root_path(), explorer_data.selected_dir);
        }
        ImGui::EndChild();
        ImGui::NextColumn();

        // Right: file list
        if (ImGui::BeginChild("FileList", ImVec2(0,0), true)) {
            render_file_list(explorer_data.selected_dir);
        }
        ImGui::EndChild();

        ImGui::Columns(1);
    } else {
        // Only directory tree
        if (ImGui::BeginChild("DirTreeOnly", ImVec2(0,0), true)) {
            render_directory_tree(explorer_data.current_dir.root_path(), explorer_data.selected_dir);
        }
        ImGui::EndChild();
    }
}

UPH_REGISTER_PANEL("File Explorer", ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, uph_file_explorer_render);