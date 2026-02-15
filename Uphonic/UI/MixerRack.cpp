#include "MixerRack.h"
#include "../Core/ProjectState.h"
#include "../Plugin/PluginManager.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <cstring>

constexpr float METER_MIN_DB = -60.0f;
constexpr float METER_MAX_DB = 0.0f;

// Meter ballistics
constexpr float PEAK_ATTACK  = 25.0f; // fast rise
constexpr float PEAK_RELEASE = 25.0f; // slow fall

// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------

static float LinearToMeter(float v)
{
    if (v <= 0.0f) return 0.0f;
    float db = 20.0f * log10f(v);
    float norm = (db - METER_MIN_DB) / (METER_MAX_DB - METER_MIN_DB);
    return std::clamp(norm, 0.0f, 1.0f);
}

static float MeterToLinear(float norm)
{
    float db = METER_MIN_DB + norm * (METER_MAX_DB - METER_MIN_DB);
    return (db <= METER_MIN_DB) ? 0.0f : powf(10.0f, db / 20.0f);
}

static float SmoothPeak(float current, float target, float dt)
{
    float speed = (target > current) ? PEAK_ATTACK : PEAK_RELEASE;
    return current * (1.0f - speed * dt) + target * speed * dt;
}

// ------------------------------------------------------------

MixerRack::MixerRack()
    : Naui::Panel("Mixer Rack"), m_selectedTrack(-1)
{
    SetMinSize(0.0f, 250.0f);
}

void MixerRack::OnRender()
{
    float stripAreaWidth =
        ImGui::GetContentRegionAvail().x - m_config.effectsPanelWidth;

    ImGui::BeginChild(
        "##MixerStrips",
        ImVec2(stripAreaWidth, 0),
        false,
        ImGuiWindowFlags_HorizontalScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    RenderMasterStrip(m_selectedTrack == -1);

    ProjectState& state = ProjectState::GetInstance();
    for (size_t i = 0; i < state.tracks.size(); i++)
    {
        ImGui::SameLine();
        RenderChannelStrip(i, i == m_selectedTrack);
    }

    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##EffectsPanel", ImVec2(0, 0), true);
    RenderEffectsPanel();
    ImGui::EndChild();
}

// ------------------------------------------------------------

void MixerRack::RenderMasterStrip(bool isSelected)
{
    ImGui::PushID(-1);
    ImGui::BeginChild(
        "##MasterStrip",
        ImVec2(m_config.stripWidth, 0),
        false,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        m_selectedTrack = -1;

    ProjectState& state = ProjectState::GetInstance();
    MasterTrack& master = state.masterTrack;

    float dt = ImGui::GetIO().DeltaTime;

    master.smoothPeakLeft  =
        SmoothPeak(master.smoothPeakLeft,  master.peakLeft,  dt);
    master.smoothPeakRight =
        SmoothPeak(master.smoothPeakRight, master.peakRight, dt);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImU32 color = IM_COL32(200, 100, 50, 255);

    draw->AddRectFilled(
        pos,
        ImVec2(pos.x + m_config.stripWidth - 16, pos.y + 3),
        color
    );

    ImGui::Dummy(ImVec2(0, 5));

    static char name[] = "Master";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##name", name, 7, ImGuiInputTextFlags_ReadOnly);

    ImGui::Spacing();
    ImGui::SetCursorPosX((m_config.stripWidth - m_config.knobSize) * 0.5f);
    ImGuiKnobs::Knob(
        "Pan",
        &master.pan,
        0.0f,
        1.0f,
        0.01f,
        "%.2f",
        ImGuiKnobVariant_Tick,
        m_config.knobSize
    );

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetCursorPosX((m_config.stripWidth - m_config.vuMeterWidth) * 0.5f);
    DrawVUMeterWithFader(
        master.smoothPeakLeft,
        master.smoothPeakRight,
        master.volume,
        m_config.vuMeterWidth,
        m_config.vuMeterHeight,
        color
    );

    ImGui::Spacing();

    float db =
        master.volume > 0.0f ? 20.0f * log10f(master.volume) : METER_MIN_DB;

    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat(
        "##masterDB", &db, 0.1f, METER_MIN_DB, 0.0f, "%.1f dB"))
    {
        master.volume =
            std::clamp(MeterToLinear(
                (db - METER_MIN_DB) /
                (METER_MAX_DB - METER_MIN_DB)),
                0.0f, 1.0f);
    }

    ImGui::EndChild();
    ImGui::PopID();
}

// ------------------------------------------------------------

void MixerRack::RenderChannelStrip(size_t idx, bool isSelected)
{
    ImGui::PushID((int)idx);
    ImGui::BeginChild(
        "##Strip",
        ImVec2(m_config.stripWidth, 0),
        false,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        m_selectedTrack = idx;

    ProjectState& state = ProjectState::GetInstance();
    AudioTrack& track = state.tracks[idx];

    float dt = ImGui::GetIO().DeltaTime;

    track.smoothPeakLeft =
        SmoothPeak(track.smoothPeakLeft, track.peakLeft, dt);
    track.smoothPeakRight =
        SmoothPeak(track.smoothPeakRight, track.peakRight, dt);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImU32 color = ImGui::ColorConvertFloat4ToU32(track.color);

    draw->AddRectFilled(
        pos,
        ImVec2(pos.x + m_config.stripWidth - 16, pos.y + 3),
        color
    );

    ImGui::Dummy(ImVec2(0, 5));

    char nameBuffer[256];
    strncpy(nameBuffer, track.name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';

    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer)))
        track.name = nameBuffer;

    ImGui::Spacing();
    ImGui::SetCursorPosX((m_config.stripWidth - m_config.knobSize) * 0.5f);
    ImGuiKnobs::Knob(
        "Pan",
        &track.pan,
        0.0f,
        1.0f,
        0.01f,
        "%.2f",
        ImGuiKnobVariant_Tick,
        m_config.knobSize
    );

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetCursorPosX((m_config.stripWidth - m_config.vuMeterWidth) * 0.5f);
    DrawVUMeterWithFader(
        track.smoothPeakLeft,
        track.smoothPeakRight,
        track.volume,
        m_config.vuMeterWidth,
        m_config.vuMeterHeight,
        color
    );

    ImGui::Spacing();

    float db =
        track.volume > 0.0f ? 20.0f * log10f(track.volume) : METER_MIN_DB;

    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat(
        "##trackDB", &db, 0.1f, METER_MIN_DB, 0.0f, "%.1f dB"))
    {
        track.volume =
            std::clamp(MeterToLinear(
                (db - METER_MIN_DB) /
                (METER_MAX_DB - METER_MIN_DB)),
                0.0f, 1.0f);
    }

    ImGui::EndChild();
    ImGui::PopID();
}

