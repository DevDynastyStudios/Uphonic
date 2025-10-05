#include "platform/platform.h"
#include "platform/event.h"
#include "platform/event_types.h"

#include <imgui.h>
#include <font_awesome.h>
#include <font_awesome.cpp>

#include <iostream>
#include <filesystem>

#include "sound_device.h"
#include "plugin_loader.h"

#include "panels/panel_manager.h"
#include "settings/layout_manager.h"
#include "settings/theme_loader.h"
#include "project_manager.h"
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

    uph_json_load_theme("themes/Default.json");
    uph_platform_initialize(&create_info);
    uph_load_layout("layouts/Default");

	std::vector<std::filesystem::path> recovery_candidates = uph_project_check_recovery();
	if(!recovery_candidates.empty())
	{
		uph_panel_show("Recover Project");
	}

    static const ImWchar icon_ranges[]{0xf000, 0xf3ff, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = 3;
    icons_config.OversampleV = 3;
    io.Fonts->AddFontDefault();
    ImFont *icon_font = io.Fonts->AddFontFromMemoryCompressedTTF((void*)font_awesome_data, font_awesome_size, 16.0f, &icons_config, icon_ranges);

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
    app->project.tracks[1].track_type = UphTrackType_Sample;

    uph_sound_device_initialize();
	uph_panel_init_all();

#if UPH_PLATFORM_WINDOWS					// <--------------------------------------------------------------------- Only doing this for Big Smoke to test
	//uph_load_vst2("C:\\Program Files\\VstPlugins\\Pianoteq 6 (64-bit).dll");
#elif UPH_PLATFORM_LINUX
    uph_load_vst2("/usr/local/lib/vst/DragonflyHallReverb-vst.so");
#endif

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
        uph_process_plugins();
    }

    uph_sound_device_shutdown();
    uph_platform_shutdown();
    ImGui::DestroyContext();
    delete app;
    return 0;
}