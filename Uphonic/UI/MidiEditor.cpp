#include "MidiEditor.h"
#include "../Core/ProjectState.h"
#include "../Config/EditorConfig.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

MidiEditor::MidiEditor() : Naui::Panel("Midi Editor")
{
    m_zoom = m_config.defaultZoom;
    m_scrollX = 0.0f;
    m_scrollY = m_config.pianoWidth;
    m_snap = m_config.defaultSnap;
    m_verticalZoom = m_config.defaultVerticalZoom;
    m_lastNoteLength = m_config.defaultNoteLength;
    m_currentTool = Tool::Draw;
    m_selection.isDragging = false;
    m_selection.isResizing = false;
    m_selection.isSelecting = false;
    m_selection.draggedNoteIndex = -1;
}

void MidiEditor::OnRender()
{
    ProjectState& state = ProjectState::GetInstance();
    if (state.patterns.empty())
    {
        ImGui::Text("No patterns found");
        return;
    }
    RenderToolbar();
    ImGui::Separator();
    
    ImGui::BeginChild("##PianoRoll", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
    RenderPianoRoll();
    ImGui::EndChild();
	HandlePatternDrop();
}

void MidiEditor::RenderToolbar()
{
    ProjectState& state = ProjectState::GetInstance();

    if (ImGui::RadioButton("Select", m_currentTool == Tool::Select))
    {
        m_currentTool = Tool::Select;
		ClearSelection();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Draw", m_currentTool == Tool::Draw))
    {
        m_currentTool = Tool::Draw;
		ClearSelection();
    }
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    ImGui::Text("Snap:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    const char* snapPreview = GetSnapName(m_snap);
    if (ImGui::BeginCombo("##Snap", snapPreview, ImGuiComboFlags_NoArrowButton))
    {
        if (ImGui::Selectable("1/4", m_snap == 4)) m_snap = 4;
        if (ImGui::Selectable("1/8", m_snap == 8)) m_snap = 8;
        if (ImGui::Selectable("1/16", m_snap == 16)) m_snap = 16;
        if (ImGui::Selectable("1/32", m_snap == 32)) m_snap = 32;
        ImGui::EndCombo();
    }
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    ImGui::Text("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::SliderFloat("##Zoom", &m_zoom, m_config.minZoom, m_config.maxZoom, "%.2fx");
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    if (m_currentTool == Tool::Select && m_selection.selectedNoteIndices.size() > 0 && ImGui::Button("Delete Selected"))
    {
        DeleteSelectedNotes();
    }
    ImGui::SameLine();
    if (ImGui::Button("Select All"))
    {
        SelectAllNotes();
    }
}

void MidiEditor::RenderPianoRoll()
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float beatWidth = m_config.defaultBeatWidth * m_zoom;
    float noteHeight = m_config.defaultNoteHeight * m_verticalZoom;
    
    if (ImGui::IsWindowHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (ImGui::GetIO().KeyCtrl && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            m_verticalZoom += wheel * m_config.verticalZoomSensitivity;
            m_verticalZoom = std::clamp(m_verticalZoom, m_config.minVerticalZoom, m_config.maxVerticalZoom);
        }
        else if (ImGui::GetIO().KeyCtrl)
        {
            m_zoom += wheel * m_config.zoomSensitivity;
            m_zoom = std::clamp(m_zoom, m_config.minZoom, m_config.maxZoom);
        }
        else if (ImGui::GetIO().KeyShift)
        {
            m_scrollX -= wheel * m_config.scrollSensitivity;
        }
        else
        {
            m_scrollY -= wheel * m_config.verticalScrollSensitivity;
        }
        m_scrollY = std::clamp(m_scrollY, 0.0f, (float)(m_config.totalKeys - 20));
    }
    
    draw->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);
    draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(30, 30, 35, 255));
    
    RenderPianoKeys(draw, canvasPos, canvasSize, noteHeight);
    RenderGrid(draw, canvasPos, canvasSize, beatWidth, noteHeight);
    RenderNotes(draw, canvasPos, canvasSize, beatWidth, noteHeight);
    
    if (m_selection.isSelecting)
    {
        ImVec2 p1 = m_selection.selectionStart;
        ImVec2 p2 = ImGui::GetMousePos();
        draw->AddRect(p1, p2, IM_COL32(100, 150, 255, 200), 0.0f, 0, 2.0f);
        draw->AddRectFilled(p1, p2, IM_COL32(100, 150, 255, 30));
    }
    
    HandleInput(canvasPos, canvasSize, beatWidth, noteHeight);
    
    draw->PopClipRect();
}

void MidiEditor::RenderPianoKeys(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight)
{
    int startKey = (int)m_scrollY;
    int endKey = std::min((int)(m_scrollY + canvasSize.y / noteHeight) + 1, m_config.totalKeys);
    
    for (int i = startKey; i < endKey; i++)
    {
        int note = m_config.totalKeys - 1 - i;
        float y = canvasPos.y + (i - m_scrollY) * noteHeight;
        
        if (!IsBlackKey(note % 12))
        {
            draw->AddRectFilled(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + m_config.pianoWidth, y + noteHeight), IM_COL32(45, 45, 48, 255));
            draw->AddRect(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + m_config.pianoWidth, y + noteHeight), IM_COL32(25, 25, 28, 255));
        }
    }
    
    float blackKeyWidth = m_config.pianoWidth * 0.6f;
    for (int i = startKey; i < endKey; i++)
    {
        int note = m_config.totalKeys - 1 - i;
        float y = canvasPos.y + (i - m_scrollY) * noteHeight;
        
        if (IsBlackKey(note % 12))
        {
            draw->AddRectFilled(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + blackKeyWidth, y + noteHeight), IM_COL32(20, 20, 22, 255));
            draw->AddRect(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + blackKeyWidth, y + noteHeight), IM_COL32(10, 10, 12, 255));
        }
    }
    
    for (int i = startKey; i < endKey; i++)
    {
        int note = m_config.totalKeys - 1 - i;
        float y = canvasPos.y + (i - m_scrollY) * noteHeight;
        
        if (note % 12 == 0)
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "C%d", (note / 12) - 1);
            draw->AddText(ImVec2(canvasPos.x + blackKeyWidth + 3, y + 2), IM_COL32(120, 120, 125, 255), buf);
        }
    }
    
    draw->AddLine(ImVec2(canvasPos.x + m_config.pianoWidth, canvasPos.y), ImVec2(canvasPos.x + m_config.pianoWidth, canvasPos.y + canvasSize.y), IM_COL32(60, 60, 65, 255), 2.0f);
}

