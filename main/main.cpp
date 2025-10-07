#include "platform/platform.h"
#include "platform/event.h"
#include "platform/event_types.h"

#include <imgui.h>

#include <iostream>
#include <filesystem>

#include "sound_device.h"
#include "plugin_loader.h"

#include "panels/panel_manager.h"
#include "io/layout_manager.h"
#include "io/theme_loader.h"
#include "io/project_manager.h"
#include "types.h"

#include "../uvi/uvi_loader.h"

int main(const int argc, const char **argv)
{
    UphPlatformCreateInfo create_info = { 1920, 1080, "Uphonic" };

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;

    uph_platform_initialize(&create_info);

    uph_json_load_theme("themes/Default.json");
    uph_load_layout("layouts/Default");
	uph_project_init();

	std::vector<std::filesystem::path> recovery_candidates = uph_project_check_recovery();
	if(!recovery_candidates.empty())
	{
		uph_panel_show("Recover Project");
	}

    bool is_running = true;
    uph_event_connect(UphSystemEventCode::Quit, [&](void *data) { is_running = false; });
    uph_event_connect(UphSystemEventCode::FileDropped, [&](void *data) {
        UphFileDropEvent *event = (UphFileDropEvent*)data;
        UphSample sample = uph_create_sample_from_file(event->path);
        if (!sample.frames)
            return;
        app->project.samples.push_back(sample);
    });

    app = new UphApplication;
    app->project.patterns.push_back(UphMidiPattern{ "Pattern 1" });

    uph_sound_device_initialize();
	uph_panel_init_all();

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
		uph_layout_process_requests();
        uph_process_plugin_loader();
    }

    uph_sound_device_shutdown();
	uph_project_shutdown();
    uph_platform_shutdown();
    ImGui::DestroyContext();
    delete app;
    return 0;
}