// ------------------------------------------------------------

void MixerRack::DrawVUMeterWithFader(
    float vuLeft,
    float vuRight,
    float& volume,
    float width,
    float height,
    ImU32 channelColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    float meterWidth = width * 0.5f - 1.0f;

    draw->AddRectFilled(
        pos,
        ImVec2(pos.x + width, pos.y + height),
        IM_COL32(30, 30, 30, 255)
    );

    float vuL = LinearToMeter(vuLeft);
    float vuR = LinearToMeter(vuRight);

    float segH = height / m_config.vuSegments;

    for (int i = 0; i < m_config.vuSegments; i++)
    {
        float level = (float)(i + 1) / m_config.vuSegments;
        float y = pos.y + height - (i + 1) * segH;

        ImU32 c =
            level > m_config.vuRedThreshold    ? IM_COL32(255, 50, 50, 255) :
            level > m_config.vuYellowThreshold ? IM_COL32(255, 200, 50, 255) :
                                                 IM_COL32(50, 255, 100, 255);

        if (level <= vuL)
            draw->AddRectFilled(
                ImVec2(pos.x + 1, y + 1),
                ImVec2(pos.x + meterWidth - 1, y + segH - 1),
                c);

        if (level <= vuR)
            draw->AddRectFilled(
                ImVec2(pos.x + meterWidth + 3, y + 1),
                ImVec2(pos.x + width - 1, y + segH - 1),
                c);
    }

    float volNorm = LinearToMeter(volume);
    float volY = pos.y + height - volNorm * height;

    draw->AddTriangleFilled(
        ImVec2(pos.x + width + 2, volY),
        ImVec2(pos.x + width + 10, volY - 5),
        ImVec2(pos.x + width + 10, volY + 5),
        channelColor
    );

    ImGui::InvisibleButton("##vumeter", ImVec2(width + 12, height));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
    {
        float norm =
            1.0f - ((ImGui::GetMousePos().y - pos.y) / height);
        volume =
            std::clamp(MeterToLinear(norm), 0.0f, 1.0f);
    }

    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
}

