#include "../platform/platform.h"
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
    bool show_file_list = true;
    bool simplified_view = true;
    bool details_dock_bottom = true; // new toggle
    bool search_popup_open = false;
    char search_buf[128] = "";
    
    float side_left_width = 320.0f;   // initial left pane width
    float bottom_top_height = 440.0f; // initial top pane height
};

static UphFileExplorer explorer_data {};

static void uph_file_explorer_init(UphPanel* panel)
{
	panel->category = UPH_CATEGORY_BROWSER;
}

static std::string format_size(uintmax_t size)
{
    static constexpr const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};		// It's only overkill if you can prove no one used it
    static constexpr int max_unit_count = sizeof(units) / sizeof(units[0]);
    int unit_index = 0;
    double reduced_size = (double) size;

    while(reduced_size >= 1024 && unit_index < max_unit_count - 1)
    {
        reduced_size /= 1024.0;
        unit_index++;
    }

    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(unit_index == 0 ? 0 : 1) << reduced_size << ' ' << units[unit_index];
    std::string out = oss.str();

    if (out.size() > 2 && out[out.size() - 3] == '.' && out[out.size() - 2] == '0')						// Trim trailing zeros
        out.erase(out.size() - 3, 2);

    return out;
}

static std::tm uph_localtime(std::time_t t)
{
    std::tm tm{};
    localtime_s(&tm, &t);
    return tm;
}

static std::tm uph_localtime(const std::filesystem::file_time_type& ftime)
{
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
    );

    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    return uph_localtime(tt);
}

std::string format_time(const fs::file_time_type& ftime)
{
    std::tm tm = uph_localtime(ftime);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

static void Splitter(bool vertical, float thickness, float* size1, float* size2, float min_size1, float min_size2)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f,0.5f,0.5f,0.15f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f,0.5f,0.5f,0.25f));

    ImGui::Button("##splitter", vertical ? ImVec2(thickness, -1) : ImVec2(-1, thickness));

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemActive()) {
        float delta = vertical ? ImGui::GetIO().MouseDelta.x : ImGui::GetIO().MouseDelta.y;
        // Clamp moves
        if (delta < 0 && *size1 + delta < min_size1) delta = min_size1 - *size1;
        if (delta > 0 && *size2 - delta < min_size2) delta = *size2 - min_size2;
        *size1 += delta;
        *size2 -= delta;
    }
}

// --- Breadcrumb bar with / separators and border ---
static void render_breadcrumb(fs::path& selected_dir)
{
    // Framed child for a polished look
    ImGui::BeginChild("BreadcrumbBar", ImVec2(0, 28), true, ImGuiWindowFlags_NoScrollbar);

    // Build segments from root to current
    std::vector<fs::path> segments;
    fs::path p = selected_dir;
    // Collect up to the root
    while (!p.empty()) {
        segments.push_back(p);
        if (p == p.root_path()) break;
        p = p.parent_path();
    }
    std::reverse(segments.begin(), segments.end());

    // If nothing (rare), show a single root
    if (segments.empty()) {
        segments.push_back(selected_dir.root_path());
    }

    // Render clickable path segments with tight spacing
    for (size_t i = 0; i < segments.size(); ++i) {
        const fs::path& seg = segments[i];

        // Segment label: filename, or drive/root if empty
        std::string label = seg.filename().string();
        if (label.empty()) {
            // Windows root like "C:\"
            std::string root = seg.root_name().string(); // "C:"
            std::string root_dir = seg.root_directory().string(); // "\" or "/"
            if(!root.empty())
                label = root;
            else if(label.empty())
                label = "\\";

            //if (!root.empty()) label = root;
            //if (!root_dir.empty() && label.find(root_dir) == std::string::npos) label += root_dir;
            //if (label.empty()) label = "\\"; // POSIX root
        }

ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
if (ImGui::SmallButton(label.c_str())) {
    selected_dir = seg;
}
ImGui::PopStyleVar();

        if (i + 1 < segments.size()) {
            ImGui::SameLine(0.0f, 0.0f);       // tight spacing before separator
            ImGui::TextUnformatted("\\");
            ImGui::SameLine(0.0f, 0.0f);       // tight spacing after separator
        }
    }

    ImGui::EndChild();
}

