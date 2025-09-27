#include "panel_manager.h"
#include "../settings/layout_manager.h"

struct UphTempoTapper
{
    
};

static UphTempoTapper tapper_data {};

static void uph_tempo_tapper_render(UphPanel* panel)
{
    if(ImGui::Button("Save Layout"))
    {
        uph_save_layout("Default");
    }

    if(ImGui::Button("Load Layout"))
    {
        uph_load_layout("Default");
    }
}

UPH_REGISTER_PANEL("Tempo Tapper", ImGuiWindowFlags_None, uph_tempo_tapper_render);