#include "platform/platform.h"
#include "platform/event.h"
#include "platform/event_types.h"

#include <imgui.h>

#include "panels/panel_manager.h"
#include "settings/layout_manager.h"
#include "settings/theme_loader.h"
#include "types.h"

int main(const int argc, const char **argv)
{
    UphPlatformCreateInfo create_info = { 1920, 1080, "Uphonic" };

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;

    uph_json_load_theme("themes/Default.json");
    uph_platform_initialize(&create_info);
    uph_load_layout("layouts/Default");

    bool is_running = true;
    uph_event_connect(UphSystemEventCode::Quit, [&](void *data) { is_running = false; });

    app = new UphApplication;
    app->project.patterns.push_back(UphMidiPattern{ "Pattern 1" });
    app->current_pattern = &app->project.patterns.back();
    app->project.tracks[0].track_type = UphTrackType::Midi;
    app->project.tracks[0].midi_track.pattern_instances.push_back(UphMidiPatternInstance{ 0, 16.0f });

    while (is_running)
    {
        uph_platform_begin();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
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