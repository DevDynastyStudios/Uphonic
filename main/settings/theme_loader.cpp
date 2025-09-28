#include "theme_loader.h"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_map>
#include <string>

// Map string keys in JSON -> ImGuiCol enum
static std::unordered_map<std::string, ImGuiCol> color_map = {
    {"Text", ImGuiCol_Text},
    {"TextDisabled", ImGuiCol_TextDisabled},
    {"WindowBg", ImGuiCol_WindowBg},
    {"ChildBg", ImGuiCol_ChildBg},
    {"PopupBg", ImGuiCol_PopupBg},
    {"Border", ImGuiCol_Border},
    {"BorderShadow", ImGuiCol_BorderShadow},
    {"FrameBg", ImGuiCol_FrameBg},
    {"FrameBgHovered", ImGuiCol_FrameBgHovered},
    {"FrameBgActive", ImGuiCol_FrameBgActive},
    {"TitleBg", ImGuiCol_TitleBg},
    {"TitleBgActive", ImGuiCol_TitleBgActive},
    {"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
    {"MenuBarBg", ImGuiCol_MenuBarBg},
    {"ScrollbarBg", ImGuiCol_ScrollbarBg},
    {"ScrollbarGrab", ImGuiCol_ScrollbarGrab},
    {"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
    {"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
    {"CheckMark", ImGuiCol_CheckMark},
    {"SliderGrab", ImGuiCol_SliderGrab},
    {"SliderGrabActive", ImGuiCol_SliderGrabActive},
    {"Button", ImGuiCol_Button},
    {"ButtonHovered", ImGuiCol_ButtonHovered},
    {"ButtonActive", ImGuiCol_ButtonActive},
    {"Header", ImGuiCol_Header},
    {"HeaderHovered", ImGuiCol_HeaderHovered},
    {"HeaderActive", ImGuiCol_HeaderActive},
    {"Separator", ImGuiCol_Separator},
    {"SeparatorHovered", ImGuiCol_SeparatorHovered},
    {"SeparatorActive", ImGuiCol_SeparatorActive},
    {"ResizeGrip", ImGuiCol_ResizeGrip},
    {"ResizeGripHovered", ImGuiCol_ResizeGripHovered},
    {"ResizeGripActive", ImGuiCol_ResizeGripActive},
    {"Tab", ImGuiCol_Tab},
    {"TabHovered", ImGuiCol_TabHovered},
    {"TabSelected", ImGuiCol_TabSelected},
    {"TabSelectedOverline", ImGuiCol_TabSelectedOverline},
    {"TabDimmed", ImGuiCol_TabDimmed},
    {"TabDimmedSelected", ImGuiCol_TabDimmedSelected},
    {"TabDimmedSelectedOverline", ImGuiCol_TabDimmedSelectedOverline},
    {"DockingPreview", ImGuiCol_DockingPreview},
    {"DockingEmptyBg", ImGuiCol_DockingEmptyBg},
    {"PlotLines", ImGuiCol_PlotLines},
    {"PlotLinesHovered", ImGuiCol_PlotLinesHovered},
    {"PlotHistogram", ImGuiCol_PlotHistogram},
    {"PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
    {"TableHeaderBg", ImGuiCol_TableHeaderBg},
    {"TableBorderStrong", ImGuiCol_TableBorderStrong},
    {"TableBorderLight", ImGuiCol_TableBorderLight},
    {"TableRowBg", ImGuiCol_TableRowBg},
    {"TableRowBgAlt", ImGuiCol_TableRowBgAlt},
    {"TextLink", ImGuiCol_TextLink},
    {"TextSelectedBg", ImGuiCol_TextSelectedBg},
    {"TreeLines", ImGuiCol_TreeLines},
    {"DragDropTarget", ImGuiCol_DragDropTarget},
    {"NavCursor", ImGuiCol_NavCursor},
    {"NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
    {"NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
    {"ModalWindowDimBg", ImGuiCol_ModalWindowDimBg}
};

// Convert "#RRGGBBAA" into ImVec4
static ImVec4 uph_color_from_hex(const std::string& hex)
{
    auto hexval = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return 0;
    };
    auto byte = [&](const char* p) -> int {
        return (hexval(p[0]) << 4) | hexval(p[1]);
    };

    int r = 255, g = 255, b = 255, a = 255;
    if (hex.size() >= 7) {
        r = byte(hex.c_str() + 1);
        g = byte(hex.c_str() + 3);
        b = byte(hex.c_str() + 5);
        if (hex.size() >= 9)
            a = byte(hex.c_str() + 7);
    }

    const float inv = 1.0f / 255.0f;
    return ImVec4(r * inv, g * inv, b * inv, a * inv);
}

void uph_json_load_theme(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return;

    nlohmann::json j;
    file >> j;

    ImVec4* colors = ImGui::GetStyle().Colors;

    for (auto& [key, value] : j.items())
    {
        auto it = color_map.find(key);
        if (it != color_map.end() && value.is_string())
        {
            colors[it->second] = uph_color_from_hex(value.get<std::string>());
        }
    }
}