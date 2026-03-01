#include "PatternRack.h"
#include "Core/ProjectState.h"
#include "DataModel/Patterns.h"
#include <algorithm>
#include <cstring>

PatternRack::PatternRack() : Naui::Panel(Naui::TR("pattern_rack.title"))
{
    m_renamingIndex = -1;
    memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
}

size_t PatternRack::GetPatternIndex(MidiPattern& pattern)
{
	return &pattern - ProjectState::GetInstance().patterns.data();
}

MidiPattern& PatternRack::GetPatternAtIndex(size_t index)
{
	ProjectState& state = ProjectState::GetInstance();
	if(index >= state.patterns.size())
		throw std::out_of_range("Sample index out of range");

	return state.patterns[index];
}

bool PatternRack::RenamePattern(size_t index, const std::string& newName)
{
	ProjectState& state = ProjectState::GetInstance();
	if(index >= state.samples.size())
		return false;

	MidiPattern& pattern = state.patterns[index];
	pattern.name = newName;
	return true;
}

void PatternRack::OnRender()
{
    ProjectState& state = ProjectState::GetInstance();

	if (ImGui::Button(Naui::TR("pattern_rack.new")))
    {
        ProjectState& state = ProjectState::GetInstance();
        MidiPattern pattern;
        pattern.name = "Pattern " + std::to_string(state.patterns.size() + 1);
        state.patterns.push_back(pattern);
    }

    for (uint16_t i = 0; i < state.patterns.size(); i++)
    {
        MidiPattern& pattern = state.patterns[i];
        
        ImGui::PushID(i);
        
        if (m_renamingIndex == (int)i)
        {
            ImGui::SetNextItemWidth(-60.0f);
            if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                pattern.name = m_renameBuffer;
                m_renamingIndex = -1;
            }
            
            if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive())
            {
                m_renamingIndex = -1;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("OK", ImVec2(50, 0)))
            {
                pattern.name = m_renameBuffer;
                m_renamingIndex = -1;
            }
        }
        else
        {
            if (ImGui::Selectable(pattern.name.c_str()))
            {
                state.currentMidiPatternIndex = i;
            }
            
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("PATTERN_INDEX", &i, sizeof(uint16_t));
                ImGui::Text("%s", pattern.name.c_str());
                ImGui::EndDragDropSource();
            }
            
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                {
                    m_renamingIndex = i;
                    strncpy(m_renameBuffer, pattern.name.c_str(), sizeof(m_renameBuffer) - 1);
                    m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
                }
                
                if (ImGui::MenuItem("Delete", nullptr, false, state.patterns.size() > 1))
                {
                    DeletePattern(i);
                    ImGui::EndPopup();
                    ImGui::PopID();
                    break;
                }
                
                if (ImGui::MenuItem("Duplicate"))
                {
                    DuplicatePattern(i);
                }
                
                ImGui::EndPopup();
            }
        }
        
        ImGui::PopID();
    }
}

void PatternRack::DeletePattern(uint16_t index)
{
    ProjectState& state = ProjectState::GetInstance();
    if (state.patterns.size() <= 1)
    {
        return;
    }
    
    state.patterns.erase(state.patterns.begin() + index);
    
    for (auto& track : state.tracks)
    {
        if (track.type == TrackType::Midi)
        {
            track.blocks.erase(
                std::remove_if(track.blocks.begin(), track.blocks.end(),
                    [index](const TimelineBlock& block) {
                        return block.midiBlock.patternIndex == index;
                    }),
                track.blocks.end()
            );
            
            for (auto& block : track.blocks)
            {
                if (block.midiBlock.patternIndex > index)
                {
                    block.midiBlock.patternIndex--;
                }
            }
        }
    }
    
    if (state.currentMidiPatternIndex == index)
    {
        state.currentMidiPatternIndex = std::min(state.currentMidiPatternIndex, (uint16_t)(state.patterns.size() - 1));
    }
    else if (state.currentMidiPatternIndex > index)
    {
        state.currentMidiPatternIndex--;
    }
}

void PatternRack::DuplicatePattern(uint16_t index)
{
    ProjectState& state = ProjectState::GetInstance();
    if (index >= state.patterns.size()) return;
    
    MidiPattern newPattern = state.patterns[index];
    newPattern.name = state.patterns[index].name + " (Copy)";
    state.patterns.push_back(newPattern);
}

