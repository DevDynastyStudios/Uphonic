#include "panel_manager.h"
#include "types.h"

#include <imgui_internal.h>
#include <imgui-knobs.h>
#include <cmath>

struct UphAudioMixer
{
    int selected_track;
};

static UphAudioMixer mixer_data;

static void uph_audio_mixer_init(UphPanel* panel)
{
	panel->category = UPH_CATEGORY_EDITOR;
}

static void DrawVUMeterWithFader(float vuLevelLeft, float vuLevelRight, float& volume, float width, float height, uint32_t channelColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    float meterWidth = width / 2.0f - 1.0f;
    
    // Left Channel Background
    draw->AddRectFilled(pos, ImVec2(pos.x + meterWidth, pos.y + height), 
        IM_COL32(30, 30, 30, 255));
    
    // Right Channel Background
    draw->AddRectFilled(ImVec2(pos.x + meterWidth + 2, pos.y), 
        ImVec2(pos.x + width, pos.y + height), 
        IM_COL32(30, 30, 30, 255));
    
    // VU Meter Segments
    float segments = 30;
    float segHeight = height / segments;
    float segSpacing = 1.0f;
    
    // Draw Left Channel
    for (int i = 0; i < segments; i++)
    {
        float segLevel = (float)i / segments;
        if (segLevel <= vuLevelLeft)
        {
            ImU32 color;
            if (segLevel > 0.85f)
                color = IM_COL32(255, 50, 50, 255); // Red
            else if (segLevel > 0.7f)
                color = IM_COL32(255, 200, 50, 255); // Yellow
            else
                color = IM_COL32(50, 255, 100, 255); // Green
            
            float y = pos.y + height - (i + 1) * segHeight;
            draw->AddRectFilled(
                ImVec2(pos.x + 1, y + segSpacing),
                ImVec2(pos.x + meterWidth - 1, y + segHeight - segSpacing),
                color
            );
        }
    }
    
    // Draw Right Channel
    for (int i = 0; i < segments; i++)
    {
        float segLevel = (float)i / segments;
        if (segLevel <= vuLevelRight)
        {
            ImU32 color;
            if (segLevel > 0.85f)
                color = IM_COL32(255, 50, 50, 255); // Red
            else if (segLevel > 0.7f)
                color = IM_COL32(255, 200, 50, 255); // Yellow
            else
                color = IM_COL32(50, 255, 100, 255); // Green
            
            float y = pos.y + height - (i + 1) * segHeight;
            draw->AddRectFilled(
                ImVec2(pos.x + meterWidth + 3, y + segSpacing),
                ImVec2(pos.x + width - 1, y + segHeight - segSpacing),
                color
            );
        }
    }
    
    // Volume triangle indicator
    float volumeY = pos.y + height - (volume * height);
    
    // Left pointing triangle
    ImVec2 p1 = ImVec2(pos.x + width + 2, volumeY);
    ImVec2 p2 = ImVec2(pos.x + width + 10, volumeY - 5);
    ImVec2 p3 = ImVec2(pos.x + width + 10, volumeY + 5);
    draw->AddTriangleFilled(p1, p2, p3, channelColor);
    
    // Make it interactive
    ImGui::InvisibleButton("##vumeter", ImVec2(width + 12, height));
    
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 0.0f))
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        float newVolume = 1.0f - ((mousePos.y - pos.y) / height);
        volume = ImClamp(newVolume, 0.0f, 1.0f);
    }
    
    // Hover feedback
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
}

static void DrawChannelStrip(int idx, bool isSelected)
{
    ImGui::PushID(idx);

    float stripWidth = 80.0f;

    if (isSelected)
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));

    ImGui::BeginChild("Strip", ImVec2(stripWidth, 0), 0, ImGuiWindowFlags_NoScrollbar);

    if (isSelected)
        ImGui::PopStyleColor();

    // selection logic
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        mixer_data.selected_track = idx;

    UphTrack &track = app->project.tracks[idx];

    // Channel color bar
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    draw->AddRectFilled(pos, ImVec2(pos.x + stripWidth - 16, pos.y + 3), track.color);
    ImGui::Dummy(ImVec2(0, 5));
    
    // Channel name
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##name", track.name, sizeof(track.name));
    
    ImGui::Spacing();

    // Pan knob
    ImGui::SetCursorPosX((stripWidth - 40) * 0.5f);
    ImGuiKnobs::Knob("Pan", &track.pan, -1.0f, 1.0f, 0.01f, "%.2f", 
        ImGuiKnobVariant_Wiper, 40);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    // Combined VU Meter and Volume Fader
    ImGui::SetCursorPosX((stripWidth - 24) * 0.5f);
    DrawVUMeterWithFader(track.peak_left, track.peak_right, track.volume, 24, 150, track.color);
    
    ImGui::Spacing();
    
    // Volume dB display
    float db = track.volume > 0.0f ? 20.0f * log10f(track.volume) : -60.0f;
    ImGui::Text(" %.1f dB", db);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Solo button
    if (track.solo)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    }
    
    if (ImGui::Button("S", ImVec2(25, 25)))
    {
        track.solo = !track.solo;
        if (track.solo)
        {
            for (UphTrack &track : app->project.tracks)
                track.solo = false;
            track.solo = true;
            app->solo_track_index = idx;
        }
        else
            app->solo_track_index = -1;
    }
    ImGui::PopStyleColor(2);
    
    ImGui::SameLine();
    
    // Mute button
    if (track.muted)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    }
    
    if (ImGui::Button("M", ImVec2(25, 25)))
    {
        track.muted = !track.muted;
    }
    ImGui::PopStyleColor(2);

    ImGui::EndChild();

    ImGui::PopID();
}

static void uph_mixer_render(UphPanel* panel)
{
    ImGui::BeginChild("MixerStrips", ImVec2(0, 0), false, 
        ImGuiWindowFlags_HorizontalScrollbar);
    
    for (uint32_t i = 0; i < app->project.tracks.size(); i++)
    {
        if (i > 0) ImGui::SameLine();
        DrawChannelStrip(i, i == mixer_data.selected_track);
    }
    
    ImGui::EndChild();
}

UPH_REGISTER_PANEL("Mixer Track", UphPanelFlags_Panel, uph_mixer_render, uph_audio_mixer_init);