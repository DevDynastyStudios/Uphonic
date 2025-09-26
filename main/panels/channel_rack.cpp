#include "panel_manager.h"

struct UphChannelRack {
    float fader;
};

static UphChannelRack channel_data { };

static void uph_channel_rack_render(UphPanel* panel) {
    UphChannelRack* data = (UphChannelRack*) panel->panel_data;
    ImGui::Text("Channel Rack");
    ImGui::SliderFloat("Fader", &data->fader, 0.0f, 1.0f);
}

UPH_REGISTER_PANEL("Channel Rack", &channel_data, ImGuiWindowFlags_None, uph_channel_rack_render);