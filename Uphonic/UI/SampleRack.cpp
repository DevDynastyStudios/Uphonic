#include "SampleRack.h"
#include "../Core/ProjectState.h"
#include "../Audio/AudioEngine.h"
#include <algorithm>
#include <cstring>

SampleRack::SampleRack() : Naui::Panel("Sample Rack")
{
    m_renamingIndex = -1;
    memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
}

void SampleRack::OnRender()
{
    ProjectState& state = ProjectState::GetInstance();
    for (uint16_t i = 0; i < state.samples.size(); i++)
    {
        AudioSample& sample = state.samples[i];

        ImGui::PushID(i);
        
        if (m_renamingIndex == (int)i)
        {
            ImGui::SetNextItemWidth(-60.0f);
            if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), 
                                ImGuiInputTextFlags_EnterReturnsTrue))
            {
                sample.name = m_renameBuffer;
                m_renamingIndex = -1;
            }
            
            if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive())
            {
                m_renamingIndex = -1;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("OK", ImVec2(50, 0)))
            {
                sample.name = m_renameBuffer;
                m_renamingIndex = -1;
            }
        }
        else
        {
            if (ImGui::Selectable(sample.name.c_str()))
            {
            }
            
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("SAMPLE_INDEX", &i, sizeof(uint16_t));
                ImGui::Text("%s", sample.name.c_str());
                ImGui::EndDragDropSource();
            }
            
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                {
                    m_renamingIndex = i;
                    strncpy(m_renameBuffer, sample.name.c_str(), sizeof(m_renameBuffer) - 1);
                    m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
                }
                
                if (ImGui::MenuItem("Delete"))
                {
                    DeleteSample(i);
                    ImGui::EndPopup();
                    ImGui::PopID();
                    break;
                }
                
                ImGui::Separator();
                
                ImGui::TextDisabled("Sample Rate: %d Hz", sample.sampleRate);
                ImGui::TextDisabled("Channels: %s", 
                                   sample.channelType == SampleChannelType::Mono ? "Mono" : "Stereo");
                ImGui::TextDisabled("Frames: %llu", sample.frameCount);
                
                ImGui::EndPopup();
            }
        }
        
        ImGui::PopID();
    }
}

void SampleRack::DeleteSample(uint16_t index)
{
    ProjectState& state = ProjectState::GetInstance();
    if (index >= state.samples.size()) return;
    
    AudioSample& sample = state.samples[index];
    if (sample.frameData)
        AudioEngine::UnloadSample(sample);
    
    state.samples.erase(state.samples.begin() + index);
    
    for (auto& track : state.tracks)
    {
        if (track.type == TrackType::Audio)
        {
            track.blocks.erase(
                std::remove_if(track.blocks.begin(), track.blocks.end(),
                    [index](const TimelineBlock& block) {
                        return block.sampleBlock.sampleIndex == index;
                    }),
                track.blocks.end()
            );
            
            for (auto& block : track.blocks)
            {
                if (block.sampleBlock.sampleIndex > index)
                {
                    block.sampleBlock.sampleIndex--;
                }
            }
        }
    }
}

