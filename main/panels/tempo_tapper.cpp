#include "panel_manager.h"
#include "../settings/layout_manager.h"
#include "../types.h"

#include <imgui-knobs.h>

struct UphTempoTapper
{
    
};

static UphTempoTapper tapper_data {};

static void uph_tempo_tapper_render(UphPanel* panel)
{
    if(ImGui::Button("Save Layout"))
    {
        uph_save_layout("layouts/Default");
    }

    if(ImGui::Button("Load Layout"))
    {
        uph_load_layout("layouts/Default");
    }
    ImGui::SameLine();
    ImGuiKnobs::Knob("BPM", &app->project.bpm, 1.0f, 1000.0f, 1.0f, "%.1f", ImGuiKnobVariant_Wiper, 32.0f);
}

UPH_REGISTER_PANEL("Tempo Tapper", ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, uph_tempo_tapper_render);