void MidiEditor::RenderGrid(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight)
{
    float gridStartX = canvasPos.x + m_config.pianoWidth;
    float gridWidth = canvasSize.x - m_config.pianoWidth;
    
    int startKey = (int)m_scrollY;
    int endKey = std::min((int)(m_scrollY + canvasSize.y / noteHeight) + 1, m_config.totalKeys);
    
    for (int i = startKey; i < endKey; i++)
    {
        int note = m_config.totalKeys - 1 - i;
        float y = canvasPos.y + (i - m_scrollY) * noteHeight;
        
        bool isBlack = IsBlackKey(note % 12);
        ImU32 bgCol = isBlack ? IM_COL32(26, 26, 30, 255) : IM_COL32(30, 30, 35, 255);
        
        draw->AddRectFilled(ImVec2(gridStartX, y), ImVec2(canvasPos.x + canvasSize.x, y + noteHeight), bgCol);
    }
    
    for (int i = startKey; i < endKey; i++)
    {
        int note = m_config.totalKeys - 1 - i;
        float y = canvasPos.y + (i - m_scrollY) * noteHeight;
        
        bool isBlack = IsBlackKey(note % 12);
        ImU32 lineCol = isBlack ? IM_COL32(35, 35, 38, 255) : IM_COL32(38, 38, 42, 255);
        
        draw->AddLine(ImVec2(gridStartX, y), ImVec2(canvasPos.x + canvasSize.x, y), lineCol);
    }
    
    int visibleBeats = (int)((gridWidth + m_scrollX) / beatWidth) + 2;
    int startBeat = (int)(m_scrollX / beatWidth);
    
    for (int i = 0; i < visibleBeats; i++)
    {
        int beat = startBeat + i;
        float x = gridStartX + beat * beatWidth - m_scrollX;
        
        if (x < gridStartX) continue;
        
        ImU32 lineCol = (beat % 4 == 0) ? IM_COL32(55, 55, 60, 255) : IM_COL32(42, 42, 46, 255);
        float thickness = (beat % 4 == 0) ? 1.5f : 1.0f;
        
        draw->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasSize.y), lineCol, thickness);
    }
}