// --- Advanced tree with auto-expansion ---
static void render_directory_tree(const fs::path& dir, const fs::path& selected_dir, const fs::path& expand_to)
{
    std::vector<fs::directory_entry> entries;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_directory())
            entries.push_back(entry);
    }
    std::sort(entries.begin(), entries.end(), [](auto& a, auto& b) 
    { 
        return a.path().filename() < b.path().filename(); 
    });

    for (auto& entry : entries) 
    {
        const auto& path = entry.path();
        std::string name = path.filename().string();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (path == selected_dir)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Auto-open nodes along the expand_to path
        bool should_open = expand_to.string().rfind(path.string(), 0) == 0;
        bool open = ImGui::TreeNodeEx(name.c_str(), flags | (should_open ? ImGuiTreeNodeFlags_DefaultOpen : 0));

        if (ImGui::IsItemClicked())
            explorer_data.selected_dir = path;

        if (open) 
        {
            render_directory_tree(path, selected_dir, expand_to);
            ImGui::TreePop();
        }
    }
}

static bool dir_has_content(const fs::path& p)
{
    std::error_code ec;
    if (!fs::exists(p, ec)) return false;
    try {
        for (auto it = fs::directory_iterator(p); it != fs::directory_iterator(); ++it) {
            return true; // found at least one entry
        }
    } catch (...) {
        // permissions or other errors: treat as having content to avoid bounce
        return true;
    }
    return false;
}

static bool dir_is_empty(const fs::path& p)
{
    std::error_code ec;
    if (!fs::exists(p, ec)) return true;
    try {
        return fs::directory_iterator(p) == fs::directory_iterator();
    } catch (...) {
        // If we can't enumerate (permissions, etc.), treat as non-empty
        return false;
    }
}

static void render_simplified_tree(const fs::path& dir, fs::path& selected_dir)
{
    if (!fs::exists(dir)) return;

    ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
    bool root_open = ImGui::TreeNodeEx(dir.filename().string().c_str(), root_flags);

    if (ImGui::IsItemClicked()) {
        selected_dir = dir;
    }

    if (root_open) {
        for (auto& entry : fs::directory_iterator(dir)) {
            if (!entry.is_directory()) continue;

            const fs::path child = entry.path();
            bool empty = dir_is_empty(child);

            // Render node
            bool child_open = ImGui::TreeNodeEx(child.filename().string().c_str(),
                                                ImGuiTreeNodeFlags_OpenOnArrow);

            // Detect arrow toggle
            if (ImGui::IsItemToggledOpen()) {
                if (!empty) {
                    // Instead of toggling, navigate into folder
                    selected_dir = child;
                    // Force close the node so it doesn’t stay expanded
                    if (child_open) ImGui::TreePop();
                    continue; // skip normal rendering
                }
                // If empty, let ImGui handle open/close normally
            }

            // Detect label click
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                if (!empty) {
                    selected_dir = child;
                }
            }

            if (child_open) {
                ImGui::TreePop();
            }
        }

        // Files in current directory
        for (auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_directory()) continue;
            std::string fname = entry.path().filename().string();
            if (ImGui::Selectable(fname.c_str(), false)) {
                // TODO: open or preview file
            }
        }

        ImGui::TreePop();
    } else {
        // Collapsing root → jump back up
        fs::path parent = dir.parent_path();
        if (!parent.empty() && parent != dir) {
            selected_dir = parent;
        } else {
            selected_dir = dir.root_path();
        }
    }
}

