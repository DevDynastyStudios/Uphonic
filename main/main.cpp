#include "platform/platform.h"
#include "platform/event.h"
#include "platform/event_types.h"

#include <imgui.h>
#include <font_awesome.h>
#include <font_awesome.cpp>

#include <iostream>
#include <filesystem>

#include "sound_device.h"

#include "panels/panel_manager.h"
#include "settings/layout_manager.h"
#include "settings/theme_loader.h"
#include "project_manager.h"
#include "types.h"

VstIntPtr VSTCALLBACK audioMasterCallbackFunction(
    AEffect* effect, VstInt32 opcode, VstInt32 index,
    VstIntPtr value, void* ptr, float opt
)
{
    switch (opcode)
    {
        case audioMasterVersion: return 2400;
        case audioMasterIdle:    return 0;
        case audioMasterGetSampleRate: return (VstIntPtr)44100;
        case audioMasterGetBlockSize:  return (VstIntPtr)512;
        case audioMasterCanDo:
        {
            const char* canDo = (const char*)ptr;
            if (!canDo) return 0;
            if (strcmp(canDo, "sendVstEvents") == 0) return 1;
            if (strcmp(canDo, "sendVstMidiEvent") == 0) return 1;
            if (strcmp(canDo, "receiveVstEvents") == 0) return 1;
            if (strcmp(canDo, "receiveVstMidiEvent") == 0) return 1;
            return 0;
        }
        case audioMasterGetTime:
        {
            VstTimeInfo* timeInfo = (VstTimeInfo*)ptr;
            if (!timeInfo) return 0;

            timeInfo->samplePos = 0.0;
            timeInfo->tempo = app->project.bpm;
            timeInfo->flags = kVstTempoValid;

            return (VstIntPtr)timeInfo;
        }
        default: return 0;
    }
}

void uph_load_vst2(const char* path)
{
	    for (int i = 0; i < 1; ++i)
    {
        typedef AEffect* (*VSTPluginMain)(audioMasterCallback audioMaster);
        UphLibrary vst_module = uph_load_library(path);
        VSTPluginMain entry = (VSTPluginMain)uph_get_proc_address(vst_module, "VSTPluginMain");
        if (!entry)
            entry = (VSTPluginMain)uph_get_proc_address(vst_module, "main");

        if (!entry)
            printf("Not a valid VST 2.x plugin.\n");

        AEffect* effect = entry(audioMasterCallbackFunction);
        effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, 44100.0f);
        effect->dispatcher(effect, effSetBlockSize, 0, 512, nullptr, 0);
        effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0);

        UphChildWindow child_window;
        if (effect->flags & effFlagsHasEditor) {
            ERect* rect = nullptr;
            effect->dispatcher(effect, effEditGetRect, 0, 0, &rect, 0);
            uint32_t width  = rect ? rect->right - rect->left : 400;
            uint32_t height = rect ? rect->bottom - rect->top : 300;
            const UphChildWindowCreateInfo child_window_create_info = {width, height, "VstHost"};
            child_window = uph_create_child_window(&child_window_create_info);
            effect->dispatcher(effect, effEditOpen, 0, 0, child_window.handle, 0);
        }

        app->project.tracks[i].instrument.effect = effect;
    }
}

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
    }

    uph_sound_device_shutdown();
    uph_platform_shutdown();
    ImGui::DestroyContext();
    delete app;
    return 0;
}