void MidiEditor::RenderNotes(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight)
{
    float gridStartX = canvasPos.x + m_config.pianoWidth;
    MidiPattern& pattern = GetCurrentPattern();
    
    ImVec4 buttonCol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    ImVec4 buttonActiveCol = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
    ImVec4 buttonHoveredCol = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
    
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    for (size_t i = 0; i < pattern.notes.size(); i++)
    {
        const auto& note = pattern.notes[i];
        bool selected = std::find(m_selection.selectedNoteIndices.begin(), m_selection.selectedNoteIndices.end(), i) != m_selection.selectedNoteIndices.end();
        
        float noteY = canvasPos.y + (m_config.totalKeys - 1 - note.keyNumber - m_scrollY) * noteHeight;
        float noteX = gridStartX + (float)note.startBeat * beatWidth - m_scrollX;
        float noteW = (float)note.lengthBeats * beatWidth;
        
        if (noteX + noteW < gridStartX || noteX > canvasPos.x + canvasSize.x) continue;
        if (noteY + noteHeight < canvasPos.y || noteY > canvasPos.y + canvasSize.y) continue;
        
        float drawX = std::max(noteX, gridStartX);
        float drawW = std::min(noteX + noteW, canvasPos.x + canvasSize.x) - drawX;
        
        if (drawW <= 0) continue;
        
        ImVec4 col = selected ? buttonActiveCol : buttonCol;
        ImU32 noteCol = ImGui::ColorConvertFloat4ToU32(col);
        ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(buttonHoveredCol);
        
        ImVec2 p1(drawX, noteY + 1);
        ImVec2 p2(drawX + drawW, noteY + noteHeight - 1);
        
        draw->AddRectFilled(p1, p2, noteCol, 2.0f);
        draw->AddRect(p1, p2, borderCol, 2.0f, 0, 1.5f);
        
        if (noteW > 25 && noteHeight >= 12.0f)
        {
            int noteInOctave = note.keyNumber % 12;
            int octave = (note.keyNumber / 12) - 1;
            char buf[8];
            snprintf(buf, sizeof(buf), "%s%d", noteNames[noteInOctave], octave);
            
            ImVec2 textSize = ImGui::CalcTextSize(buf);
            float textX = std::max(drawX + 4, noteX + 4);
            float textY = noteY + (noteHeight - textSize.y) * 0.5f;
            
            if (textX + textSize.x < drawX + drawW)
            {
                draw->AddText(ImVec2(textX, textY), IM_COL32(200, 200, 205, 255), buf);
            }
        }
        
        if (selected && noteX + noteW >= gridStartX && noteHeight >= 8.0f)
        {
            float handleX = std::max(noteX + noteW - 4, gridStartX);
            ImVec2 handleP1(handleX, noteY + 3);
            ImVec2 handleP2(noteX + noteW - 1, noteY + noteHeight - 3);
            ImU32 handleCol = ImGui::ColorConvertFloat4ToU32(buttonHoveredCol);
            draw->AddRectFilled(handleP1, handleP2, handleCol);
        }
    }
}