// --- File list with search filter ---
static void render_file_list(const fs::path& dir, const char* filter)
{
    if (!fs::exists(dir)) return;

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

        // Apply filter
        if (filter && filter[0] != '\0') {
            if (name.find(filter) == std::string::npos)
                return;
        }

        if (ImGui::Selectable(name.c_str(), false,
                              ImGuiSelectableFlags_SpanAllColumns)) {
            // Single click
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (entry.is_directory())
                explorer_data.selected_dir = path;
            else {
                // TODO: open file
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

// --- Main render ---
static void uph_file_explorer_render(UphPanel* panel)
{
// --- Top bar ---
{
    // Left: search opens popup
    if (ImGui::Button("🔍 Search")) {
        explorer_data.search_popup_open = true;
        ImGui::OpenPopup("SearchPopup");
    }

    // Right: options dropdown
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 120);
    if (ImGui::BeginCombo("##Options", "⚙ Options")) {
        if (ImGui::MenuItem(explorer_data.show_file_list ? "📂 Hide File Details" : "📂 Show File Details"))
            explorer_data.show_file_list = !explorer_data.show_file_list;

        if (ImGui::MenuItem(explorer_data.simplified_view ? "🌳 Switch to Advanced View" : "🌿 Switch to Simplified View"))
            explorer_data.simplified_view = !explorer_data.simplified_view;

        if (ImGui::MenuItem(explorer_data.details_dock_bottom ? "↔ Dock Details to Side" : "↕ Dock Details to Bottom"))
            explorer_data.details_dock_bottom = !explorer_data.details_dock_bottom;

        ImGui::EndCombo();
    }

    // Search popup
    if (ImGui::BeginPopup("SearchPopup")) {
        ImGui::SetNextItemWidth(250.0f);
        ImGui::InputText("##search", explorer_data.search_buf, sizeof(explorer_data.search_buf));
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
            explorer_data.search_popup_open = false;
        }
        ImGui::EndPopup();
    }
}
    // --- Breadcrumb bar ---
    render_breadcrumb(explorer_data.selected_dir);
    ImGui::Separator();

    // If details hidden, still frame the tree for consistent visuals
    if (!explorer_data.show_file_list) {
        ImGui::BeginChild("TreeOnly", ImVec2(0,0), true);
        if (explorer_data.simplified_view)
            render_simplified_tree(explorer_data.selected_dir, explorer_data.selected_dir);
        else
            render_directory_tree(explorer_data.current_dir.root_path(),
                                  explorer_data.selected_dir,
                                  explorer_data.selected_dir);
        ImGui::EndChild();
        return;
    }

    // Layout with persistent pixel sizes
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float splitter_thickness = 4.0f;

    if (explorer_data.details_dock_bottom) {
        // Top/Bottom layout (works in both simplified and advanced modes)
        float top_h    = explorer_data.bottom_top_height;
        float bottom_h = std::max(50.0f, avail.y - top_h - splitter_thickness);
        top_h          = std::max(50.0f, avail.y - bottom_h - splitter_thickness);

        ImGui::BeginChild("Tree_Bottom", ImVec2(avail.x, top_h), true);
        if (explorer_data.simplified_view)
            render_simplified_tree(explorer_data.selected_dir, explorer_data.selected_dir);
        else
            render_directory_tree(explorer_data.current_dir.root_path(),
                                  explorer_data.selected_dir,
                                  explorer_data.selected_dir);
        ImGui::EndChild();

        // Splitter (horizontal)
        Splitter(false, splitter_thickness, &top_h, &bottom_h, 50.0f, 50.0f);
        explorer_data.bottom_top_height = top_h; // persist pixel height

        ImGui::BeginChild("FileList_Bottom", ImVec2(avail.x, bottom_h), true);
        render_file_list(explorer_data.selected_dir, explorer_data.search_buf);
        ImGui::EndChild();

    } else {
        // Left/Right layout (works in both simplified and advanced modes)
        float left_w  = explorer_data.side_left_width;
        float right_w = std::max(100.0f, avail.x - left_w - splitter_thickness);
        left_w        = std::max(100.0f, avail.x - right_w - splitter_thickness);

        ImGui::BeginChild("Tree_Side", ImVec2(left_w, avail.y), true);
        if (explorer_data.simplified_view)
            render_simplified_tree(explorer_data.selected_dir, explorer_data.selected_dir);
        else
            render_directory_tree(explorer_data.current_dir.root_path(),
                                  explorer_data.selected_dir,
                                  explorer_data.selected_dir);
        ImGui::EndChild();

        ImGui::SameLine();
        // Splitter (vertical)
        Splitter(true, splitter_thickness, &left_w, &right_w, 100.0f, 100.0f);
        explorer_data.side_left_width = left_w; // persist pixel width

        ImGui::SameLine();
        ImGui::BeginChild("FileList_Side", ImVec2(right_w, avail.y), true);
        render_file_list(explorer_data.selected_dir, explorer_data.search_buf);
        ImGui::EndChild();
    }
}

UPH_REGISTER_PANEL("File Explorer", UphPanelFlags_Panel, uph_file_explorer_render, uph_file_explorer_init);