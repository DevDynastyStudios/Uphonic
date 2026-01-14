#include "Style.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace Naui
{

static ImVec2 ReadVec2(const nlohmann::json& j)
{
    return ImVec2(j[0].get<float>(), j[1].get<float>());
}

void Style::Load(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return;

    nlohmann::json j;
    file >> j;

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle(); // reset to avoid carryover

    // --- Spacing ---
    if (j.contains("WindowPadding"))     style.WindowPadding     = ReadVec2(j["WindowPadding"]);
    if (j.contains("FramePadding"))      style.FramePadding      = ReadVec2(j["FramePadding"]);
    if (j.contains("ItemSpacing"))       style.ItemSpacing       = ReadVec2(j["ItemSpacing"]);
    if (j.contains("ItemInnerSpacing"))  style.ItemInnerSpacing  = ReadVec2(j["ItemInnerSpacing"]);
    if (j.contains("IndentSpacing"))     style.IndentSpacing     = j["IndentSpacing"];

    // --- Rounding ---
    if (j.contains("WindowRounding"))    style.WindowRounding    = j["WindowRounding"];
    if (j.contains("ChildRounding"))     style.ChildRounding     = j["ChildRounding"];
    if (j.contains("FrameRounding"))     style.FrameRounding     = j["FrameRounding"];
    if (j.contains("PopupRounding"))     style.PopupRounding     = j["PopupRounding"];
    if (j.contains("ScrollbarRounding")) style.ScrollbarRounding = j["ScrollbarRounding"];
    if (j.contains("GrabRounding"))      style.GrabRounding      = j["GrabRounding"];
    if (j.contains("TabRounding"))       style.TabRounding       = j["TabRounding"];

    // --- Sizes ---
    if (j.contains("ScrollbarSize"))     style.ScrollbarSize     = j["ScrollbarSize"];
    if (j.contains("GrabMinSize"))       style.GrabMinSize       = j["GrabMinSize"];

    // --- Borders ---
    if (j.contains("WindowBorderSize"))  style.WindowBorderSize  = j["WindowBorderSize"];
    if (j.contains("FrameBorderSize"))   style.FrameBorderSize   = j["FrameBorderSize"];
    if (j.contains("PopupBorderSize"))   style.PopupBorderSize   = j["PopupBorderSize"];

    // --- Alignment ---
    if (j.contains("WindowTitleAlign"))  style.WindowTitleAlign  = ReadVec2(j["WindowTitleAlign"]);

    // --- AA ---
    if (j.contains("AntiAliasedLines"))  style.AntiAliasedLines  = j["AntiAliasedLines"];
    if (j.contains("AntiAliasedFill"))   style.AntiAliasedFill   = j["AntiAliasedFill"];

    // --- Fonts ---
    if (j.contains("Font"))
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        auto& f = j["Font"];
        ImFontConfig cfg{};
        cfg.OversampleH = f.value("OversampleH", 3);
        cfg.OversampleV = f.value("OversampleV", 3);
        cfg.RasterizerMultiply = f.value("RasterizerMultiply", 1.0f);
        cfg.PixelSnapH = false;

        float size = f.value("Size", 18.0f);

        io.FontDefault = io.Fonts->AddFontFromFileTTF(
            f["File"].get<std::string>().c_str(),
            size,
            &cfg
        );

        if (f.contains("BoldFile"))
        {
            io.Fonts->AddFontFromFileTTF(
                f["BoldFile"].get<std::string>().c_str(),
                size,
                &cfg
            );
        }
    }
}

void Style::LoadDefault()
{
    Load("Styles/Default.json");
}

}
