#include "MidiEditor.h"
#include "Core/ProjectState.h"
#include "Naui/Localization/Localization.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

MidiEditor::MidiEditor() : Naui::Panel(Naui::TR("midi_editor.title"))
{
	ProjectState& s = ProjectState::GetInstance();
	m_zoom = s.settings.generalSettings.defaultHorizontalZoom;
	m_snap = m_config.defaultSnap;
	m_verticalZoom = s.settings.generalSettings.defaultVerticalZoom;
	m_lastNoteLength = s.settings.midiSettings.defaultNoteLength;
}

void MidiEditor::OnRegisterShortcuts(Naui::ShortcutTable& table)
{
	table.Register("midi.delete",		 	{ ImGuiKey_Delete },			 	PRIORITY_PANEL, [this]{ DeleteSelectedNotes(); });
	table.Register("midi.play",		 		{ ImGuiKey_Space },					PRIORITY_PANEL, [this]{ /* play/pause */ });
	table.Register("midi.selectAll",	 	{ ImGuiKey_LeftCtrl, ImGuiKey_A }, 	PRIORITY_PANEL, [this]{ SelectAllNotes(); });
	table.Register("midi.duplicate",	 	{ ImGuiKey_LeftCtrl, ImGuiKey_D }, 	PRIORITY_PANEL, [this]{ /* duplicate */ });
	table.Register("midi.quantize",	 		{ ImGuiKey_Q },						PRIORITY_PANEL, [this]{ /* quantize */ });
	table.Register("midi.transpose.up", 	{ ImGuiKey_UpArrow },			 	PRIORITY_PANEL, [this]{ /* +1 semitone */ });
	table.Register("midi.transpose.down", 	{ ImGuiKey_DownArrow },				PRIORITY_PANEL, [this]{ /* -1 semitone */ });
}

int MidiEditor::PianoTopRow() const
{
	return (m_config.totalKeys - 1) - pianoKeys.layout.highestNote;
}

int MidiEditor::PianoBottomRow() const
{
	return (m_config.totalKeys - 1) - pianoKeys.layout.lowestNote;
}

void MidiEditor::ClampScrollY(float noteHeight, float viewHeight)
{
	const float bodyHeight = viewHeight - m_config.rulerHeight;
	const float visibleRows = bodyHeight / noteHeight;
	const float minScroll = (float)PianoTopRow();
	const float maxScroll = (float)PianoBottomRow() - visibleRows + 1.0f;
	m_scrollY = std::clamp(m_scrollY, minScroll, std::max(minScroll, maxScroll));
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
	pianoKeys.RenderDebugPanel();
	ImGui::Separator();

	ImGui::BeginChild("##PianoRoll", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
	RenderPianoRoll();
	ImGui::EndChild();

	HandlePatternDrop();
}

void MidiEditor::RenderToolbar()
{
	int toolInt = (int)m_currentTool;
	m_toolbarRenderer.Render(&toolInt, &m_zoom, &m_snap, !m_selection.selectedNoteIndices.empty(), &m_timeSig, m_config, [this]{ DeleteSelectedNotes(); }, [this]{ SelectAllNotes(); }, [this]{ ClearSelection(); });
	m_currentTool = (Tool)toolInt;

	// ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
	// if (ImGui::Button(pianoKeys.showDebugPanel ? "Hide Tuner" : "Key Tuner"))
	//		pianoKeys.showDebugPanel = !pianoKeys.showDebugPanel;

	ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
	if (ImGui::Button(m_showPlayheads ? "Hide Playheads" : "Show Playheads"))
		m_showPlayheads = !m_showPlayheads;
}

void MidiEditor::RenderPianoRoll()
{
	ProjectState& state = ProjectState::GetInstance();
	ImDrawList* draw = ImGui::GetWindowDrawList();
	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();

	const float beatWidth = state.settings.midiSettings.defaultBeatWidth * m_zoom;
	const float noteHeight = state.settings.midiSettings.defaultBeatHeight * m_verticalZoom;
	const float pianoW = PianoWidth();

	if (ImGui::IsWindowHovered())
	{
		const float wheel = ImGui::GetIO().MouseWheel;
		if (ImGui::GetIO().KeyCtrl && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
		{
			m_verticalZoom += wheel * state.settings.generalSettings.verticalZoomSensitivity;
			m_verticalZoom = std::clamp(m_verticalZoom, m_config.minVerticalZoom, m_config.maxVerticalZoom);
		}
		else if (ImGui::GetIO().KeyCtrl)
		{
			m_zoom += wheel * state.settings.generalSettings.horizontalZoomSensitivity;
			m_zoom = std::clamp(m_zoom, m_config.minZoom, m_config.maxZoom);
		}
		else if (ImGui::GetIO().KeyShift)
		{
			m_scrollX -= wheel * state.settings.generalSettings.horizontalScrollSensitivity;
		}
		else
		{
			m_scrollY -= wheel * state.settings.generalSettings.verticalScrollSensitivity;
		}

		m_scrollX = std::max(0.0f, m_scrollX);
	}

	ClampScrollY(noteHeight, canvasSize.y);
	draw->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);
	draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(28, 28, 32, 255));

	m_gridRenderer.Render(draw, canvasPos, canvasSize, beatWidth, noteHeight, m_scrollX, m_scrollY, pianoW, m_snap, m_timeSig, m_config, pianoKeys.layout.lowestNote, pianoKeys.layout.highestNote);
	m_noteRenderer.Render(draw, canvasPos, canvasSize, beatWidth, noteHeight, m_scrollX, m_scrollY, pianoW, GetCurrentPattern(), m_selection.selectedNoteIndices, m_config);
	pianoKeys.Render(draw, canvasPos, canvasSize, noteHeight, m_scrollY, m_config);

	RenderPlayhead(draw, canvasPos, canvasSize, beatWidth, pianoW);

	if (m_selection.isSelecting)
	{
		const ImVec2 p1 = m_selection.selectionStart;
		const ImVec2 p2 = ImGui::GetMousePos();
		draw->AddRectFilled(p1, p2, IM_COL32(100, 150, 255, 30));
		draw->AddRect(p1, p2, IM_COL32(100, 150, 255, 200), 0.0f, 0, 2.0f);
	}

	pianoKeys.HandleInput(canvasPos, canvasSize, noteHeight, m_scrollY, m_config);
	HandleInput(canvasPos, canvasSize, beatWidth, noteHeight);
	draw->PopClipRect();
}

