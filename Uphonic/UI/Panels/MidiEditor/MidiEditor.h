#pragma once
#include "Naui.h"
#include "Core/ProjectState.h"
#include "Models/DataModel/Patterns.h"
#include "Config/EditorConfig.h"
#include "PianoKeyRenderer.h"
#include "GridRenderer.h"
#include "NoteRenderer.h"
#include "ToolbarRenderer.h"

#include <imgui.h>
#include <vector>

class MidiEditor : public Naui::Panel
{
public:
	MidiEditor();
	PianoKeyRenderer pianoKeys;

protected:
	void OnRegisterShortcuts(Naui::ShortcutTable& table) override;
	void OnRender() override;

private:
	enum class Tool : int { Select = 0, Draw = 1 };

	struct NoteSelection
	{
		std::vector<int> selectedNoteIndices;
		bool isDragging = false;
		bool isResizing = false;
		bool isSelecting = false;
		int draggedNoteIndex = -1;
		ImVec2 dragStartPosition{};
		ImVec2 selectionStart{};
		ImVec2 selectionEnd{};

		struct OriginalPosition { double startBeat; int pitch; };
		std::vector<OriginalPosition> originalPositions;
	};

	struct ActivePlayhead
	{
		double patternLocalBeat;
		ImVec4 trackColor;
	};

	void RenderToolbar();
	void RenderPianoRoll();
	void RenderPlayhead(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float pianoW);
	void HandlePatternDrop();
	void HandleInput(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight);
	void UpdateCursor(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight, double beatPos, int noteNum, bool inGrid);

	void CreateNote(double beatPos, int noteNum);
	int	FindNoteAt(double beatPos, int noteNum);
	double SnapToGrid(double value);
	void StoreOriginalPositions();
	void ClearSelection();
	void SelectAllNotes();
	void DeleteSelectedNotes();

	MidiPattern& GetCurrentPattern();

	float PianoWidth() const { return pianoKeys.layout.keyLength; }
	int PianoTopRow() const;
	int PianoBottomRow() const;
	void ClampScrollY(float noteHeight, float viewHeight);

	GridRenderer m_gridRenderer;
	NoteRenderer m_noteRenderer;
	ToolbarRenderer m_toolbarRenderer;

	TimeSignature m_timeSig;
	MidiEditorConfig m_config;
	NoteSelection m_selection;

	Tool m_currentTool	= Tool::Draw;
	float m_zoom = 1.0f;
	float m_scrollX = 0.0f;
	float m_scrollY = 48.0f;
	int m_snap = 4;
	float m_verticalZoom = 1.0f;
	float m_lastNoteLength = 0.25f;
	int m_previewNote = -1;

	std::set<int> m_playheadPressedKeys;
	bool m_showPlayheads = true;
};