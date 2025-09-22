#include "platform/platform.h"
#include "platform/event.h"
#include "platform/event_types.h"

#include <imgui.h>
#include "panels/panel_manager.h"

int main(const int argc, const char **argv)
{
    UphPlatformCreateInfo create_info = {.width = 1920, .height = 1080, .title = "Uphonic"};

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    uph_platform_initialize(&create_info);

    bool is_running = true;
    uph_event_connect(UphSystemEventCode::Quit, [&](void *data) { is_running = false; });

    while (is_running)
    {
        uph_platform_begin();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
        ImGui::ShowDemoWindow();
        uph_panel_renderer();
        ImGui::Render();
        uph_platform_end();
    }

    uph_platform_shutdown();
    ImGui::DestroyContext();
    return 0;
}