void MidiEditor::HandleInput(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight)
{
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		if (m_previewNote != -1)
		{
			pianoKeys.ReleaseKey(m_previewNote);
			m_previewNote = -1;
		}
		m_selection.isDragging	 = false;
		m_selection.isResizing	 = false;
		m_selection.isSelecting	 = false;
		m_selection.draggedNoteIndex = -1;
		m_selection.originalPositions.clear();
	}

	if (!ImGui::IsWindowHovered() && !m_selection.isDragging && !m_selection.isResizing && !m_selection.isSelecting)
		return;

	const ImVec2 mousePos = ImGui::GetMousePos();
	const float pianoW = PianoWidth();
	const float gridStartX = canvasPos.x + pianoW;
	const float bodyTopY = canvasPos.y + m_config.rulerHeight;
	const bool inGrid = mousePos.x >= gridStartX && mousePos.x < canvasPos.x + canvasSize.x && mousePos.y >= bodyTopY && mousePos.y < canvasPos.y + canvasSize.y;

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
	{
		const ImVec2 delta = ImGui::GetIO().MouseDelta;
		if (ImGui::GetIO().KeyCtrl)
		{
			m_verticalZoom -= delta.y * 0.01f;
			m_verticalZoom = std::clamp(m_verticalZoom, m_config.minVerticalZoom, m_config.maxVerticalZoom);
		}
		else
		{
			m_scrollX -= delta.x;
			m_scrollY -= delta.y / noteHeight;
		}
		m_scrollX = std::max(0.0f, m_scrollX);
		ClampScrollY(noteHeight, canvasSize.y);
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
		return;
	}

	const float relX = mousePos.x - gridStartX + m_scrollX;
	const float relY = mousePos.y - bodyTopY;
	const double beatPos = relX / beatWidth;
	const int noteNum = (m_config.totalKeys - 1) - (int)((relY + m_scrollY * noteHeight) / noteHeight);
	UpdateCursor(canvasPos, canvasSize, beatWidth, noteHeight, beatPos, noteNum, inGrid);

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inGrid)
	{
		if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
			return;

		const int clicked = FindNoteAt(beatPos, noteNum);

		if (noteNum >= 0 && noteNum <= 127)
		{
			if (m_previewNote != -1 && m_previewNote != noteNum)
				pianoKeys.ReleaseKey(m_previewNote);

			pianoKeys.PressKey(noteNum);
			m_previewNote = noteNum;
		}

		if (m_currentTool == Tool::Draw)
		{
			if (clicked == -1)
			{
				ClearSelection();
				CreateNote(beatPos, noteNum);
				m_selection.selectedNoteIndices.push_back((int)GetCurrentPattern().notes.size() - 1);
				m_selection.isDragging = true;
				m_selection.draggedNoteIndex = m_selection.selectedNoteIndices[0];
				m_selection.dragStartPosition = mousePos;
				StoreOriginalPositions();
			}
			else
			{
				MidiPattern& pat = GetCurrentPattern();
				const float endX = gridStartX + (float)(pat.notes[clicked].startBeat + pat.notes[clicked].lengthBeats) * beatWidth - m_scrollX;
				if (mousePos.x > endX - 6 && mousePos.x < endX + 2)
				{
					m_selection.isResizing = true;
					m_selection.draggedNoteIndex = clicked;
				}
				else
				{
					if (!ImGui::GetIO().KeyCtrl)
						ClearSelection();

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
				MidiPattern& pat = GetCurrentPattern();
				const float endX = gridStartX + (float)(pat.notes[clicked].startBeat + pat.notes[clicked].lengthBeats) * beatWidth - m_scrollX;

				if (mousePos.x > endX - 6 && mousePos.x < endX + 2)
				{
					m_selection.isResizing = true;
					m_selection.draggedNoteIndex = clicked;
				}
				else
				{
					const bool already = std::find(m_selection.selectedNoteIndices.begin(), m_selection.selectedNoteIndices.end(), clicked) != m_selection.selectedNoteIndices.end();
					if (!ImGui::GetIO().KeyCtrl && !already)
						ClearSelection();

					if (!already)
						m_selection.selectedNoteIndices.push_back(clicked);

					m_selection.isDragging = true;
					m_selection.draggedNoteIndex = clicked;
					m_selection.dragStartPosition = mousePos;
					StoreOriginalPositions();
				}
			}
			else
			{
				if (!ImGui::GetIO().KeyCtrl) ClearSelection();
				m_selection.isSelecting = true;
				m_selection.selectionStart = mousePos;
			}
		}
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && inGrid)
	{
		if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
			return;

		if (m_currentTool == Tool::Select)
			ClearSelection();
		else
		{
			const int idx = FindNoteAt(beatPos, noteNum);
			if (idx != -1)
				GetCurrentPattern().notes.erase(GetCurrentPattern().notes.begin() + idx);
		}
	}

	if (m_selection.isDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		MidiPattern& pat = GetCurrentPattern();
		const ImVec2 delta = { mousePos.x - m_selection.dragStartPosition.x, mousePos.y - m_selection.dragStartPosition.y };
		double deltaBeat = delta.x / beatWidth;
		const int deltaPitch = -(int)(delta.y / noteHeight);

		double minStart = DBL_MAX;
		for (int idx : m_selection.selectedNoteIndices)
		{
			if (idx < (int)m_selection.originalPositions.size())
				minStart = std::min(minStart, m_selection.originalPositions[idx].startBeat);
		}

		if (minStart + deltaBeat < 0.0)
			deltaBeat = -minStart;

		for (int idx : m_selection.selectedNoteIndices)
		{
			if (idx < 0 || idx >= (int)pat.notes.size())
				continue;

			if (idx >= (int)m_selection.originalPositions.size())
				continue;

			const int newPitch = std::clamp(m_selection.originalPositions[idx].pitch + deltaPitch, 0, 127);
			pat.notes[idx].startBeat = SnapToGrid(m_selection.originalPositions[idx].startBeat + deltaBeat);
			pat.notes[idx].keyNumber = newPitch;
		}

		if (!m_selection.selectedNoteIndices.empty())
		{
			const int idx = m_selection.selectedNoteIndices[0];
			if (idx >= 0 && idx < (int)pat.notes.size())
			{
				const int dragPitch = pat.notes[idx].keyNumber;
				if (dragPitch != m_previewNote)
				{
					if (m_previewNote != -1)
						pianoKeys.ReleaseKey(m_previewNote);

					pianoKeys.PressKey(dragPitch);
					m_previewNote = dragPitch;
				}
			}
		}
	}

	if (m_selection.isResizing && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		MidiPattern& pat = GetCurrentPattern();
		const int idx = m_selection.draggedNoteIndex;
		if (idx >= 0 && idx < (int)pat.notes.size())
		{
			const double snapCell = (m_snap > 0) ? (4.0 / (double)m_snap) : 0.25;
			const double newLen = std::max(snapCell, SnapToGrid(beatPos - pat.notes[idx].startBeat));
			pat.notes[idx].lengthBeats = newLen;
			m_lastNoteLength = (float)newLen;
		}
	}

	if (m_selection.isSelecting && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		m_selection.selectionEnd = mousePos;

		MidiPattern& pat = GetCurrentPattern();
		const ImVec2 boxMin = { std::min<float>(m_selection.selectionStart.x, mousePos.x), std::min<float>(m_selection.selectionStart.y, mousePos.y) };
		const ImVec2 boxMax = { std::max<float>(m_selection.selectionStart.x, mousePos.x), std::max<float>(m_selection.selectionStart.y, mousePos.y) };

		for (int i = 0; i < (int)pat.notes.size(); ++i)
		{
			const auto& n = pat.notes[i];
			const float x1 = gridStartX + (float)n.startBeat * beatWidth - m_scrollX;
			const float x2 = x1 + (float)n.lengthBeats * beatWidth;
			const float y = bodyTopY + ((float)((m_config.totalKeys - 1) - n.keyNumber) - m_scrollY) * noteHeight;

			if (x2 >= boxMin.x && x1 <= boxMax.x && y + noteHeight >= boxMin.y && y <= boxMax.y)
			{
				if (std::find(m_selection.selectedNoteIndices.begin(), m_selection.selectedNoteIndices.end(), i) == m_selection.selectedNoteIndices.end())
					m_selection.selectedNoteIndices.push_back(i);
			}
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
		DeleteSelectedNotes();

	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
		SelectAllNotes();
}

void MidiEditor::RenderPlayhead(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float pianoW)
{
	ProjectState& state = ProjectState::GetInstance();
	if (!state.isPlaying || !m_showPlayheads)
	{
		for (int key : m_playheadPressedKeys)
		{
			pianoKeys.ReleaseKey(key);
		}

		m_playheadPressedKeys.clear();
		return;
	}

	const float gridStartX = canvasPos.x + pianoW;
	const float bodyTopY = canvasPos.y + m_config.rulerHeight;
	const MidiPattern& pattern = GetCurrentPattern();
	std::vector<ActivePlayhead> playheads;

	for (const auto& track : state.tracks)
	{
		if (track.silenced)
			continue;

		if (track.type != TrackType::Midi)
			continue;

		for (const auto& block : track.blocks)
		{
			if ((int)block.midiBlock.patternIndex != (int)state.currentMidiPatternIndex)
				continue;

			const double blockStart = block.midiBlock.startBeat;
			const double blockEnd = blockStart + block.midiBlock.lengthBeats;

			if (state.timelinePositionBeats < blockStart || state.timelinePositionBeats >= blockEnd)
				continue;

			ActivePlayhead ph;
			ph.patternLocalBeat = (state.timelinePositionBeats - blockStart) + block.midiBlock.startOffsetBeats;
			ph.trackColor = {track.color.x, track.color.y, track.color.z, track.color.w};
			playheads.push_back(ph);
			break;
		}
	}

	std::set<int> nowPlaying;
	for (const auto& ph : playheads)
	{
		for (const auto& note : pattern.notes)
		{
			if (ph.patternLocalBeat >= note.startBeat && ph.patternLocalBeat < note.startBeat + note.lengthBeats)
				nowPlaying.insert((int)note.keyNumber);
		}
	}

	for (int key : m_playheadPressedKeys)
		if (nowPlaying.find(key) == nowPlaying.end())
			pianoKeys.ReleaseKey(key);

	for (int key : nowPlaying)
		if (m_playheadPressedKeys.find(key) == m_playheadPressedKeys.end())
			pianoKeys.PressKey(key);

	m_playheadPressedKeys = nowPlaying;
	if (playheads.empty())
		return;

	for (int idx = 0; idx < (int)playheads.size(); ++idx)
	{
		const ActivePlayhead& ph = playheads[idx];
		const float baseX = gridStartX + (float)ph.patternLocalBeat * beatWidth - m_scrollX;
		const float x = baseX + (float)idx * 2.0f;

		if (x < gridStartX || x > canvasPos.x + canvasSize.x)
			continue;

		const ImVec4& tc = ph.trackColor;
		const ImU32 col = IM_COL32((int)(tc.x * 255), (int)(tc.y * 255), (int)(tc.z * 255), 210);
		draw->AddLine(ImVec2(x, bodyTopY), ImVec2(x, canvasPos.y + canvasSize.y), col, 2.0f);
		draw->AddTriangleFilled(ImVec2(x, bodyTopY), ImVec2(x - 5, bodyTopY - 8), ImVec2(x + 5, bodyTopY - 8), col);

		const float sqSize = 6.0f;
		draw->AddRectFilled(ImVec2(x - sqSize * 0.5f, bodyTopY - sqSize - 2.0f), ImVec2(x - sqSize * 0.5f + sqSize, bodyTopY - 2.0f), col, 1.0f);
	}
}

void MidiEditor::UpdateCursor(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight, double beatPos, int noteNum, bool inGrid)
{
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

	if (!inGrid)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		return;
	}

	const ImVec2 mousePos = ImGui::GetMousePos();
	const float gridStartX = canvasPos.x + PianoWidth();
	const int hovered = FindNoteAt(beatPos, noteNum);
	if (hovered != -1)
	{
		MidiPattern& pat = GetCurrentPattern();
		const float endX = gridStartX + (float)(pat.notes[hovered].startBeat + pat.notes[hovered].lengthBeats) * beatWidth - m_scrollX;
		ImGui::SetMouseCursor(mousePos.x > endX - 6 && mousePos.x < endX + 2 ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_Hand);
	}
	else
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
	}
}

