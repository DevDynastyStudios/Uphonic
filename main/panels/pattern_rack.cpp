#include "panel_manager.h"

struct UphPatternRack 
{
    float fader;
};

static UphPatternRack pattern_data { };

static void uph_pattern_rack_render(UphPanel* panel) 
{
    ImGui::Text("Pattern Rack");
    ImGui::SliderFloat("Fader", &pattern_data.fader, 0.0f, 1.0f);
}

UPH_REGISTER_PANEL("Pattern Rack", ImGuiWindowFlags_None, uph_pattern_rack_render, false);