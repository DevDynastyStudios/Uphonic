#pragma once

#include "Naui.h"
#include "../Core/ProjectState.h"
#include "../Config/EditorConfig.h"
#include <vector>

class MidiEditor : public Naui::Panel
{
public:
    MidiEditor();

protected:
    void OnRender() override;

private:
    enum class Tool { Select, Draw, Erase };
    
    struct NoteSelection
    {
        std::vector<int> selectedNoteIndices;
        bool isDragging;
        bool isResizing;
        bool isSelecting;
        int draggedNoteIndex;
        ImVec2 dragStartPosition;
        ImVec2 selectionStart;
        ImVec2 selectionEnd;
        
        struct OriginalPosition
        {
            double startBeat;
            int pitch;
        };
        std::vector<OriginalPosition> originalPositions;
    };
    
    void RenderToolbar();
    void RenderPianoRoll();
    void RenderPianoKeys(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight);
    void RenderGrid(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight);
    void RenderNotes(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight);
    void HandleInput(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight);
	void HandlePatternDrop(void);
    void UpdateCursor(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight, double beatPos, int noteNum, bool inGrid);
    void CreateNote(double beatPos, int noteNum);
    int FindNoteAt(double beatPos, int noteNum);
    double SnapToGrid(double value);
    void StoreOriginalPositions();
    void ClearSelection();
    void SelectAllNotes();
    void DeleteSelectedNotes();
    bool IsBlackKey(int noteInOctave);
    const char* GetSnapName(int snap);
    MidiPattern& GetCurrentPattern();
    
    MidiEditorConfig m_config;
    NoteSelection m_selection;
    Tool m_currentTool;
    float m_zoom;
    float m_scrollX;
    float m_scrollY;
    int m_snap;
    float m_verticalZoom;
    float m_lastNoteLength;
};

