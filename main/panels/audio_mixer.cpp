#include "panel_manager.h"

struct UphAudioMixer
{
    int selected_track;
    float fader;
};

static UphAudioMixer mixer_data { 0, 0.75f };

static void uph_mixer_render(UphPanel* panel) 
{
    ImGui::Text("Mixer Track: %d", mixer_data.selected_track);
    ImGui::SliderFloat("Fader", &mixer_data.fader, 0.0f, 1.0f);
}

UPH_REGISTER_PANEL("Mixer Track", UphPanelFlags::Panel, uph_mixer_render, nullptr);