void MidiEditor::HandleInput(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight)
{
    if (ImGui::IsMouseReleased(0))
    {
        m_selection.isDragging = false;
        m_selection.isResizing = false;
        m_selection.isSelecting = false;
        m_selection.draggedNoteIndex = -1;
        m_selection.originalPositions.clear();
    }
    
    if (!ImGui::IsWindowHovered() && !m_selection.isDragging && !m_selection.isResizing && !m_selection.isSelecting)
        return;

    ImVec2 mousePos = ImGui::GetMousePos();
    float gridStartX = canvasPos.x + m_config.pianoWidth;
    
    bool inGrid = mousePos.x >= gridStartX && 
                mousePos.x < canvasPos.x + canvasSize.x &&
                mousePos.y >= canvasPos.y && 
                mousePos.y < canvasPos.y + canvasSize.y;
    
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        
        if (ImGui::GetIO().KeyCtrl)
        {
            m_verticalZoom += delta.y * -0.01f;
            m_verticalZoom = std::clamp(m_verticalZoom, m_config.minVerticalZoom, m_config.maxVerticalZoom);
        }
        else
        {
            m_scrollX -= delta.x;
            m_scrollY -= delta.y / noteHeight;
        }
        
        m_scrollX = std::max(0.0f, m_scrollX);
        m_scrollY = std::clamp(m_scrollY, 0.0f, (float)(m_config.totalKeys - 20));
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        return;
    }
    
    float relativeX = mousePos.x - gridStartX + m_scrollX;
    float relativeY = mousePos.y - canvasPos.y;
    
    double beatPos = relativeX / beatWidth;
    int noteNum = m_config.totalKeys - 1 - (int)((relativeY + m_scrollY * noteHeight) / noteHeight);
    
    UpdateCursor(canvasPos, canvasSize, beatWidth, noteHeight, beatPos, noteNum, inGrid);
    
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inGrid)
    {
        if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            return;
        
        int clicked = FindNoteAt(beatPos, noteNum);
        
        if (m_currentTool == Tool::Draw)
        {
            if (clicked == -1)
            {
                ClearSelection();
                CreateNote(beatPos, noteNum);
                m_selection.selectedNoteIndices.push_back((int)(GetCurrentPattern().notes.size() - 1));
                m_selection.isDragging = true;
                m_selection.draggedNoteIndex = m_selection.selectedNoteIndices[0];
                m_selection.dragStartPosition = mousePos;
                StoreOriginalPositions();
            }
            else
            {
                MidiPattern& pattern = GetCurrentPattern();
                float noteEnd = (float)(pattern.notes[clicked].startBeat + pattern.notes[clicked].lengthBeats);
                float endX = gridStartX + noteEnd * beatWidth - m_scrollX;
                
                if (mousePos.x > endX - 6 && mousePos.x < endX + 2)
                {
                    m_selection.isResizing = true;
                    m_selection.draggedNoteIndex = clicked;
                }
                else
                {
                    if (!ImGui::GetIO().KeyCtrl)
                    {
                        ClearSelection();
                    }
                    m_selection.selectedNoteIndices.push_back(clicked);
                    m_selection.isDragging = true;
                    m_selection.draggedNoteIndex = clicked;
                    m_selection.dragStartPosition = mousePos;
                    StoreOriginalPositions();
                }
            }
        }
        else if (m_currentTool == Tool::Select)
        {
            if (clicked != -1)
            {
                MidiPattern& pattern = GetCurrentPattern();
                float noteEnd = (float)(pattern.notes[clicked].startBeat + pattern.notes[clicked].lengthBeats);
                float endX = gridStartX + noteEnd * beatWidth - m_scrollX;
                
                if (mousePos.x > endX - 6 && mousePos.x < endX + 2)
                {
                    m_selection.isResizing = true;
                    m_selection.draggedNoteIndex = clicked;
                }
                else
                {
                    bool alreadySelected = std::find(m_selection.selectedNoteIndices.begin(), m_selection.selectedNoteIndices.end(), clicked) != m_selection.selectedNoteIndices.end();
                    if (!ImGui::GetIO().KeyCtrl && !alreadySelected)
                    {
                        ClearSelection();
                    }
                    if (!alreadySelected)
                    {
                        m_selection.selectedNoteIndices.push_back(clicked);
                    }
                    m_selection.isDragging = true;
                    m_selection.draggedNoteIndex = clicked;
                    m_selection.dragStartPosition = mousePos;
                    StoreOriginalPositions();
                }
            }
            else
            {
                if (!ImGui::GetIO().KeyCtrl)
                {
                    ClearSelection();
                }
                m_selection.isSelecting = true;
                m_selection.selectionStart = mousePos;
            }
        }
    } else if(ImGui::IsMouseDown(ImGuiMouseButton_Right) && inGrid)
	{
		if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
			return;

		if(m_currentTool == Tool::Select)
			ClearSelection();
		else
		{
			int noteIndex = FindNoteAt(beatPos, noteNum);
			if(noteIndex != -1)
			{
				MidiPattern& pattern = GetCurrentPattern();
				pattern.notes.erase(pattern.notes.begin() + noteIndex);
			}
		}
	}
    
    if (m_selection.isDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        MidiPattern& pattern = GetCurrentPattern();
        ImVec2 mouseDelta = ImVec2(mousePos.x - m_selection.dragStartPosition.x, mousePos.y - m_selection.dragStartPosition.y);
        double deltaBeat = mouseDelta.x / beatWidth;
        int deltaPitch = -(int)(mouseDelta.y / noteHeight);
        
        double minOriginalStart = DBL_MAX;
        for (int idx : m_selection.selectedNoteIndices)
        {
            if (idx < (int)m_selection.originalPositions.size())
            {
                minOriginalStart = std::min(minOriginalStart, m_selection.originalPositions[idx].startBeat);
            }
        }
        
        if (minOriginalStart + deltaBeat < 0.0)
        {
            deltaBeat = -minOriginalStart;
        }
        
        for (int idx : m_selection.selectedNoteIndices)
        {
            if (idx >= 0 && idx < (int)pattern.notes.size() && idx < (int)m_selection.originalPositions.size())
            {
                double newStart = m_selection.originalPositions[idx].startBeat + deltaBeat;
                pattern.notes[idx].startBeat = SnapToGrid(newStart);
                pattern.notes[idx].keyNumber = std::clamp(m_selection.originalPositions[idx].pitch + deltaPitch, 0, 127);
            }
        }
    }
    
    if (m_selection.isResizing && ImGui::IsMouseDragging(0))
    {
        MidiPattern& pattern = GetCurrentPattern();
        if (m_selection.draggedNoteIndex >= 0 && m_selection.draggedNoteIndex < (int)pattern.notes.size())
        {
            double newLength = beatPos - pattern.notes[m_selection.draggedNoteIndex].startBeat;
            double snappedLength = SnapToGrid(newLength);
            
            double minLength = 4.0 / 32.0;
            snappedLength = std::max(minLength, snappedLength);
            
            pattern.notes[m_selection.draggedNoteIndex].lengthBeats = snappedLength;
            m_lastNoteLength = (float)snappedLength;
        }
    }
    
    if (m_selection.isSelecting && ImGui::IsMouseDragging(0))
    {
        m_selection.selectionEnd = mousePos;
        
        MidiPattern& pattern = GetCurrentPattern();
        ImVec2 boxMin(std::min(m_selection.selectionStart.x, m_selection.selectionEnd.x), std::min(m_selection.selectionStart.y, m_selection.selectionEnd.y));
        ImVec2 boxMax(std::max(m_selection.selectionStart.x, m_selection.selectionEnd.x), std::max(m_selection.selectionStart.y, m_selection.selectionEnd.y));
        
        for (int i = 0; i < pattern.notes.size(); i++)
        {
            const auto& note = pattern.notes[i];
            float noteX1 = gridStartX + (float)note.startBeat * beatWidth - m_scrollX;
            float noteX2 = noteX1 + (float)note.lengthBeats * beatWidth;
            float noteY = canvasPos.y + (m_config.totalKeys - 1 - note.keyNumber - m_scrollY) * noteHeight;
            
            bool inBox = noteX2 >= boxMin.x && noteX1 <= boxMax.x &&
                        noteY + noteHeight >= boxMin.y && noteY <= boxMax.y;
            
            if (inBox)
            {
                if (std::find(m_selection.selectedNoteIndices.begin(), m_selection.selectedNoteIndices.end(), i) == m_selection.selectedNoteIndices.end())
                {
                    m_selection.selectedNoteIndices.push_back(i);
                }
            }
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        DeleteSelectedNotes();
    }
    
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
    {
        SelectAllNotes();
    }
}