void MidiEditor::HandlePatternDrop()
{
	ProjectState& state = ProjectState::GetInstance();
	if (ImGui::GetDragDropPayload() && ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("PATTERN_INDEX"))
			state.currentMidiPatternIndex = *(const uint16_t*)p->Data;

		ImGui::EndDragDropTarget();
	}
}

void MidiEditor::CreateNote(double beatPos, int noteNum)
{
	const double snapCellSize = (m_snap > 0) ? (4.0 / (double)m_snap) : 0.25;
	const double length = std::max<double>(m_lastNoteLength, snapCellSize);

	MidiNote note;
	note.keyNumber = std::clamp(noteNum, 0, 127);
	note.startBeat = SnapToGrid(beatPos);
	note.lengthBeats = length;
	note.velocity = 100;
	GetCurrentPattern().notes.push_back(note);
}

int MidiEditor::FindNoteAt(double beatPos, int noteNum)
{
	MidiPattern& pat = GetCurrentPattern();
	for (int i = ((int)pat.notes.size() - 1); i >= 0; --i)
	{
		const auto& n = pat.notes[i];
		if (n.keyNumber == noteNum && beatPos >= n.startBeat && beatPos <= n.startBeat + n.lengthBeats)
			return i;
	}

	return -1;
}

double MidiEditor::SnapToGrid(double value)
{
	const double snapSize = 4.0 / (double)m_snap;
	return std::floor(value / snapSize) * snapSize;
}

