#pragma once

#include "Core/ProjectState.h"
#include "Config/EditorConfig.h"
#include "Audio/AudioEngine.h"
#include "Models/EditorModel/GeneralSettings.h"
#include "Models/EditorModel/TimelineSettings.h"

#include <imgui.h>
#include <vector>
#include <utility>

class SongTimeline : public Naui::Panel
{
public:
    SongTimeline();
	static AudioTrack& CreateTrack(std::string title, ImVec4 color = ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
	static void NormalizeTracks();
	static bool MuteTrack(size_t index, bool mute);
	static bool SoloTrack(size_t index, bool solo);
	static bool RecordTrack(size_t index, bool record);

	static size_t GetTrackIndex(AudioTrack& track);
	static AudioTrack& GetTrackAtIndex(size_t index);
	static double GetPlayheadPositionNormalized();
	static double GetTrackDuration(size_t trackIndex);
	static double GetTimelineDuration();
	static bool RenameTrack(size_t index, const std::string& newName);
	
protected:
	void OnRegisterShortcuts(Naui::ShortcutTable& table) override;
    void OnRender() override;

private:
    struct TrackUIState
    {
        float height;
        bool collapsed;
    };
    
    enum class Tool { Select, Draw, Cut, Delete };
    
    struct InstanceSelection
    {
        bool isDragging;
        bool isResizing;
        bool isResizingLeft;
        bool isSelecting;
        int draggedTrack;
        int draggedInstance;
        ImVec2 dragStartPos;
        ImVec2 selectionStart;
        ImVec2 selectionEnd;
        
        struct OriginalPosition
        {
            double startBeat;
            int trackIndex;
        };
        std::vector<OriginalPosition> originalPositions;
        std::vector<std::pair<int, int>> selectedInstances;
    };

    void EnsureUIStates();
    void RenderToolbar();
    void RenderTimeline();
    void RenderRuler(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth);
    void RenderPlayhead(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth);
    void RenderTrack(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, size_t trackIndex, float yPos);
    void RenderTrackHeader(ImDrawList* draw, ImVec2 canvasPos, size_t trackIndex, float yPos);
    void RenderTrackContextMenu(size_t trackIndex);
    void RenderMidiInstance(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, size_t trackIndex, size_t instanceIndex, float yPos, float trackStartX, TrackUIState& ui);
    void RenderSampleInstance(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, size_t trackIndex, size_t instanceIndex, float yPos, float trackStartX, TrackUIState& ui);
    void HandleInput(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth);
    void ResetDragState();
    bool HandleRulerClick(ImVec2 mousePos, ImVec2 canvasPos, ImVec2 canvasSize, float trackStartX, float beatWidth, bool& isScrubbing);
    bool IsInTimelineArea(ImVec2 mousePos, ImVec2 canvasPos, ImVec2 canvasSize, float trackStartX, float timelineStartY);
    void UpdateCursor(ImVec2 mousePos, double beatPos, int trackIdx, float trackStartX, float beatWidth);
    bool HandleKeyboardShortcuts();
    void HandleMouseClick(ImVec2 mousePos, double beatPos, int trackIdx, float trackStartX, float beatWidth, ImGuiMouseButton btn);
    void HandleDrawToolClick(int instTrack, int instIdx, int trackIdx, double beatPos, ImVec2 mousePos, float trackStartX, float beatWidth);
    void HandleSelectToolClick(int instTrack, int instIdx, int trackIdx, ImVec2 mousePos, float trackStartX, float beatWidth);
    void HandleDeleteToolClick(int instTrack, int instIdx);
    bool TryStartResize(int trackIdx, int instIdx, ImVec2 mousePos, float trackStartX, float beatWidth);
    void StartDragging(int trackIdx, int instIdx, ImVec2 mousePos);
    void HandleMouseDrag(ImVec2 mousePos, double beatPos, int trackIdx, float timelineStartY, float trackStartX, float beatWidth);
    void HandleRightResize(double beatPos);
    void HandleLeftResize(double beatPos);
    void HandleInstanceDrag(ImVec2 mousePos, int hoverTrack, float timelineStartY, float beatWidth);
    void HandleSingleInstanceDrag(int hoverTrack, double deltaBeat, float timelineStartY);
    void HandleMultiInstanceDrag(int hoverTrack, double deltaBeat, float timelineStartY);
    void TryMoveSelectionToTrack(int hoverTrack);
    void HandleBoxSelection(ImVec2 mousePos, float timelineStartY, float trackStartX, float beatWidth);
    void SelectAllInstances();
    void CreateInstance(int trackIdx, double beatPos);
    std::pair<int, int> FindInstanceAt(double beatPos, int trackIdx);
    int GetTrackAtY(float mouseY, float timelineStartY);
    double SnapToGrid(double value);
    void StoreOriginalInstancePositions();
    void ClearSelection();
    bool IsInstanceSelected(int trackIdx, int instanceIdx);
    void DeleteSelectedInstances();
    void DeleteTrack(size_t trackIndex);
    const char* GetSnapName(int snap);
    
    TimelineConfig m_config;
    std::vector<TrackUIState> m_trackUIStates;
    InstanceSelection m_selection;
    Tool m_currentTool;
    float m_zoom;
    float m_scrollX;
    float m_scrollY;
    int m_snap;
};