void MidiEditor::HandlePatternDrop(void)
{
	ProjectState& state = ProjectState::GetInstance();
	if (ImGui::GetDragDropPayload())
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("PATTERN_INDEX"))
			{
				state.currentMidiPatternIndex = *(const uint16_t*)payload->Data;
			}
			ImGui::EndDragDropTarget();
		}
	}
}

void MidiEditor::UpdateCursor(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight, double beatPos, int noteNum, bool inGrid)
{
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

    if (m_selection.isDragging)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        return;
    }
    
    if (m_selection.isResizing)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        return;
    }
    
    if (m_selection.isSelecting)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        return;
    }
    
    if (!inGrid)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        return;
    }
    
    ImVec2 mousePos = ImGui::GetMousePos();
    float gridStartX = canvasPos.x + m_config.pianoWidth;
    int hoveredNote = FindNoteAt(beatPos, noteNum);
    
    if (hoveredNote != -1)
    {
        MidiPattern& pattern = GetCurrentPattern();
        float noteEnd = (float)(pattern.notes[hoveredNote].startBeat + pattern.notes[hoveredNote].lengthBeats);
        float endX = gridStartX + noteEnd * beatWidth - m_scrollX;
        
        if (mousePos.x > endX - 6 && mousePos.x < endX + 2)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
    }
}

