#include "platform/platform.h"
#include "platform/event.h"
#include "platform/event_types.h"

#include <imgui.h>
#include "panels/panel_manager.h"
#include "settings/layout_manager.h"
#include "types.h"

void uph_create_palette()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    
    // Text
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
    
    // Backgrounds
    colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    
    // Borders
    colors[ImGuiCol_Border]                 = ImVec4(0.27f, 0.27f, 0.29f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    
    // Frames
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.91f, 0.64f, 0.49f, 1.00f); // peach
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    
    // Titles
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // neutral, no blue
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    
    // Menus / Scrollbars
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.91f, 0.64f, 0.49f, 1.00f); // peach
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    
    // Check / Sliders
    colors[ImGuiCol_CheckMark]              = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.91f, 0.64f, 0.49f, 1.00f); // peach
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    
    // Buttons
    colors[ImGuiCol_Button]                 = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.91f, 0.64f, 0.49f, 1.00f); // peach
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    
    // Headers
    colors[ImGuiCol_Header]                 = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.91f, 0.64f, 0.49f, 1.00f); // peach
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    
    // Separators
    colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.91f, 0.64f, 0.49f, 0.78f); // peach
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    
    // Resize grips
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.91f, 0.64f, 0.49f, 0.60f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.91f, 0.64f, 0.49f, 0.90f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.94f, 0.46f, 0.54f, 1.00f);
    
    // Tabs
    colors[ImGuiCol_Tab]                    = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.91f, 0.64f, 0.49f, 1.00f); // peach
    colors[ImGuiCol_TabSelected]            = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.91f, 0.64f, 0.49f, 1.00f); // peach
    
    // Docking
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.94f, 0.46f, 0.54f, 0.70f); // pink
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    
    // Plots
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.91f, 0.64f, 0.49f, 1.00f); // peach
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    
    // Tables
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    // Tables
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.27f, 0.27f, 0.29f, 1.00f); // gray
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f); // lighter gray
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // transparent
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f); // subtle alt row
    
    // Links / Selection
    colors[ImGuiCol_TextLink]               = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.91f, 0.64f, 0.49f, 0.35f); // peach
    
    // Tree lines
    colors[ImGuiCol_TreeLines]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f); // gray
    
    // Drag & drop
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.94f, 0.46f, 0.54f, 0.90f); // pink highlight
    
    // Navigation
    colors[ImGuiCol_NavCursor]              = ImVec4(0.94f, 0.46f, 0.54f, 1.00f); // pink
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // white glow
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.10f, 0.10f, 0.11f, 0.60f); // dark dim
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.10f, 0.10f, 0.11f, 0.70f); // dark dim
}

int main(const int argc, const char **argv)
{
    UphPlatformCreateInfo create_info = { 1920, 1080, "Uphonic" };

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;

    uph_create_palette();
    uph_platform_initialize(&create_info);
    uph_load_layout("Default");

    bool is_running = true;
    uph_event_connect(UphSystemEventCode::Quit, [&](void *data) { is_running = false; });

    app = new UphApplication;

    while (is_running)
    {
        uph_platform_begin();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
        ImGui::ShowStyleEditor();
        uph_panel_render_all();
        ImGui::Render();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
        uph_platform_end();
    }

    uph_platform_shutdown();
    ImGui::DestroyContext();
    return 0;
}