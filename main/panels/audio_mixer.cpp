#include "panel_manager.h"
#include "types.h"

#include <imgui_internal.h>
#include <imgui-knobs.h>
#include <cmath>

struct MixerChannel
{
    char name[32];
    float volume;
    float pan;
    float vuLevelLeft;
    float vuLevelRight;
    bool mute;
    bool solo;
    bool arm;
    ImVec4 color;
};

struct UphAudioMixer
{
    int selected_track;
    float fader;
    MixerChannel channels[8];
    int soloCount;
    float time;
    
    UphAudioMixer() : selected_track(0), fader(0.75f), soloCount(0), time(0.0f)
    {
        ImVec4 colors[] = {
            ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
            ImVec4(0.4f, 0.4f, 1.0f, 1.0f),
            ImVec4(1.0f, 1.0f, 0.4f, 1.0f),
            ImVec4(1.0f, 0.4f, 1.0f, 1.0f),
            ImVec4(0.4f, 1.0f, 1.0f, 1.0f),
            ImVec4(1.0f, 0.7f, 0.4f, 1.0f),
            ImVec4(0.7f, 0.4f, 1.0f, 1.0f),
        };
        
        for (int i = 0; i < 8; i++)
        {
            snprintf(channels[i].name, sizeof(channels[i].name), "Track %d", i + 1);
            channels[i].volume = 0.75f;
            channels[i].pan = 0.0f;
            channels[i].vuLevelLeft = 0.0f;
            channels[i].vuLevelRight = 0.0f;
            channels[i].mute = false;
            channels[i].solo = false;
            channels[i].arm = false;
            channels[i].color = colors[i];
        }
    }
};

static UphAudioMixer mixer_data;

static void DrawVUMeterWithFader(float vuLevelLeft, float vuLevelRight, float& volume, float width, float height, ImVec4 channelColor)
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
    ImU32 triangleColor = ImGui::ColorConvertFloat4ToU32(channelColor);
    
    // Left pointing triangle
    ImVec2 p1 = ImVec2(pos.x + width + 2, volumeY);
    ImVec2 p2 = ImVec2(pos.x + width + 10, volumeY - 5);
    ImVec2 p3 = ImVec2(pos.x + width + 10, volumeY + 5);
    draw->AddTriangleFilled(p1, p2, p3, triangleColor);
    
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

inline float lin_to_db(float lin) { return 20.0f * log10(lin); }
inline float db_to_lin(float db) { return pow(10.0f, db / 20.0f); }

static void DrawChannelStrip(MixerChannel& ch, int idx, bool isSelected)
{
    ImGui::PushID(idx);
    
    float stripWidth = 80.0f;
    
    // Highlight selected channel
    if (isSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
    }
    
    ImGui::BeginChild("Strip", 
                     ImVec2(stripWidth, 0), true, 
                     ImGuiWindowFlags_NoScrollbar);
    
    if (isSelected)
    {
        ImGui::PopStyleColor();
    }
    
    // Make channel selectable
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
    {
        mixer_data.selected_track = idx;
    }
    
    // Channel color bar
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    draw->AddRectFilled(pos, ImVec2(pos.x + stripWidth - 16, pos.y + 3), 
                       ImGui::ColorConvertFloat4ToU32(ch.color));
    ImGui::Dummy(ImVec2(0, 5));
    
    // Channel name
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##name", ch.name, sizeof(ch.name));
    
    ImGui::Spacing();
    UphTrack &track = app->project.tracks[0];

    // Pan knob
    ImGui::SetCursorPosX((stripWidth - 40) * 0.5f);
    ImGuiKnobs::Knob("Pan", &track.pan, -1.0f, 1.0f, 0.01f, "%.2f", 
        ImGuiKnobVariant_Wiper, 40);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    // Combined VU Meter and Volume Fader
    ImGui::SetCursorPosX((stripWidth - 24) * 0.5f);
    DrawVUMeterWithFader(track.peak_left, track.peak_right, track.volume, 24, 150, ch.color);
    
    ImGui::Spacing();
    
    // Volume dB display
    float db = track.volume > 0.0f ? 20.0f * log10f(track.volume) : -60.0f;
    ImGui::Text(" %.1f dB", db);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Solo button
    if (ch.solo)
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
        ch.solo = !ch.solo;
        mixer_data.soloCount += ch.solo ? 1 : -1;
    }
    ImGui::PopStyleColor(2);
    
    ImGui::SameLine();
    
    // Mute button
    if (ch.mute)
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
        ch.mute = !ch.mute;
    }
    ImGui::PopStyleColor(2);
    
    // Arm button
    if (ch.arm)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    }
    
    if (ImGui::Button("R", ImVec2(55, 25)))
    {
        ch.arm = !ch.arm;
    }
    ImGui::PopStyleColor(2);
    
    ImGui::EndChild();
    ImGui::PopID();
}

static void uph_mixer_render(UphPanel* panel)
{
    // Update VU meters based on actual channel data
    // NOTE: You should replace this with actual audio level data from your audio engine
    // For now, this is just a placeholder that decays the levels
    
    for (int i = 0; i < 8; i++)
    {
        MixerChannel& ch = mixer_data.channels[i];
        
        // Decay the levels if no input
        ch.vuLevelLeft *= 0.95f;
        ch.vuLevelRight *= 0.95f;
        
        // Apply mute and solo logic
        if (ch.mute || (mixer_data.soloCount > 0 && !ch.solo))
        {
            ch.vuLevelLeft *= 0.8f;
            ch.vuLevelRight *= 0.8f;
        }
    }
    
    // Render mixer strips
    ImGui::BeginChild("MixerStrips", ImVec2(0, 0), false, 
                     ImGuiWindowFlags_HorizontalScrollbar);
    
    for (int i = 0; i < 8; i++)
    {
        if (i > 0) ImGui::SameLine();
        DrawChannelStrip(mixer_data.channels[i], i, i == mixer_data.selected_track);
    }
    
    ImGui::EndChild();
}

// Helper function to set VU levels from your audio engine
// Call this from your audio processing code
static void uph_mixer_set_levels(int channel, float leftLevel, float rightLevel)
{
    if (channel >= 0 && channel < 8)
    {
        mixer_data.channels[channel].vuLevelLeft = ImClamp(leftLevel, 0.0f, 1.0f);
        mixer_data.channels[channel].vuLevelRight = ImClamp(rightLevel, 0.0f, 1.0f);
    }
}

UPH_REGISTER_PANEL("Mixer Track", UphPanelFlags::Panel, uph_mixer_render, nullptr);