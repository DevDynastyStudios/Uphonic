#include "MixerRack.h"
#include "../Core/ProjectState.h"
#include "../Plugin/PluginManager.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <cstring>

MixerRack::MixerRack() : Naui::Panel("Mixer Rack"), m_selectedTrack(-1)
{
    SetMinSize(0.0f, 250.0f);
}

void MixerRack::OnRender()
{
    float stripAreaWidth = ImGui::GetContentRegionAvail().x - m_config.effectsPanelWidth;
    
    ImGui::BeginChild("##MixerStrips", ImVec2(stripAreaWidth, 0), false, 
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
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

void MixerRack::RenderMasterStrip(bool isSelected)
{
    ImGui::PushID(-1);

    ImGui::BeginChild("##MasterStrip", ImVec2(m_config.stripWidth, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        m_selectedTrack = -1;

    ProjectState& state = ProjectState::GetInstance();
    MasterTrack& master = state.masterTrack;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImU32 masterColor = IM_COL32(200, 100, 50, 255);
    draw->AddRectFilled(pos, ImVec2(pos.x + m_config.stripWidth - 16, pos.y + 3), masterColor);
    ImGui::Dummy(ImVec2(0, 5));
    
    ImGui::SetNextItemWidth(-1);
    static char masterName[] = "Master";
    ImGui::InputText("##name", masterName, 7, ImGuiInputTextFlags_ReadOnly);
    
    ImGui::Spacing();

    ImGui::SetCursorPosX((m_config.stripWidth - m_config.knobSize) * 0.5f);
    ImGuiKnobs::Knob("Pan", &master.pan, 0.0f, 1.0f, 0.01f, "%.2f", ImGuiKnobVariant_Tick, m_config.knobSize);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Pan: %.2f", master.pan);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::SetCursorPosX((m_config.stripWidth - m_config.vuMeterWidth) * 0.5f);
    DrawVUMeterWithFader(master.peakLeft, master.peakRight, master.volume, 
                        m_config.vuMeterWidth, m_config.vuMeterHeight, masterColor);
    
    ImGui::Spacing();
    
    float db = master.volume > 0.0f ? 20.0f * log10f(master.volume) : m_config.minDecibel;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat("##masterDB", &db, 0.1f, m_config.minDecibel, 0.0f, "%.1f dB"))
    {
        master.volume = (db <= m_config.minDecibel) ? 0.0f : powf(10.0f, db / 20.0f);
        master.volume = std::clamp(master.volume, 0.0f, 1.0f);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImVec4 muteColor = master.muted ? 
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    ImVec4 muteHover = master.muted ? 
        ImVec4(1.0f, 0.6f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    
    ImGui::SetCursorPosX((m_config.stripWidth - 50) * 0.5f);
    
    ImGui::PushStyleColor(ImGuiCol_Button, muteColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, muteHover);
    
    if (ImGui::Button("M", ImVec2(m_config.buttonSize, m_config.buttonSize)))
    {
        master.muted = !master.muted;
    }
    ImGui::PopStyleColor(2);

    ImGui::EndChild();
    ImGui::PopID();
}

void MixerRack::RenderChannelStrip(size_t idx, bool isSelected)
{
    ImGui::PushID(static_cast<int>(idx));

    ImGui::BeginChild("##Strip", ImVec2(m_config.stripWidth, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        m_selectedTrack = idx;

    ProjectState& state = ProjectState::GetInstance();
    AudioTrack& track = state.tracks[idx];

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImU32 trackColor = ImGui::ColorConvertFloat4ToU32(track.color);
    draw->AddRectFilled(pos, ImVec2(pos.x + m_config.stripWidth - 16, pos.y + 3), trackColor);
    ImGui::Dummy(ImVec2(0, 5));
    
    ImGui::SetNextItemWidth(-1);
    char nameBuffer[256];
    strncpy(nameBuffer, track.name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    
    if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer)))
    {
        track.name = nameBuffer;
    }
    
    ImGui::Spacing();

    ImGui::SetCursorPosX((m_config.stripWidth - m_config.knobSize) * 0.5f);
    ImGuiKnobs::Knob("Pan", &track.pan, 0.0f, 1.0f, 0.01f, "%.2f", ImGuiKnobVariant_Tick, m_config.knobSize);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Pan: %.2f", track.pan);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::SetCursorPosX((m_config.stripWidth - m_config.vuMeterWidth) * 0.5f);
    DrawVUMeterWithFader(track.peakLeft, track.peakRight, track.volume, 
                        m_config.vuMeterWidth, m_config.vuMeterHeight, trackColor);
    
    ImGui::Spacing();
    
    float db = track.volume > 0.0f ? 20.0f * log10f(track.volume) : m_config.minDecibel;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat("##trackDB", &db, 0.1f, m_config.minDecibel, 0.0f, "%.1f dB"))
    {
        track.volume = (db <= m_config.minDecibel) ? 0.0f : powf(10.0f, db / 20.0f);
        track.volume = std::clamp(track.volume, 0.0f, 1.0f);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImVec4 soloColor = track.solo ? 
        ImVec4(1.0f, 0.7f, 0.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    ImVec4 soloHover = track.solo ? 
        ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    
    ImGui::PushStyleColor(ImGuiCol_Button, soloColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, soloHover);
    
    if (ImGui::Button("S", ImVec2(m_config.buttonSize, m_config.buttonSize)))
    {
        track.solo = !track.solo;
        if (track.solo)
        {
            ProjectState& state = ProjectState::GetInstance();
            for (auto& t : state.tracks)
                t.solo = false;
            track.solo = true;
        }
    }
    ImGui::PopStyleColor(2);
    
    ImGui::SameLine();
    
    ImVec4 muteColor = track.muted ? 
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    ImVec4 muteHover = track.muted ? 
        ImVec4(1.0f, 0.6f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    
    ImGui::PushStyleColor(ImGuiCol_Button, muteColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, muteHover);
    
    if (ImGui::Button("M", ImVec2(m_config.buttonSize, m_config.buttonSize)))
    {
        track.muted = !track.muted;
    }
    ImGui::PopStyleColor(2);

    ImGui::EndChild();
    ImGui::PopID();
}

void MixerRack::DrawVUMeterWithFader(float vuLevelLeft, float vuLevelRight, float& volume, 
                             float width, float height, ImU32 channelColor)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    float meterWidth = width / 2.0f - 1.0f;
    
    draw->AddRectFilled(pos, ImVec2(pos.x + meterWidth, pos.y + height), 
        IM_COL32(30, 30, 30, 255));
    
    draw->AddRectFilled(ImVec2(pos.x + meterWidth + 2, pos.y), 
        ImVec2(pos.x + width, pos.y + height), 
        IM_COL32(30, 30, 30, 255));
    
    float segHeight = height / m_config.vuSegments;
    float segSpacing = 1.0f;
    
    for (int i = 0; i < m_config.vuSegments; i++)
    {
        float segLevel = static_cast<float>(i) / m_config.vuSegments;
        if (segLevel <= vuLevelLeft)
        {
            ImU32 color;
            if (segLevel > m_config.vuRedThreshold)
                color = IM_COL32(255, 50, 50, 255);
            else if (segLevel > m_config.vuYellowThreshold)
                color = IM_COL32(255, 200, 50, 255);
            else
                color = IM_COL32(50, 255, 100, 255);
            
            float y = pos.y + height - (i + 1) * segHeight;
            draw->AddRectFilled(
                ImVec2(pos.x + 1, y + segSpacing),
                ImVec2(pos.x + meterWidth - 1, y + segHeight - segSpacing),
                color
            );
        }
    }
    
    for (int i = 0; i < m_config.vuSegments; i++)
    {
        float segLevel = static_cast<float>(i) / m_config.vuSegments;
        if (segLevel <= vuLevelRight)
        {
            ImU32 color;
            if (segLevel > m_config.vuRedThreshold)
                color = IM_COL32(255, 50, 50, 255);
            else if (segLevel > m_config.vuYellowThreshold)
                color = IM_COL32(255, 200, 50, 255);
            else
                color = IM_COL32(50, 255, 100, 255);
            
            float y = pos.y + height - (i + 1) * segHeight;
            draw->AddRectFilled(
                ImVec2(pos.x + meterWidth + 3, y + segSpacing),
                ImVec2(pos.x + width - 1, y + segHeight - segSpacing),
                color
            );
        }
    }
    
    float volumeY = pos.y + height - (volume * height);
    
    ImVec2 p1(pos.x + width + 2, volumeY);
    ImVec2 p2(pos.x + width + 10, volumeY - 5);
    ImVec2 p3(pos.x + width + 10, volumeY + 5);
    draw->AddTriangleFilled(p1, p2, p3, channelColor);
    
    ImGui::InvisibleButton("##vumeter", ImVec2(width + 12, height));
    
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 0.0f))
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        float newVolume = 1.0f - ((mousePos.y - pos.y) / height);
        volume = std::clamp(newVolume, 0.0f, 1.0f);
    }
    
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
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
                if (entry.is_regular_file() && entry.path().extension() == ".dll")
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