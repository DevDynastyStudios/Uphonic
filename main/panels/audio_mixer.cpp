#include "panel_manager.h"

struct UphAudioMixer
{
    int selected_track;
    float fader;
};

static UphAudioMixer mixer_data { 0, 0.75f };

static void uph_mixer_render(UphPanel* panel) 
{
    UphAudioMixer* data = (UphAudioMixer*) panel->panel_data;
    ImGui::Text("Mixer Track: %d", data->selected_track);
    ImGui::SliderFloat("Fader", &data->fader, 0.0f, 1.0f);
}

UPH_REGISTER_PANEL("Mixer Track", &mixer_data, ImGuiWindowFlags_None, uph_mixer_render);