void MixerRack::RenderEffectsPanel()
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ProjectState& state = ProjectState::GetInstance();
    if (m_selectedTrack == -1)
    {
        ImGui::Text("Master Effects");
    }
    else if (m_selectedTrack >= 0 && m_selectedTrack < (int)state.tracks.size())
    {
        ImGui::Text("%s - Effects", state.tracks[m_selectedTrack].name.c_str());
    }
    else
    {
        ImGui::Text("No Track Selected");
    }
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    std::vector<PluginEffect>* effects = nullptr;
    if (m_selectedTrack == -1)
    {
        effects = &state.masterTrack.effects;
    }
    else if (m_selectedTrack >= 0 && m_selectedTrack < (int)state.tracks.size())
    {
        effects = &state.tracks[m_selectedTrack].effects;
    }
    
    if (!effects)
    {
        ImGui::TextDisabled("No track selected");
        return;
    }
    
    if (ImGui::Button("+ Add Effect", ImVec2(-1, 0)))
    {
        ImGui::OpenPopup("AddEffectPopup");
    }
    
    if (ImGui::BeginPopup("AddEffectPopup", ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::Text("Select UVI");
        ImGui::Separator();

        int pluginId = 0;
        
        for (const auto& path : state.settings.pluginSearchPaths)
        {
            if (!std::filesystem::exists(path)) continue;
            
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".vst3")
                {
                    ImGui::PushID(pluginId++);
                    if (ImGui::Selectable(entry.path().filename().replace_extension().string().c_str()))
                    {
                        PluginEffect newEffect;
                        PluginManager::LoadEffect(newEffect, entry.path());
                        effects->push_back(newEffect);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopID();
                }
            }
        }
        
        ImGui::EndPopup();
    }
    
    ImGui::Spacing();
    
    ImGui::BeginChild("##EffectsList", ImVec2(0, 0), false);
    
    for (int i = 0; i < (int)effects->size(); i++)
    {
        PluginEffect& effect = (*effects)[i];
        
        ImGui::PushID(i);
        
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 slotSize(ImGui::GetContentRegionAvail().x, 60.0f);
        
        ImU32 bgColor = IM_COL32(45, 45, 50, 255);
        ImU32 borderColor = IM_COL32(60, 60, 70, 255);
        
        draw->AddRectFilled(cursorPos, 
            ImVec2(cursorPos.x + slotSize.x, cursorPos.y + slotSize.y),
            bgColor, 4.0f);
        draw->AddRect(cursorPos, 
            ImVec2(cursorPos.x + slotSize.x, cursorPos.y + slotSize.y),
            borderColor, 4.0f);
        
        ImGui::Dummy(ImVec2(0, 5));
        ImGui::Indent(10.0f);
        
        if (effect.plugin)
        {
            ImGui::Text("%s", effect.plugin->GetName());
        }
        else
        {
            ImGui::TextDisabled("Empty Slot");
        }
        
        ImGui::Spacing();
        
        ImGui::BeginDisabled(!effect.plugin);
        if (ImGui::SmallButton("Open"))
        {
            PluginManager::OpenEffect(effect);
        }
        ImGui::EndDisabled();
        
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove"))
        {
            if (effect.plugin)
            {
                PluginManager::UnloadEffect(effect);
            }
            effects->erase(effects->begin() + i);
            ImGui::Unindent(10.0f);
            ImGui::PopID();
            break;
        }
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::SameLine();
        ImGui::BeginDisabled(i == 0);
        if (ImGui::ArrowButton("##up", ImGuiDir_Up))
        {
            std::swap((*effects)[i], (*effects)[i - 1]);
        }
        ImGui::EndDisabled();
        
        ImGui::SameLine();
        ImGui::BeginDisabled(i == (int)effects->size() - 1);
        if (ImGui::ArrowButton("##down", ImGuiDir_Down))
        {
            std::swap((*effects)[i], (*effects)[i + 1]);
        }
        ImGui::EndDisabled();
        ImGui::PopStyleVar();
        
        ImGui::Unindent(10.0f);
        ImGui::Dummy(ImVec2(0, 5));
        
        ImGui::PopID();
    }
    
    if (effects->empty())
    {
        ImGui::Spacing();
        ImGui::TextDisabled("No effects loaded");
        ImGui::TextDisabled("Click '+ Add Effect' to add one");
    }
    
    ImGui::EndChild();
}