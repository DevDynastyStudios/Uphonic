#include "platform/platform.h"
#include "platform/event.h"
#include "platform/event_types.h"

#include <imgui.h>
#include "panels/panel_manager.h"

void uph_create_panel_menus()
{
    
}

int main(const int argc, const char **argv)
{
    UphPlatformCreateInfo create_info = {.width = 1920, .height = 1080, .title = "Uphonic"};

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    uph_platform_initialize(&create_info);

    bool is_running = true;
    uph_event_connect(UphSystemEventCode::Quit, [&](void *data) { is_running = false; });

    while (is_running)
    {
        uph_platform_begin();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
        uph_panel_render_all();
        ImGui::Render();
        uph_platform_end();
    }

    uph_platform_shutdown();
    ImGui::DestroyContext();
    return 0;
}