void MidiEditor::CreateNote(double beatPos, int noteNum)
{
    double snappedStart = SnapToGrid(beatPos);
    
    MidiNote note;
    note.keyNumber = std::clamp(noteNum, 0, 127);
    note.startBeat = snappedStart;
    note.lengthBeats = m_lastNoteLength;
    note.velocity = 100;
    
    GetCurrentPattern().notes.push_back(note);
}

int MidiEditor::FindNoteAt(double beatPos, int noteNum)
{
    MidiPattern& pattern = GetCurrentPattern();
    for (int i = (int)pattern.notes.size() - 1; i >= 0; i--)
    {
        const auto& note = pattern.notes[i];
        if (note.keyNumber == noteNum &&
            beatPos >= note.startBeat && 
            beatPos <= note.startBeat + note.lengthBeats)
        {
            return i;
        }
    }
    return -1;
}

double MidiEditor::SnapToGrid(double value)
{
    double snapSize = 4.0 / m_snap;
    return std::round(value / snapSize) * snapSize;
}

void MidiEditor::StoreOriginalPositions()
{
    m_selection.originalPositions.clear();
    MidiPattern& pattern = GetCurrentPattern();
    m_selection.originalPositions.resize(pattern.notes.size());
    for (size_t i = 0; i < pattern.notes.size(); i++)
    {
        m_selection.originalPositions[i] = {pattern.notes[i].startBeat, (int)pattern.notes[i].keyNumber};
    }
}

void MidiEditor::ClearSelection()
{
    m_selection.selectedNoteIndices.clear();
}

void MidiEditor::SelectAllNotes()
{
    MidiPattern& pattern = GetCurrentPattern();
    m_selection.selectedNoteIndices.clear();
    for (int i = 0; i < pattern.notes.size(); i++)
    {
        m_selection.selectedNoteIndices.push_back(i);
    }
}

void MidiEditor::DeleteSelectedNotes()
{
    MidiPattern& pattern = GetCurrentPattern();
    std::sort(m_selection.selectedNoteIndices.rbegin(), m_selection.selectedNoteIndices.rend());
    for (int idx : m_selection.selectedNoteIndices)
    {
        if (idx >= 0 && idx < (int)pattern.notes.size())
        {
            pattern.notes.erase(pattern.notes.begin() + idx);
        }
    }
    m_selection.selectedNoteIndices.clear();
}

bool MidiEditor::IsBlackKey(int noteInOctave)
{
    return noteInOctave == 1 || noteInOctave == 3 || 
           noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10;
}

const char* MidiEditor::GetSnapName(int snap)
{
    switch (snap)
    {
        case 4: return "1/4";
        case 8: return "1/8";
        case 16: return "1/16";
        case 32: return "1/32";
        default: return "Unknown";
    }
}

MidiPattern& MidiEditor::GetCurrentPattern()
{
    ProjectState& state = ProjectState::GetInstance();
    if (state.currentMidiPatternIndex >= state.patterns.size())
    {
        state.currentMidiPatternIndex = 0;
    }
    return state.patterns[state.currentMidiPatternIndex];
}