void MidiEditor::StoreOriginalPositions()
{
	MidiPattern& pat = GetCurrentPattern();
	m_selection.originalPositions.resize(pat.notes.size());
	for (size_t i = 0; i < pat.notes.size(); ++i)
	{
		m_selection.originalPositions[i] = { pat.notes[i].startBeat, (int)pat.notes[i].keyNumber };
	}
}

void MidiEditor::ClearSelection()
{
	m_selection.selectedNoteIndices.clear();
}

void MidiEditor::SelectAllNotes()
{
	MidiPattern& pat = GetCurrentPattern();
	m_selection.selectedNoteIndices.clear();
	for (int i = 0; i < (int)pat.notes.size(); ++i)
	{
		m_selection.selectedNoteIndices.push_back(i);
	}
}

void MidiEditor::DeleteSelectedNotes()
{
	MidiPattern& pat = GetCurrentPattern();
	std::sort(m_selection.selectedNoteIndices.rbegin(), m_selection.selectedNoteIndices.rend());
	for (int idx : m_selection.selectedNoteIndices)
	{
		if (idx >= 0 && idx < (int)pat.notes.size())
			pat.notes.erase(pat.notes.begin() + idx);
	}

	m_selection.selectedNoteIndices.clear();
}

MidiPattern& MidiEditor::GetCurrentPattern()
{
	ProjectState& state = ProjectState::GetInstance();
	if (state.currentMidiPatternIndex >= state.patterns.size())
		state.currentMidiPatternIndex = 0;

	return state.patterns[state.currentMidiPatternIndex];
}