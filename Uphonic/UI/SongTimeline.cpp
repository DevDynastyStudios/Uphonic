#include "SongTimeline.h"
#include "../Core/ProjectState.h"
#include "../Plugin/PluginManager.h"
#include "../Audio/AudioEngine.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <cfloat>
#include <imgui_internal.h>

static constexpr float SECONDS_PER_MINUTE = 60.0f;
static constexpr float BEATS_PER_MEASURE = 4.0f;
static constexpr float MINIMUM_PITCH_RANGE = 8.0f;
static constexpr float WAVEFORM_AMPLITUDE_SCALE = 0.45f;
static constexpr float WAVEFORM_CENTER_LINE_ALPHA = 60.0f;
static constexpr float RESIZE_HANDLE_ALPHA = 200.0f;
static constexpr float MUTED_COLOR_MULTIPLIER = 0.4f;
static constexpr float SELECTED_COLOR_MULTIPLIER = 1.3f;
static constexpr float BORDER_COLOR_MULTIPLIER = 1.5f;
static constexpr float MINIMUM_NOTE_LENGTH_BEATS = 0.125f;

static bool ToggleButton(const char* id, bool* v, const char* label)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    float h = ImGui::GetFrameHeight();
    ImVec2 size(h, h);

    ImGuiID buttonId = window->GetID(id);

    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, buttonId))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, buttonId, &hovered, &held);

    if (pressed)
        *v = !*v;

    ImU32 col;
    if (*v)
        col = ImGui::GetColorU32(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    else
        col = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);

    ImGui::RenderNavHighlight(bb, buttonId);
    ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    if (label && label[0] != '\0')
    {
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        ImGui::TextUnformatted(label);
    }

    return pressed;
}

SongTimeline::SongTimeline() : Naui::Panel("Song Timeline")
{
	m_zoom = m_config.defaultZoom;
	m_scrollX = 0.0f;
	m_scrollY = 0.0f;
	m_snap = m_config.defaultSnap;
	m_currentTool = Tool::Select;
	m_selection.isDragging = false;
	m_selection.isResizing = false;
	m_selection.isResizingLeft = false;
	m_selection.isSelecting = false;
	m_selection.draggedTrack = -1;
	m_selection.draggedInstance = -1;
	
	ProjectState& state = ProjectState::GetInstance();
	if (state.tracks.empty())
	{
		AudioTrack track;
		track.type = TrackType::Midi;
		track.color = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
		track.name = "MIDI 1";
		state.tracks.push_back(track);
	}
}

AudioTrack& SongTimeline::CreateTrack(std::string title, ImVec4 color)
{
	ProjectState& state = ProjectState::GetInstance();
	AudioTrack track;
	track.type = TrackType::Midi;
	track.color = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
	track.name = title;
	state.tracks.push_back(track);
	return state.tracks.back();
}

double SongTimeline::GetTimelineDuration()
{
	ProjectState& state = ProjectState::GetInstance();
	double endBeat = 0.0;

	for (const auto& track : state.tracks)
	{
		for (const auto& block : track.blocks)
		{
			double blockEnd = (track.type == TrackType::Midi) ? block.midiBlock.startBeat + block.midiBlock.lengthBeats : block.sampleBlock.startBeat + block.sampleBlock.lengthBeats;

			if (blockEnd > endBeat)
				endBeat = blockEnd;
		}
	}

	return endBeat;
}

double SongTimeline::GetPlayheadPositionNormalized()
{
	ProjectState& state = ProjectState::GetInstance();
	double duration = GetTimelineDuration();
	if(duration <= 0.0)
		return 0.0;

	return state.timelinePositionBeats / duration;
}

void SongTimeline::OnRender()
{
	RenderToolbar();
	ImGui::Separator();
	
	ImGui::BeginChild("##TimelineMain", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	RenderTimeline();
	ImGui::EndChild();
}

void SongTimeline::EnsureUIStates()
{
	ProjectState& state = ProjectState::GetInstance();
	while (m_trackUIStates.size() < state.tracks.size())
	{
		TrackUIState state;
		state.height = m_config.defaultTrackHeight;
		state.collapsed = false;
		m_trackUIStates.push_back(state);
	}
}

void SongTimeline::RenderToolbar()
{
	ProjectState& state = ProjectState::GetInstance();
	
	if (ImGui::Button(state.isPlaying ? "Stop" : "Play"))
	{
		AudioEngine::StopAllNotes();
		state.isPlaying = !state.isPlaying;
	}
	ImGui::SameLine();
	if (ImGui::Button("<<"))
	{
		state.timelinePositionBeats = 0.0;
		AudioEngine::StopAllNotes();
	}
	
	ImGui::SameLine();
	ImGui::Text("|");
	ImGui::SameLine();
	
	ImGui::Text("BPM:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("##BPM", &state.beatsPerMinute, 0.1f, 20.0f, 999.0f, "%.1f"))
	{
		for (auto& track : state.tracks)
		{
			for (auto& block : track.blocks)
			{
				if (track.type != TrackType::Audio)
					continue;
				const AudioSample& sample = state.samples[block.sampleBlock.sampleIndex];
				const double secondsPerBeat = SECONDS_PER_MINUTE / state.beatsPerMinute;
				const double sampleDurationSeconds = (double)sample.frameCount / state.settings.audioSampleRate;
				const double sampleDurationBeats = sampleDurationSeconds / secondsPerBeat;
				const double availableDuration = (sampleDurationBeats - block.sampleBlock.startOffsetBeats) * block.sampleBlock.stretchScale;
				block.sampleBlock.lengthBeats = std::min<double>(block.sampleBlock.lengthBeats, availableDuration);
			}
		}
	}
	ImGui::SameLine();
	ImGui::Text("|");
	ImGui::SameLine();
	
	if (ImGui::RadioButton("Select", m_currentTool == Tool::Select))
	{
		m_currentTool = Tool::Select;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Draw", m_currentTool == Tool::Draw))
	{
		m_currentTool = Tool::Draw;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Cut", m_currentTool == Tool::Cut))
	{
		m_currentTool = Tool::Cut;
	}
	
	ImGui::SameLine();
	ImGui::Text("|");
	ImGui::SameLine();
	
	ImGui::Text("Snap:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	const char* snapPreview = GetSnapName(m_snap);
	if (ImGui::BeginCombo("##Snap", snapPreview, ImGuiComboFlags_NoArrowButton))
	{
		if (ImGui::Selectable("Off", m_snap == 0)) m_snap = 0;
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
	
	if (ImGui::Button("+ Audio Track"))
	{
		AudioTrack track;
		track.type = TrackType::Audio;
		track.color = ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
		track.name = "Audio " + std::to_string(state.tracks.size() + 1);
		state.tracks.push_back(track);
	}
	ImGui::SameLine();
	if (ImGui::Button("+ MIDI Track"))
	{
		AudioTrack track;
		track.type = TrackType::Midi;
		track.color = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
		track.name = "MIDI " + std::to_string(state.tracks.size() + 1);
		state.tracks.push_back(track);
	}

	ImGui::SameLine();
	std::string playheadText = m_config.followPlayhead ? "Unfollow Playhead" : "Follow Playhead";
	ToggleButton("playhead_toggle", &m_config.followPlayhead, "Follow Playhead");
}

void SongTimeline::RenderTimeline()
{
	EnsureUIStates();
	
	ImDrawList* draw = ImGui::GetWindowDrawList();
	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	
	float beatWidth = m_config.defaultBeatWidth * m_zoom;
	
	if (ImGui::IsWindowHovered())
	{
		float wheel = ImGui::GetIO().MouseWheel;
		if (ImGui::GetIO().KeyCtrl)
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
		m_scrollX = std::max<float>(0.0f, m_scrollX);
		m_scrollY = std::max<float>(0.0f, m_scrollY);
	}

	if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
	{
		ImVec2 delta = ImGui::GetIO().MouseDelta;
		m_scrollX -= delta.x;
		m_scrollY -= delta.y;
		m_scrollX = std::max<float>(0.0f, m_scrollX);
		m_scrollY = std::max<float>(0.0f, m_scrollY);
	}
	
	draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(25, 25, 28, 255));
	ImGui::PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

	float timelineStartY = canvasPos.y;
	float currentY = timelineStartY - m_scrollY + m_config.rulerHeight;
	ProjectState& state = ProjectState::GetInstance();
	for (size_t i = 0; i < state.tracks.size(); i++)
	{
		if (currentY > canvasPos.y + canvasSize.y) break;
		if (currentY + m_trackUIStates[i].height < timelineStartY)
		{
			currentY += m_trackUIStates[i].height;
			continue;
		}
		
		RenderTrack(draw, canvasPos, canvasSize, beatWidth, i, currentY);
		currentY += m_trackUIStates[i].height;
	}

	ImGui::PopClipRect();

	ImGui::PushClipRect(ImVec2(canvasPos.x + m_config.trackHeaderWidth, canvasPos.y), ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);
	RenderRuler(draw, canvasPos, canvasSize, beatWidth);
	RenderPlayhead(draw, canvasPos, canvasSize, beatWidth);
	ImGui::PopClipRect();

	HandleInput(canvasPos, canvasSize, beatWidth);

	if (m_selection.isSelecting)
	{
		ImVec2 p1 = m_selection.selectionStart;
		ImVec2 p2 = ImGui::GetMousePos();
		draw->AddRect(p1, p2, IM_COL32(100, 150, 255, 200), 0.0f, 0, 2.0f);
		draw->AddRectFilled(p1, p2, IM_COL32(100, 150, 255, 30));
	}
}

void SongTimeline::RenderRuler(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth)
{
	float rulerStartX = canvasPos.x + m_config.trackHeaderWidth;
	float rulerWidth = canvasSize.x - m_config.trackHeaderWidth;
	
	draw->AddRectFilled(ImVec2(canvasPos.x, canvasPos.y), ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + m_config.rulerHeight), IM_COL32(35, 35, 38, 255));
	draw->AddRectFilled(ImVec2(canvasPos.x, canvasPos.y), ImVec2(canvasPos.x + m_config.trackHeaderWidth, canvasPos.y + m_config.rulerHeight), IM_COL32(30, 30, 33, 255));
	draw->AddLine(ImVec2(canvasPos.x, canvasPos.y + m_config.rulerHeight), ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + m_config.rulerHeight), IM_COL32(60, 60, 65, 255), 1.0f);
	
	int visibleBeats = (int)((rulerWidth + m_scrollX) / beatWidth) + 2;
	int startBeat = (int)(m_scrollX / beatWidth);
	
	for (int i = 0; i < visibleBeats; i++)
	{
		int beat = startBeat + i;
		float x = rulerStartX + beat * beatWidth - m_scrollX;
		
		if (x < rulerStartX - beatWidth) continue;
		if (x > canvasPos.x + canvasSize.x) break;
		
		bool isMeasure = (beat % (int)BEATS_PER_MEASURE == 0);
		ImU32 lineCol = isMeasure ? IM_COL32(100, 100, 105, 255) : IM_COL32(60, 60, 65, 255);
		float lineHeight = isMeasure ? m_config.rulerHeight * 0.6f : m_config.rulerHeight * 0.3f;
		
		draw->AddLine(ImVec2(x, canvasPos.y + m_config.rulerHeight - lineHeight), ImVec2(x, canvasPos.y + m_config.rulerHeight), lineCol, 1.0f);
		
		if (isMeasure)
		{
			char buf[16];
			snprintf(buf, sizeof(buf), "%d", (beat / (int)BEATS_PER_MEASURE) + 1);
			draw->AddText(ImVec2(x + 3, canvasPos.y + 5), IM_COL32(180, 180, 185, 255), buf);
		}
	}
}

void SongTimeline::RenderPlayhead(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth)
{
	ProjectState& state = ProjectState::GetInstance();
	float leftEdge  = canvasPos.x + m_config.trackHeaderWidth;
	float rightEdge = leftEdge + canvasSize.x;
	float playheadX = leftEdge + state.timelinePositionBeats * beatWidth - m_scrollX;
	bool playheadVisible = (playheadX >= leftEdge && playheadX <= rightEdge);

	if (state.isPlaying && m_config.followPlayhead)
	{
	    float lockPosX = leftEdge + canvasSize.x * m_config.playheadLockPosition;
	    float desiredScroll = (state.timelinePositionBeats * beatWidth) - (lockPosX - leftEdge);
	    float speed = 0.15f;
	    m_scrollX = m_scrollX + (desiredScroll - m_scrollX) * speed;
	}

	draw->AddLine(ImVec2(playheadX, canvasPos.y), ImVec2(playheadX, canvasPos.y + canvasSize.y), IM_COL32(255, 200, 100, 255), 2.0f);
	
	ImVec2 tri[3] = {
		ImVec2(playheadX, canvasPos.y + m_config.rulerHeight),
		ImVec2(playheadX - 6, canvasPos.y + m_config.rulerHeight - 10),
		ImVec2(playheadX + 6, canvasPos.y + m_config.rulerHeight - 10)
	};
	draw->AddTriangleFilled(tri[0], tri[1], tri[2], IM_COL32(255, 200, 100, 255));
}

void SongTimeline::RenderTrack(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, size_t trackIndex, float yPos)
{
	ProjectState& state = ProjectState::GetInstance();
	AudioTrack& track = state.tracks[trackIndex];
	TrackUIState& ui = m_trackUIStates[trackIndex];
	float trackStartX = canvasPos.x + m_config.trackHeaderWidth;
	
	ImU32 laneColor = (trackIndex % 2 == 0) ? IM_COL32(28, 28, 31, 255) : IM_COL32(25, 25, 28, 255);
	draw->AddRectFilled(ImVec2(trackStartX, yPos), ImVec2(canvasPos.x + canvasSize.x, yPos + ui.height), laneColor);
	draw->AddLine(ImVec2(trackStartX, yPos + ui.height), ImVec2(canvasPos.x + canvasSize.x, yPos + ui.height), IM_COL32(40, 40, 45, 255), 1.0f);
	
	int visibleBeats = (int)((canvasSize.x - m_config.trackHeaderWidth + m_scrollX) / beatWidth) + 2;
	int startBeat = (int)(m_scrollX / beatWidth);
	
	for (int i = 0; i < visibleBeats; i++)
	{
		int beat = startBeat + i;
		float x = trackStartX + beat * beatWidth - m_scrollX;
		
		if (beat % (int)BEATS_PER_MEASURE == 0)
		{
			draw->AddLine(ImVec2(x, yPos), ImVec2(x, yPos + ui.height), IM_COL32(40, 40, 45, 100), 1.0f);
		}
	}
	
	if (track.type == TrackType::Midi)
	{
		for (size_t i = 0; i < track.blocks.size(); i++)
		{
			RenderMidiInstance(draw, canvasPos, canvasSize, beatWidth, trackIndex, i, yPos, trackStartX, ui);
		}
	}
	else
	{
		for (size_t i = 0; i < track.blocks.size(); i++)
		{
			RenderSampleInstance(draw, canvasPos, canvasSize, beatWidth, trackIndex, i, yPos, trackStartX, ui);
		}
	}

	if (ImGui::GetDragDropPayload())
	{
		ImGui::SetCursorScreenPos(ImVec2(trackStartX, yPos));
		ImGui::InvisibleButton(("##track_drop_" + std::to_string(trackIndex)).c_str(), ImVec2(canvasSize.x - m_config.trackHeaderWidth, ui.height));

		if (ImGui::BeginDragDropTarget())
		{
			if (track.type == TrackType::Audio)
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAMPLE_INDEX"))
				{
					uint16_t sampleIdx = *(const uint16_t*)payload->Data;
					
					ImVec2 mousePos = ImGui::GetMousePos();
					double beatPos = (mousePos.x - trackStartX + m_scrollX) / beatWidth;
					double snappedStart = SnapToGrid(beatPos);
					
					if (sampleIdx < state.samples.size())
					{
						TimelineBlock instance;
						instance.sampleBlock.startBeat = snappedStart;
						
						const AudioSample& sample = state.samples[sampleIdx];
						double secondsPerBeat = SECONDS_PER_MINUTE / state.beatsPerMinute;
						double sampleDurationSeconds = (double)sample.frameCount / state.settings.audioSampleRate;
						instance.sampleBlock.lengthBeats = sampleDurationSeconds / secondsPerBeat;
						
						instance.sampleBlock.startOffsetBeats = 0.0;
						instance.sampleBlock.stretchScale = 1.0;
						instance.sampleBlock.sampleIndex = sampleIdx;
						
						track.blocks.push_back(instance);
					}
				}
			}
			else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PATTERN_INDEX"))
			{
				uint16_t patternIdx = *(const uint16_t*)payload->Data;
				
				ImVec2 mousePos = ImGui::GetMousePos();
				double beatPos = (mousePos.x - trackStartX + m_scrollX) / beatWidth;
				double snappedStart = SnapToGrid(beatPos);
				
				if (patternIdx < state.patterns.size())
				{
					TimelineBlock instance;
					instance.midiBlock.startBeat = snappedStart;
					
					instance.midiBlock.lengthBeats = m_config.defaultInstanceLength;
					instance.midiBlock.startOffsetBeats = 0.0;
					instance.midiBlock.patternIndex = patternIdx;
					
					track.blocks.push_back(instance);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	RenderTrackHeader(draw, canvasPos, trackIndex, yPos);
}

void SongTimeline::RenderTrackHeader(ImDrawList* draw, ImVec2 canvasPos, size_t trackIndex, float yPos)
{
	ProjectState& state = ProjectState::GetInstance();
	TrackUIState& ui = m_trackUIStates[trackIndex];
	AudioTrack& track = state.tracks[trackIndex];
	
	draw->AddRectFilled(ImVec2(canvasPos.x, yPos), ImVec2(canvasPos.x + m_config.trackHeaderWidth, yPos + ui.height), IM_COL32(35, 35, 38, 255));
	draw->AddLine(ImVec2(canvasPos.x + m_config.trackHeaderWidth, yPos), ImVec2(canvasPos.x + m_config.trackHeaderWidth, yPos + ui.height), IM_COL32(50, 50, 55, 255), 2.0f);
	
	ImU32 trackCol = ImGui::ColorConvertFloat4ToU32(track.color);
	draw->AddRectFilled(ImVec2(canvasPos.x + 5, yPos + 8), ImVec2(canvasPos.x + 10, yPos + ui.height - 8), trackCol);
	
	ImGui::PushID((int)trackIndex);
	if (ImGui::IsMouseHoveringRect(ImVec2(canvasPos.x, yPos), ImVec2(canvasPos.x + m_config.trackHeaderWidth, yPos + ui.height)))
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			ImGui::OpenPopup("TrackContextMenu");
		else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			PluginManager::OpenEffect(track.instrument);
		}
	}
	if (ImGui::BeginPopupContextItem("TrackContextMenu"))
	{
		RenderTrackContextMenu(trackIndex);
		ImGui::EndPopup();
	}
	ImGui::PopID();

	ImGui::SetCursorScreenPos(ImVec2(canvasPos.x + 15, yPos + 10));
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 225, 255));
	ImGui::Text("%s", track.name.c_str());
	ImGui::PopStyleColor();
	
	const char* typeStr = (track.type == TrackType::Audio) ? "Audio" : "MIDI";
	ImGui::SetCursorScreenPos(ImVec2(canvasPos.x + 15, yPos + 28));
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(140, 140, 145, 255));
	ImGui::TextUnformatted(typeStr);
	ImGui::PopStyleColor();
	
	if (ui.height >= 60.0f)
	{
		float buttonY = yPos + 50;
		
		ImGui::SetCursorScreenPos(ImVec2(canvasPos.x + 10, buttonY));
		ImGui::PushID((int)trackIndex * 1000);
		
		ImGui::PushStyleColor(ImGuiCol_Button, track.muted ? IM_COL32(180, 100, 50, 180) : IM_COL32(60, 60, 65, 180));
		if (ImGui::SmallButton("M"))
		{
			track.muted = !track.muted;
		}
		ImGui::PopStyleColor();
		
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, track.solo ? IM_COL32(100, 180, 100, 180) : IM_COL32(60, 60, 65, 180));
		if (ImGui::SmallButton("S"))
		{
			track.solo = !track.solo;
		}
		ImGui::PopStyleColor();
		
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, track.armed ? IM_COL32(200, 80, 80, 180) : IM_COL32(60, 60, 65, 180));
		if (ImGui::SmallButton("R"))
		{
			track.armed = !track.armed;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();
		
		ImGui::PopID();
	}
}

void SongTimeline::RenderTrackContextMenu(size_t trackIndex)
{
	ProjectState& state = ProjectState::GetInstance();
	AudioTrack& track = state.tracks[trackIndex];
	
	ImGui::Text("Track Properties");
	ImGui::Separator();
	
	char nameBuffer[256];
	strncpy(nameBuffer, track.name.c_str(), sizeof(nameBuffer) - 1);
	nameBuffer[sizeof(nameBuffer) - 1] = '\0';
	
	ImGui::SetNextItemWidth(200);
	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
	{
		track.name = nameBuffer;
	}
	
	ImGui::SetNextItemWidth(200);
	ImGui::ColorEdit4("Color", (float*)&track.color, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
	
	ImGui::Separator();
	
	ImGui::SetNextItemWidth(200);
	float height = m_trackUIStates[trackIndex].height;
	if (ImGui::SliderFloat("Height", &height, m_config.minTrackHeight, m_config.maxTrackHeight, "%.0f px"))
	{
		m_trackUIStates[trackIndex].height = height;
	}
	
	ImGui::Separator();
	
	const char* typeStr = (track.type == TrackType::Audio) ? "Audio Track" : "MIDI Track";
	ImGui::TextDisabled("Type: %s", typeStr);
	
	if (track.type == TrackType::Midi)
	{
		ImGui::Separator();
		ImGui::Text("Instrument");
		
		if (track.instrument.plugin)
		{
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", track.instrument.plugin->GetName());
			
			if (ImGui::Button("Open Editor", ImVec2(200, 0)))
			{
				PluginManager::OpenEffect(track.instrument);
			}
			
			if (ImGui::Button("Remove Instrument", ImVec2(200, 0)))
			{
				PluginManager::UnloadEffect(track.instrument);
			}
		}
		else
		{
			ImGui::TextDisabled("No instrument loaded");
		}
		
		if (ImGui::BeginMenu("Load Instrument"))
		{
			int pluginId = 0;
			for (const auto& path : state.settings.pluginSearchPaths)
			{
				if (!std::filesystem::exists(path)) continue;
				
				for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
				{
					if (entry.is_regular_file() && entry.path().extension() == ".vst3")
					{
						ImGui::PushID(pluginId++);
						if (ImGui::MenuItem(entry.path().filename().replace_extension().string().c_str()))
						{
							PluginEffect& effect = track.instrument;
							if (effect.plugin)
								PluginManager::UnloadEffect(effect);
							PluginManager::LoadEffect(track.instrument, entry.path());
						}
						ImGui::PopID();
					}
				}
			}
			ImGui::EndMenu();
		}
	}
	
	ImGui::Separator();
	
	if (ImGui::MenuItem("Delete Track", nullptr, false, state.tracks.size() > 1))
	{
		DeleteTrack(trackIndex);
	}
	
	ImGui::Separator();
	
	if (ImGui::MenuItem("Clear All Blocks"))
	{
		track.blocks.clear();
	}
}

void SongTimeline::DeleteTrack(size_t trackIndex)
{
	ProjectState& state = ProjectState::GetInstance();
	if (trackIndex >= state.tracks.size() || state.tracks.size() <= 1) return;
	
	AudioTrack& track = state.tracks[trackIndex];
	
	if (track.instrument.plugin)
	{
		PluginManager::UnloadEffect(track.instrument);
	}
	
	for (auto& effect : track.effects)
	{
		if (effect.plugin)
		{
			PluginManager::UnloadEffect(effect);
		}
	}
	
	state.tracks.erase(state.tracks.begin() + trackIndex);
	
	if (trackIndex < m_trackUIStates.size())
	{
		m_trackUIStates.erase(m_trackUIStates.begin() + trackIndex);
	}
	
	m_selection.selectedInstances.erase(
		std::remove_if(m_selection.selectedInstances.begin(), m_selection.selectedInstances.end(),
			[trackIndex](const std::pair<int, int>& sel) {
				return sel.first == (int)trackIndex;
			}),
		m_selection.selectedInstances.end()
	);
	
	for (auto& [tIdx, iIdx] : m_selection.selectedInstances)
	{
		if (tIdx > (int)trackIndex)
		{
			tIdx--;
		}
	}
}

void SongTimeline::RenderMidiInstance(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, size_t trackIndex, size_t instanceIndex, float yPos, float trackStartX, TrackUIState& ui)
{
	ProjectState& state = ProjectState::GetInstance();
	AudioTrack& track = state.tracks[trackIndex];
	TimelineBlock& instance = track.blocks[instanceIndex];
	
	float instX = trackStartX + (float)instance.midiBlock.startBeat * beatWidth - m_scrollX;
	float instWidth = (float)instance.midiBlock.lengthBeats * beatWidth;
	float instY = yPos + m_config.instancePadding;
	float instHeight = ui.height - m_config.instancePadding * 2;
	float instStartOffset = (float)instance.midiBlock.startOffsetBeats * beatWidth;

	if (instX + instWidth < trackStartX || instX > canvasPos.x + canvasSize.x) return;
	
	bool selected = IsInstanceSelected((int)trackIndex, (int)instanceIndex);
	
	ImVec4 baseColor = track.color;
	if (track.muted)
	{
		baseColor.x *= MUTED_COLOR_MULTIPLIER;
		baseColor.y *= MUTED_COLOR_MULTIPLIER;
		baseColor.z *= MUTED_COLOR_MULTIPLIER;
	}
	if (selected)
	{
		baseColor.x = std::min<float>(1.0f, baseColor.x * SELECTED_COLOR_MULTIPLIER);
		baseColor.y = std::min<float>(1.0f, baseColor.y * SELECTED_COLOR_MULTIPLIER);
		baseColor.z = std::min<float>(1.0f, baseColor.z * SELECTED_COLOR_MULTIPLIER);
	}
	
	ImU32 instCol = ImGui::ColorConvertFloat4ToU32(baseColor);
	ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(
		ImVec4(baseColor.x * BORDER_COLOR_MULTIPLIER, baseColor.y * BORDER_COLOR_MULTIPLIER, baseColor.z * BORDER_COLOR_MULTIPLIER, 1.0f));
	
	draw->AddRectFilled(ImVec2(instX, instY), ImVec2(instX + instWidth, instY + instHeight), instCol, 3.0f);
	draw->AddRect(ImVec2(instX, instY), ImVec2(instX + instWidth, instY + instHeight), borderCol, 3.0f, 0, selected ? 2.5f : 1.5f);
	
	ImGui::PushClipRect(ImVec2(instX, instY), ImVec2(instX + instWidth, instY + instHeight), true);
	if (instWidth > 40.0f && instHeight > 20.0f)
	{
		ImVec2 textPos(instX + 8, instY + 6);
		draw->AddText(textPos, IM_COL32(255, 255, 255, 255), state.patterns[instance.midiBlock.patternIndex].name.c_str());
	}
	ImGui::PopClipRect();
	
	if (instWidth > 20.0f && instHeight > 25.0f && 
		instance.midiBlock.patternIndex < (int)state.patterns.size())
	{
		MidiPattern& pattern = state.patterns[instance.midiBlock.patternIndex];
		if (!pattern.notes.empty())
		{
			int minPitch = 127;
			int maxPitch = 0;
			for (const auto& note : pattern.notes)
			{
				minPitch = std::min<int>(minPitch, (int)note.keyNumber);
				maxPitch = std::max<int>(maxPitch, (int)note.keyNumber + 1);
			}
			
			int pitchRange = maxPitch - minPitch;
			if (pitchRange < (int)MINIMUM_PITCH_RANGE) pitchRange = (int)MINIMUM_PITCH_RANGE;
			
			float noteAreaY = instY + 20;
			float noteAreaHeight = instHeight - 22;
			
			if (noteAreaHeight > 10.0f)
			{
				ImU32 noteCol = IM_COL32(255, 255, 255, 180);
				
				for (const MidiNote& note : pattern.notes)
				{
					float noteX = instX - instStartOffset + (float)(note.startBeat / instance.midiBlock.lengthBeats) * instWidth;
					float noteW = (float)(note.lengthBeats / instance.midiBlock.lengthBeats) * instWidth;
					
					if (noteX < instX)
					{
						noteW -= (instX - noteX);
						noteX = instX;
					}
					if (noteX + noteW > instX + instWidth)
					{
						noteW = instX + instWidth - noteX;
					}
					
					if (noteW < 1.0f)
						continue;
					
					float pitchNormalized = (float)(note.keyNumber - minPitch) / (float)pitchRange;
					float noteHeight = std::max<float>(2.0f, noteAreaHeight / (pitchRange + 1));
					float noteY = noteAreaY + noteAreaHeight - (pitchNormalized * noteAreaHeight) - noteHeight;
					
					if (noteY < noteAreaY) noteY = noteAreaY;
					if (noteY + noteHeight > noteAreaY + noteAreaHeight)
					{
						noteHeight = noteAreaY + noteAreaHeight - noteY;
					}
					
					if (noteHeight < 1.0f)
						continue;
					
					draw->AddRectFilled(ImVec2(noteX, noteY), ImVec2(noteX + noteW, noteY + noteHeight), noteCol, 1.0f);
				}
			}
		}
	}

	if (selected && instHeight > 20.0f)
	{
		draw->AddRectFilled(ImVec2(instX, instY + m_config.instancePadding), ImVec2(instX + m_config.resizeHandleWidth, instY + instHeight - m_config.instancePadding), IM_COL32(255, 255, 255, (int)RESIZE_HANDLE_ALPHA));
		draw->AddRectFilled(ImVec2(instX + instWidth - m_config.resizeHandleWidth, instY + m_config.instancePadding), ImVec2(instX + instWidth, instY + instHeight - m_config.instancePadding), IM_COL32(255, 255, 255, (int)RESIZE_HANDLE_ALPHA));
	}
}

void SongTimeline::RenderSampleInstance(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, size_t trackIndex, size_t instanceIndex, float yPos, float trackStartX, TrackUIState& ui)
{
	ProjectState& state = ProjectState::GetInstance();
	AudioTrack& track = state.tracks[trackIndex];
	TimelineBlock& instance = track.blocks[instanceIndex];
	
	float instX = (float)(trackStartX + instance.sampleBlock.startBeat * beatWidth - m_scrollX);
	float instWidth = (float)(instance.sampleBlock.lengthBeats * beatWidth);
	float instY = yPos + m_config.instancePadding;
	float instHeight = ui.height - m_config.instancePadding * 2;
	
	if (instX + instWidth < trackStartX || instX > canvasPos.x + canvasSize.x) 
		return;
	
	bool selected = IsInstanceSelected((int)trackIndex, (int)instanceIndex);
	ImVec4 baseColor = track.color;
	if (track.muted)
	{
		baseColor.x *= MUTED_COLOR_MULTIPLIER;
		baseColor.y *= MUTED_COLOR_MULTIPLIER;
		baseColor.z *= MUTED_COLOR_MULTIPLIER;
	}
	if (selected)
	{
		baseColor.x = std::min<float>(1.0f, baseColor.x * SELECTED_COLOR_MULTIPLIER);
		baseColor.y = std::min<float>(1.0f, baseColor.y * SELECTED_COLOR_MULTIPLIER);
		baseColor.z = std::min<float>(1.0f, baseColor.z * SELECTED_COLOR_MULTIPLIER);
	}
	
	ImU32 instCol = ImGui::ColorConvertFloat4ToU32(baseColor);
	ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(ImVec4(baseColor.x * BORDER_COLOR_MULTIPLIER, baseColor.y * BORDER_COLOR_MULTIPLIER, baseColor.z * BORDER_COLOR_MULTIPLIER, 1.0f));
	
	draw->AddRectFilled(ImVec2(instX, instY), ImVec2(instX + instWidth, instY + instHeight), instCol, 3.0f);
	draw->AddRect(ImVec2(instX, instY), ImVec2(instX + instWidth, instY + instHeight), borderCol, 3.0f, 0, selected ? 2.5f : 1.5f);
	
	if (instWidth > 20.0f && instHeight > 20.0f && 
		instance.sampleBlock.sampleIndex < state.samples.size())
	{
		const AudioSample& sample = state.samples[instance.sampleBlock.sampleIndex];	
		if (sample.frameData && sample.frameCount > 0)
		{
			const float secondsPerBeat = SECONDS_PER_MINUTE / state.beatsPerMinute;
			const double sampleDurationSeconds = (double)sample.frameCount / state.settings.audioSampleRate;
			const double sampleDurationBeats = sampleDurationSeconds / secondsPerBeat;
			
			const double startOffsetBeats = instance.sampleBlock.startOffsetBeats;
			const double visibleDurationBeats = instance.sampleBlock.lengthBeats;
			
			const double startOffsetSeconds = startOffsetBeats * secondsPerBeat / instance.sampleBlock.stretchScale;
			const double visibleDurationSeconds = visibleDurationBeats * secondsPerBeat / instance.sampleBlock.stretchScale;
			
			const uint64_t startFrame = (uint64_t)(startOffsetSeconds * state.settings.audioSampleRate);
			const uint64_t endFrame = std::min<uint64_t>((uint64_t)((startOffsetSeconds + visibleDurationSeconds) * state.settings.audioSampleRate), sample.frameCount);
			
			if (startFrame < sample.frameCount && endFrame > startFrame)
			{
				const float waveformY = instY + 22;
				const float waveformHeight = instHeight - 24;
				const float waveformCenter = waveformY + waveformHeight / 2.0f;
				
				const uint64_t frameCount = endFrame - startFrame;
				const int numPixels = (int)instWidth;
				
				ImU32 waveCol = IM_COL32(255, 255, 255, 180);
				ImU32 waveFillCol = IM_COL32(255, 255, 255, 40);
				
				if (frameCount < (uint64_t)numPixels * 2)
				{
					float prevX = instX;
					float prevYTop = waveformCenter;
					float prevYBottom = waveformCenter;
					
					for (int px = 0; px < numPixels; px++)
					{
						const float x = std::floor(instX + px);
						if (x < trackStartX || x > canvasPos.x + canvasSize.x) 
							continue;
						
						const double t = (double)px / numPixels;
						const uint64_t frameIdx = startFrame + (uint64_t)(t * frameCount);
						
						if (frameIdx >= sample.frameCount) 
							continue;
						
						float sampleValue = 0.0f;
						
						if (sample.channelType == SampleChannelType::Stereo)
						{
							const uint64_t offset = frameIdx * 2;
							sampleValue = (sample.frameData[offset] + sample.frameData[offset + 1]) * 0.5f;
						}
						else
						{
							sampleValue = sample.frameData[frameIdx];
						}
						
						sampleValue = std::clamp(sampleValue, -1.0f, 1.0f);
						const float yOffset = sampleValue * (waveformHeight * WAVEFORM_AMPLITUDE_SCALE);
						const float yTop = waveformCenter - std::abs(yOffset);
						const float yBottom = waveformCenter + std::abs(yOffset);
						
						if (px > 0)
						{
							draw->AddLine(ImVec2(prevX, prevYTop), ImVec2(x, yTop), waveCol, 1.0f);
							draw->AddLine(ImVec2(prevX, prevYBottom), ImVec2(x, yBottom), waveCol, 1.0f);
						}
						
						if (yBottom - yTop > 1.0f)
						{
							draw->AddLine(ImVec2(x, yTop), ImVec2(x, yBottom), waveFillCol, 1.0f);
						}
						
						prevX = x;
						prevYTop = yTop;
						prevYBottom = yBottom;
					}
				}
				else
				{
					for (int px = 0; px < numPixels; px++)
					{
						const float x = std::floor(instX + px);
						if (x < trackStartX || x > canvasPos.x + canvasSize.x) 
							continue;
						
						const double tStart = (double)px / numPixels;
						const double tEnd = (double)(px + 1) / numPixels;
						const uint64_t frameStart = startFrame + (uint64_t)(tStart * frameCount);
						const uint64_t frameEnd = std::min<uint64_t>(startFrame + (uint64_t)(tEnd * frameCount), endFrame);
						
						float minVal = 0.0f;
						float maxVal = 0.0f;
						for (uint64_t f = frameStart; f < frameEnd && f < sample.frameCount; f++)
						{
							float val = 0.0f;
							
							if (sample.channelType == SampleChannelType::Stereo)
							{
								const uint64_t offset = f * 2;
								val = (sample.frameData[offset] + sample.frameData[offset + 1]) * 0.5f;
							}
							else
							{
								val = sample.frameData[f];
							}
							
							minVal = std::min<float>(minVal, val);
							maxVal = std::max<float>(maxVal, val);
						}
						
						minVal = std::clamp(minVal, -1.0f, 1.0f);
						maxVal = std::clamp(maxVal, -1.0f, 1.0f);
						
						const float yMin = waveformCenter - (minVal * waveformHeight * WAVEFORM_AMPLITUDE_SCALE);
						const float yMax = waveformCenter - (maxVal * waveformHeight * WAVEFORM_AMPLITUDE_SCALE);
						
						if (std::abs(yMax - yMin) > 0.5f)
						{
							draw->AddLine(ImVec2(x, yMax), ImVec2(x, yMin), waveCol, 1.0f);
						}
						else
						{
							draw->AddLine(ImVec2(x, waveformCenter), ImVec2(x, waveformCenter), waveCol, 1.0f);
						}
					}
				}
				
				draw->AddLine(ImVec2(instX, waveformCenter), ImVec2(instX + instWidth, waveformCenter), IM_COL32(255, 255, 255, (int)WAVEFORM_CENTER_LINE_ALPHA), 1.0f);
			}
		}
	}
	
	ImGui::PushClipRect(ImVec2(instX, instY), ImVec2(instX + instWidth, instY + instHeight), true);
	if (instWidth > 40.0f && instHeight > 20.0f && instance.sampleBlock.sampleIndex < state.samples.size())
	{
		ImVec2 textPos(instX + 8, instY + 6);
		draw->AddText(textPos, IM_COL32(255, 255, 255, 255), state.samples[instance.sampleBlock.sampleIndex].name.c_str());
	}

	ImGui::PopClipRect();
	if (selected && instHeight > 20.0f)
	{
		draw->AddRectFilled(ImVec2(instX, instY + m_config.instancePadding), ImVec2(instX + m_config.resizeHandleWidth, instY + instHeight - m_config.instancePadding), IM_COL32(255, 255, 255, (int)RESIZE_HANDLE_ALPHA));
		draw->AddRectFilled(ImVec2(instX + instWidth - m_config.resizeHandleWidth, instY + m_config.instancePadding), ImVec2(instX + instWidth, instY + instHeight - m_config.instancePadding), IM_COL32(255, 255, 255, (int)RESIZE_HANDLE_ALPHA));
	}
}

void SongTimeline::HandleInput(ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth)
{
	ImVec2 mousePos = ImGui::GetMousePos();
	float trackStartX = canvasPos.x + m_config.trackHeaderWidth;
	float timelineStartY = canvasPos.y + m_config.rulerHeight;
	
	if (ImGui::IsMouseReleased(0))
		ResetDragState();
	
	static bool isScrubbing = false;
	if (HandleRulerClick(mousePos, canvasPos, canvasSize, trackStartX, beatWidth, isScrubbing))
		return;
	
	bool isInteracting = m_selection.isDragging || m_selection.isResizing || m_selection.isResizingLeft || m_selection.isSelecting;
	if (!isInteracting)
	{
		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
			return;

		if (HandleKeyboardShortcuts())
			return;
		if (ImGui::IsKeyPressed(ImGuiKey_Space))
		{
			ProjectState& state = ProjectState::GetInstance();
			state.isPlaying = !state.isPlaying;
			AudioEngine::StopAllNotes();
			return;
		}
	}
	
	bool inTimeline = IsInTimelineArea(mousePos, canvasPos, canvasSize, trackStartX, timelineStartY);
	if (!inTimeline && !isInteracting)
		return;
	
	double beatPos = (mousePos.x - trackStartX + m_scrollX) / beatWidth;
	int trackIdx = GetTrackAtY(mousePos.y, timelineStartY);
	
	UpdateCursor(mousePos, beatPos, trackIdx, trackStartX, beatWidth);
	
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		HandleMouseClick(mousePos, beatPos, trackIdx, trackStartX, beatWidth, ImGuiMouseButton_Left);
	else if(ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		HandleMouseClick(mousePos, beatPos, trackIdx, trackStartX, beatWidth, ImGuiMouseButton_Right);

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		HandleMouseDrag(mousePos, beatPos, trackIdx, timelineStartY, trackStartX, beatWidth);
}

void SongTimeline::ResetDragState()
{
	m_selection.isDragging = false;
	m_selection.isResizing = false;
	m_selection.isResizingLeft = false;
	m_selection.isSelecting = false;
	m_selection.draggedTrack = -1;
	m_selection.draggedInstance = -1;
	m_selection.originalPositions.clear();
}

bool SongTimeline::HandleRulerClick(ImVec2 mousePos, ImVec2 canvasPos, ImVec2 canvasSize, float trackStartX, float beatWidth, bool& isScrubbing)
{
	bool inRuler = mousePos.y >= canvasPos.y && mousePos.y < canvasPos.y + m_config.rulerHeight && mousePos.x >= trackStartX;
	if (ImGui::IsMouseClicked(0) && inRuler && ImGui::IsWindowHovered())
	{
		float visibleWidth = canvasSize.x;
        float relativeX = mousePos.x - trackStartX;
        m_config.playheadLockPosition = std::clamp(relativeX / visibleWidth, 0.0f, 1.0f);

		isScrubbing = true;
		ProjectState& state = ProjectState::GetInstance();
		state.isDraggingPlayhead = true;
		AudioEngine::StopAllNotes();
	}
	
	if (isScrubbing)
	{
		if (ImGui::IsMouseReleased(0))
		{
			isScrubbing = false;
			ProjectState& state = ProjectState::GetInstance();
			state.isDraggingPlayhead = false;
			return false;
		}
		
		double beatPos = (mousePos.x - trackStartX + m_scrollX) / beatWidth;
		ProjectState& state = ProjectState::GetInstance();
		state.timelinePositionBeats = SnapToGrid(std::max<double>(0.0, beatPos));
		return true;
	}
	
	return false;
}

bool SongTimeline::IsInTimelineArea(ImVec2 mousePos, ImVec2 canvasPos, ImVec2 canvasSize, float trackStartX, float timelineStartY)
{
	return mousePos.x >= trackStartX && 
		   mousePos.x < canvasPos.x + canvasSize.x &&
		   mousePos.y >= timelineStartY && 
		   mousePos.y < canvasPos.y + canvasSize.y;
}

void SongTimeline::UpdateCursor(ImVec2 mousePos, double beatPos, int trackIdx, float trackStartX, float beatWidth)
{
	ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
	if (m_selection.isResizing || m_selection.isResizingLeft)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		return;
	}
	
	if (m_selection.isDragging)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
		return;
	}
	
	if (m_selection.isSelecting)
		return;
	
	auto [instTrack, instIdx] = FindInstanceAt(beatPos, trackIdx);
	if (instTrack >= 0 && instIdx >= 0)
	{
		ProjectState& state = ProjectState::GetInstance();
		AudioTrack& track = state.tracks[instTrack];
		TimelineBlock& block = track.blocks[instIdx];
		
		double blockStart = 0.0;
		double blockLength = 0.0;
		if (track.type == TrackType::Midi)
		{
			blockStart = block.midiBlock.startBeat;
			blockLength = block.midiBlock.lengthBeats;
		}
		else
		{
			blockStart = block.sampleBlock.startBeat;
			blockLength = block.sampleBlock.lengthBeats;
		}
		
		float instStartX = (float)(trackStartX + blockStart * beatWidth - m_scrollX);
		float instEndX = (float)(instStartX + blockLength * beatWidth);
		ImGuiMouseCursor cursorFlag = (mousePos.x >= instStartX - 2 && mousePos.x <= instStartX + 6) || (mousePos.x >= instEndX - 6 && mousePos.x <= instEndX + 2) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_Hand; 
		ImGui::SetMouseCursor(cursorFlag);
	}
}

bool SongTimeline::HandleKeyboardShortcuts()
{
	if (ImGui::IsKeyPressed(ImGuiKey_Delete))
	{
		DeleteSelectedInstances();
		return true;
	}
	
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
	{
		SelectAllInstances();
		return true;
	}
	
	return false;
}

void SongTimeline::HandleMouseClick(ImVec2 mousePos, double beatPos, int trackIdx, float trackStartX, float beatWidth, ImGuiMouseButton btn)
{
	if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
		return;

	auto [instTrack, instIdx] = FindInstanceAt(beatPos, trackIdx);
	switch (m_currentTool)
	{
		case Tool::Draw:
			HandleDrawToolClick(instTrack, instIdx, trackIdx, beatPos, mousePos, trackStartX, beatWidth);
			break;
		case Tool::Select:
			if(btn == ImGuiMouseButton_Left)
				HandleSelectToolClick(instTrack, instIdx, trackIdx, mousePos, trackStartX, beatWidth);
			else
				HandleDeleteToolClick(instTrack, instIdx);

			break;
		case Tool::Cut:
			break;
	}
}

void SongTimeline::HandleDrawToolClick(int instTrack, int instIdx, int trackIdx, double beatPos, ImVec2 mousePos, float trackStartX, float beatWidth)
{
	ProjectState& state = ProjectState::GetInstance();
	if (instIdx == -1 && trackIdx >= 0)
	{
		if (state.tracks[trackIdx].type == TrackType::Audio && state.samples.empty())
			return;
		ClearSelection();
		CreateInstance(trackIdx, beatPos);
		
		AudioTrack& track = state.tracks[trackIdx];
		int newIdx = (int)(track.blocks.size() - 1);
		StartDragging(trackIdx, newIdx, mousePos);
		return;
	}
	
	if (instTrack >= 0 && instIdx >= 0)
	{
		if (TryStartResize(instTrack, instIdx, mousePos, trackStartX, beatWidth))
			return;
		
		bool alreadySelected = IsInstanceSelected(instTrack, instIdx);
		if (alreadySelected)
		{
			StartDragging(instTrack, instIdx, mousePos);
			StoreOriginalInstancePositions();
		}
		else
		{
			if (!ImGui::GetIO().KeyCtrl)
				ClearSelection();

			if (ImGui::GetIO().KeyCtrl)
				m_selection.selectedInstances.push_back({instTrack, instIdx});

			StartDragging(instTrack, instIdx, mousePos);
		}
	}
}

void SongTimeline::HandleSelectToolClick(int instTrack, int instIdx, int trackIdx, ImVec2 mousePos, float trackStartX, float beatWidth)
{
	if (instTrack >= 0 && instIdx >= 0)
	{
		if (TryStartResize(instTrack, instIdx, mousePos, trackStartX, beatWidth))
			return;
		
		bool alreadySelected = IsInstanceSelected(instTrack, instIdx);
		if (!ImGui::GetIO().KeyCtrl && !alreadySelected)
			ClearSelection();
		
		if (!alreadySelected)
			m_selection.selectedInstances.push_back({instTrack, instIdx});
		
		StartDragging(instTrack, instIdx, mousePos);
		StoreOriginalInstancePositions();
	}
	else
	{
		if (!ImGui::GetIO().KeyCtrl)
			ClearSelection();

		m_selection.isSelecting = true;
		m_selection.selectionStart = mousePos;
	}
}

void SongTimeline::HandleDeleteToolClick(int instTrack, int instIdx)
{
	if (instTrack >= 0 && instIdx >= 0)
	{
		ProjectState& state = ProjectState::GetInstance();
		AudioTrack& track = state.tracks[instTrack];
		track.blocks.erase(track.blocks.begin() + instIdx);
		ClearSelection();
	}
}

bool SongTimeline::TryStartResize(int trackIdx, int instIdx, ImVec2 mousePos, float trackStartX, float beatWidth)
{
	ProjectState& state = ProjectState::GetInstance();
	AudioTrack& track = state.tracks[trackIdx];
	TimelineBlock& block = track.blocks[instIdx];
	
	double blockStart = 0.0;
	double blockLength = 0.0;
	if (track.type == TrackType::Midi)
	{
		blockStart = block.midiBlock.startBeat;
		blockLength = block.midiBlock.lengthBeats;
	}
	else
	{
		blockStart = block.sampleBlock.startBeat;
		blockLength = block.sampleBlock.lengthBeats;
	}
	
	float instStartX = (float)(trackStartX + blockStart * beatWidth - m_scrollX);
	float instEndX = (float)(instStartX + blockLength * beatWidth);
	
	if (mousePos.x >= instStartX - 2 && mousePos.x <= instStartX + 6)
	{
		m_selection.isResizingLeft = true;
		m_selection.draggedTrack = trackIdx;
		m_selection.draggedInstance = instIdx;
		return true;
	}
	
	if (mousePos.x >= instEndX - 6 && mousePos.x <= instEndX + 2)
	{
		m_selection.isResizing = true;
		m_selection.draggedTrack = trackIdx;
		m_selection.draggedInstance = instIdx;
		return true;
	}
	
	return false;
}

void SongTimeline::StartDragging(int trackIdx, int instIdx, ImVec2 mousePos)
{
	m_selection.isDragging = true;
	m_selection.draggedTrack = trackIdx;
	m_selection.draggedInstance = instIdx;
	m_selection.dragStartPos = mousePos;
	
	if (m_selection.selectedInstances.empty())
	{
		m_selection.originalPositions.clear();
		ProjectState& state = ProjectState::GetInstance();
		AudioTrack& track = state.tracks[trackIdx];
		TimelineBlock& block = track.blocks[instIdx];
		double start = 0.0;
		if (track.type == TrackType::Midi)
			start = block.midiBlock.startBeat;
		else
			start = block.sampleBlock.startBeat;

		m_selection.originalPositions.push_back({start, trackIdx});
	}
}

void SongTimeline::HandleMouseDrag(ImVec2 mousePos, double beatPos, int trackIdx, float timelineStartY, float trackStartX, float beatWidth)
{
	if (m_selection.isResizing)
		HandleRightResize(beatPos);

	else if (m_selection.isResizingLeft)
		HandleLeftResize(beatPos);

	else if (m_selection.isDragging)
		HandleInstanceDrag(mousePos, trackIdx, timelineStartY, beatWidth);

	else if (m_selection.isSelecting)
		HandleBoxSelection(mousePos, timelineStartY, trackStartX, beatWidth);
}

void SongTimeline::HandleRightResize(double beatPos)
{
	ProjectState& state = ProjectState::GetInstance();
	if (m_selection.draggedTrack < 0 || m_selection.draggedInstance < 0 || m_selection.draggedTrack >= (int)state.tracks.size())
		return;
	
	AudioTrack& track = state.tracks[m_selection.draggedTrack];
	if (m_selection.draggedInstance >= (int)track.blocks.size())
		return;
	
	TimelineBlock& block = track.blocks[m_selection.draggedInstance];
	double newLength = 0.0;
	double blockStart = 0.0;
	if (track.type == TrackType::Midi)
	{
		blockStart = block.midiBlock.startBeat;
		newLength = beatPos - blockStart;
		double snappedLength = SnapToGrid(newLength);
		block.midiBlock.lengthBeats = std::max<double>(m_config.minInstanceLength, snappedLength);
	}
	else
	{
		blockStart = block.sampleBlock.startBeat;
		newLength = beatPos - blockStart;
		double snappedLength = SnapToGrid(newLength);
		
		if (block.sampleBlock.sampleIndex < state.samples.size())
		{
			const AudioSample& sample = state.samples[block.sampleBlock.sampleIndex];
			const double secondsPerBeat = SECONDS_PER_MINUTE / state.beatsPerMinute;
			const double sampleDurationSeconds = (double)sample.frameCount / state.settings.audioSampleRate;
			const double sampleDurationBeats = sampleDurationSeconds / secondsPerBeat;
			
			const double availableDuration = (sampleDurationBeats - block.sampleBlock.startOffsetBeats) * block.sampleBlock.stretchScale;
			const double maxLength = std::max<double>(m_config.minInstanceLength, availableDuration);
			
			snappedLength = std::min<double>(snappedLength, maxLength);
		}
		
		block.sampleBlock.lengthBeats = std::max<double>(m_config.minInstanceLength, snappedLength);
	}
}

void SongTimeline::HandleLeftResize(double beatPos)
{
	ProjectState& state = ProjectState::GetInstance();
	if (m_selection.draggedTrack < 0 || m_selection.draggedInstance < 0 || m_selection.draggedTrack >= (int)state.tracks.size())
		return;
	
	AudioTrack& track = state.tracks[m_selection.draggedTrack];
	if (m_selection.draggedInstance >= (int)track.blocks.size())
		return;
	
	TimelineBlock& block = track.blocks[m_selection.draggedInstance];
	if (track.type == TrackType::Midi)
	{
		double originalEnd = block.midiBlock.startBeat + block.midiBlock.lengthBeats;
		double newStart = SnapToGrid(beatPos);
		
		newStart = std::max<double>(0.0, newStart);
		newStart = std::min<double>(originalEnd - m_config.minInstanceLength, newStart);
		
		double delta = newStart - block.midiBlock.startBeat;
		double newOffset = block.midiBlock.startOffsetBeats + delta;
		
		if (newOffset < 0.0)
		{
			delta = -block.midiBlock.startOffsetBeats;
			newStart = block.midiBlock.startBeat + delta;
		}
		
		block.midiBlock.startBeat = newStart;
		block.midiBlock.lengthBeats = originalEnd - newStart;
		block.midiBlock.startOffsetBeats = std::max<double>(0.0, block.midiBlock.startOffsetBeats + delta);
	}
	else
	{
		double originalEnd = block.sampleBlock.startBeat + block.sampleBlock.lengthBeats;
		double newStart = SnapToGrid(beatPos);
		newStart = std::max<double>(0.0, newStart);
		newStart = std::min<double>(originalEnd - m_config.minInstanceLength, newStart);
		
		double delta = newStart - block.sampleBlock.startBeat;
		double newOffset = block.sampleBlock.startOffsetBeats + delta;
		if (newOffset < 0.0)
		{
			delta = -block.sampleBlock.startOffsetBeats;
			newStart = block.sampleBlock.startBeat + delta;
		}
		
		block.sampleBlock.startBeat = newStart;
		block.sampleBlock.lengthBeats = originalEnd - newStart;
		block.sampleBlock.startOffsetBeats = std::max<double>(0.0, block.sampleBlock.startOffsetBeats + delta);
	}
}

void SongTimeline::HandleInstanceDrag(ImVec2 mousePos, int hoverTrack, float timelineStartY, float beatWidth)
{
	ImVec2 delta = ImVec2(mousePos.x - m_selection.dragStartPos.x, mousePos.y - m_selection.dragStartPos.y);
	double deltaBeat = delta.x / beatWidth;
	
	if (m_selection.selectedInstances.empty())
	{
		HandleSingleInstanceDrag(hoverTrack, deltaBeat, timelineStartY);
		return;
	}
	
	HandleMultiInstanceDrag(hoverTrack, deltaBeat, timelineStartY);
}

void SongTimeline::HandleSingleInstanceDrag(int hoverTrack, double deltaBeat, float timelineStartY)
{
	ProjectState& state = ProjectState::GetInstance();
	if (m_selection.draggedTrack < 0 || m_selection.draggedInstance < 0)
	{
		return;
	}
	
	if (hoverTrack >= 0 && hoverTrack != m_selection.draggedTrack)
	{
		AudioTrack& srcTrack = state.tracks[m_selection.draggedTrack];
		AudioTrack& dstTrack = state.tracks[hoverTrack];
		
		if (srcTrack.type == dstTrack.type && m_selection.draggedInstance < (int)srcTrack.blocks.size())
		{
			TimelineBlock block = srcTrack.blocks[m_selection.draggedInstance];
			srcTrack.blocks.erase(srcTrack.blocks.begin() + m_selection.draggedInstance);
			dstTrack.blocks.push_back(block);
			m_selection.draggedTrack = hoverTrack;
			m_selection.draggedInstance = (int)(dstTrack.blocks.size() - 1);
			m_selection.originalPositions[0].trackIndex = hoverTrack;
		}
	}
	
	if (!m_selection.originalPositions.empty())
	{
		double newStart = m_selection.originalPositions[0].startBeat + deltaBeat;
		newStart = SnapToGrid(std::max<double>(0.0, newStart));
		
		AudioTrack& track = state.tracks[m_selection.draggedTrack];
		if (m_selection.draggedInstance < (int)track.blocks.size())
		{
			if (track.type == TrackType::Midi)
				track.blocks[m_selection.draggedInstance].midiBlock.startBeat = newStart;
			else
				track.blocks[m_selection.draggedInstance].sampleBlock.startBeat = newStart;
		}
	}
}

void SongTimeline::HandleMultiInstanceDrag(int hoverTrack, double deltaBeat, float timelineStartY)
{
	TryMoveSelectionToTrack(hoverTrack);
	
	double minOriginalStart = DBL_MAX;
	for (const auto& orig : m_selection.originalPositions)
	{
		minOriginalStart = std::min<double>(minOriginalStart, orig.startBeat);
	}
	
	if (minOriginalStart + deltaBeat < 0.0)
		deltaBeat = -minOriginalStart;
	
	ProjectState& state = ProjectState::GetInstance();
	for (size_t i = 0; i < m_selection.selectedInstances.size(); i++)
	{
		if (i >= m_selection.originalPositions.size()) 
			continue;
		
		const auto& [tIdx, iIdx] = m_selection.selectedInstances[i];
		AudioTrack& track = state.tracks[tIdx];
		if (iIdx >= (int)track.blocks.size()) 
			continue;
		
		double newStart = m_selection.originalPositions[i].startBeat + deltaBeat;
		newStart = SnapToGrid(std::max<double>(0.0, newStart));
		
		if (track.type == TrackType::Midi)
			track.blocks[iIdx].midiBlock.startBeat = newStart;
		else
			track.blocks[iIdx].sampleBlock.startBeat = newStart;
	}
}

void SongTimeline::TryMoveSelectionToTrack(int hoverTrack)
{
	if (hoverTrack < 0 || m_selection.selectedInstances.empty())
		return;
	
	int firstTrack = m_selection.selectedInstances[0].first;
	for (const auto& [tIdx, _] : m_selection.selectedInstances)
	{
		if (tIdx != firstTrack)
			return;
	}
	
	if (hoverTrack == firstTrack)
		return;
	
	ProjectState& state = ProjectState::GetInstance();
	AudioTrack& srcTrack = state.tracks[firstTrack];
	AudioTrack& dstTrack = state.tracks[hoverTrack];
	
	if (srcTrack.type != dstTrack.type)
		return;
	
	std::vector<std::pair<int, int>> sorted = m_selection.selectedInstances;
	std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
	
	std::vector<std::pair<int, int>> newSelection;
	for (const auto& [tIdx, iIdx] : sorted)
	{
		if (iIdx >= (int)srcTrack.blocks.size()) 
			continue;
		
		TimelineBlock block = srcTrack.blocks[iIdx];
		srcTrack.blocks.erase(srcTrack.blocks.begin() + iIdx);
		dstTrack.blocks.push_back(block);
		newSelection.push_back({hoverTrack, (int)dstTrack.blocks.size() - 1});
	}
	
	m_selection.selectedInstances = newSelection;
	m_selection.draggedTrack = hoverTrack;
	for (auto& orig : m_selection.originalPositions)
	{
		orig.trackIndex = hoverTrack;
	}
}

void SongTimeline::HandleBoxSelection(ImVec2 mousePos, float timelineStartY, float trackStartX, float beatWidth)
{
	m_selection.selectionEnd = mousePos;
	
	ImVec2 boxMin(std::min<float>(m_selection.selectionStart.x, m_selection.selectionEnd.x), std::min<float>(m_selection.selectionStart.y, m_selection.selectionEnd.y));
	ImVec2 boxMax(std::max<float>(m_selection.selectionStart.x, m_selection.selectionEnd.x), std::max<float>(m_selection.selectionStart.y, m_selection.selectionEnd.y));
	
	float currentY = timelineStartY - m_scrollY;
	ProjectState& state = ProjectState::GetInstance();
	
	for (size_t t = 0; t < state.tracks.size(); t++)
	{
		AudioTrack& track = state.tracks[t];
		float trackHeight = m_trackUIStates[t].height;
		
		if (currentY + trackHeight < boxMin.y || currentY > boxMax.y)
		{
			currentY += trackHeight;
			continue;
		}
		
		for (size_t i = 0; i < track.blocks.size(); i++)
		{
			const TimelineBlock& block = track.blocks[i];
			
			double blockStart = 0.0;
			double blockLength = 0.0;
			if (track.type == TrackType::Midi)
			{
				blockStart = block.midiBlock.startBeat;
				blockLength = block.midiBlock.lengthBeats;
			}
			else
			{
				blockStart = block.sampleBlock.startBeat;
				blockLength = block.sampleBlock.lengthBeats;
			}
			
			float instX1 = (float)(trackStartX + blockStart * beatWidth - m_scrollX);
			float instX2 = (float)(instX1 + blockLength * beatWidth);
			float instY = currentY + m_config.instancePadding;
			float instHeight = trackHeight - m_config.instancePadding * 2;
			
			bool inBox = instX2 >= boxMin.x && instX1 <= boxMax.x && instY + instHeight >= boxMin.y && instY <= boxMax.y;
			
			if (inBox && !IsInstanceSelected((int)t, (int)i))
				m_selection.selectedInstances.push_back({(int)t, (int)i});
		}
		
		currentY += trackHeight;
	}
}

void SongTimeline::SelectAllInstances()
{
	m_selection.selectedInstances.clear();
	ProjectState& state = ProjectState::GetInstance();
	for (size_t t = 0; t < state.tracks.size(); t++)
	{
		AudioTrack& track = state.tracks[t];
		for (size_t i = 0; i < track.blocks.size(); i++)
		{
			m_selection.selectedInstances.push_back({(int)t, (int)i});
		}
	}
}

void SongTimeline::CreateInstance(int trackIdx, double beatPos)
{
	ProjectState& state = ProjectState::GetInstance();
	if (trackIdx < 0 || trackIdx >= (int)state.tracks.size()) 
		return;
	
	double snappedStart = SnapToGrid(beatPos);
	AudioTrack& track = state.tracks[trackIdx];
	
	if (track.type == TrackType::Midi)
	{
		TimelineBlock instance;
		instance.midiBlock.startBeat = snappedStart;
		instance.midiBlock.lengthBeats = m_config.defaultInstanceLength;
		instance.midiBlock.startOffsetBeats = 0.0;
		instance.midiBlock.patternIndex = state.currentMidiPatternIndex;
		track.blocks.push_back(instance);
	}
	else
	{
		TimelineBlock instance;
		instance.sampleBlock.startBeat = snappedStart;
		instance.sampleBlock.lengthBeats = m_config.defaultInstanceLength;
		instance.sampleBlock.startOffsetBeats = 0.0;
		instance.sampleBlock.stretchScale = 1.0;
		instance.sampleBlock.sampleIndex = 0;
		track.blocks.push_back(instance);
	}
}

std::pair<int, int> SongTimeline::FindInstanceAt(double beatPos, int trackIdx)
{
	ProjectState& state = ProjectState::GetInstance();
	if (trackIdx < 0 || trackIdx >= (int)state.tracks.size())
		return {-1, -1};
	
	AudioTrack& track = state.tracks[trackIdx];
	
	for (int i = (int)track.blocks.size() - 1; i >= 0; i--)
	{
		const TimelineBlock& inst = track.blocks[i];
		double blockStart = 0.0;
		double blockLength = 0.0;
		
		if (track.type == TrackType::Midi)
		{
			blockStart = inst.midiBlock.startBeat;
			blockLength = inst.midiBlock.lengthBeats;
		}
		else
		{
			blockStart = inst.sampleBlock.startBeat;
			blockLength = inst.sampleBlock.lengthBeats;
		}
		
		if (beatPos >= blockStart && beatPos <= blockStart + blockLength)
		{
			return {trackIdx, i};
		}
	}

	return {-1, -1};
}

int SongTimeline::GetTrackAtY(float mouseY, float timelineStartY)
{
	float currentY = timelineStartY - m_scrollY;
	ProjectState& state = ProjectState::GetInstance();
	for (size_t i = 0; i < state.tracks.size(); i++)
	{
		if (mouseY >= currentY && mouseY < currentY + m_trackUIStates[i].height)
			return (int)i;

		currentY += m_trackUIStates[i].height;
	}

	return -1;
}

double SongTimeline::SnapToGrid(double value)
{
	if (m_snap == 0) 
		return value;

	double snapSize = BEATS_PER_MEASURE / m_snap;
	return std::round(value / snapSize) * snapSize;
}

void SongTimeline::StoreOriginalInstancePositions()
{
	m_selection.originalPositions.clear();
	ProjectState& state = ProjectState::GetInstance();
	for (const auto& [tIdx, iIdx] : m_selection.selectedInstances)
	{
		AudioTrack& track = state.tracks[tIdx];
		double start = 0.0;
		if (track.type == TrackType::Midi && iIdx < (int)track.blocks.size())
			start = track.blocks[iIdx].midiBlock.startBeat;
		else if (track.type == TrackType::Audio && iIdx < (int)track.blocks.size())
			start = track.blocks[iIdx].sampleBlock.startBeat;
		
		m_selection.originalPositions.push_back({start, tIdx});
	}
}

void SongTimeline::ClearSelection()
{
	m_selection.selectedInstances.clear();
}

bool SongTimeline::IsInstanceSelected(int trackIdx, int instanceIdx)
{
	for (const auto& [tIdx, iIdx] : m_selection.selectedInstances)
	{
		if (tIdx == trackIdx && iIdx == instanceIdx)
			return true;
	}

	return false;
}

void SongTimeline::DeleteSelectedInstances()
{
	std::vector<std::pair<int, int>> sorted = m_selection.selectedInstances;
	std::sort(sorted.rbegin(), sorted.rend());
	
	ProjectState& state = ProjectState::GetInstance();
	for (const auto& [tIdx, iIdx] : sorted)
	{
		if (tIdx >= 0 && tIdx < (int)state.tracks.size())
		{
			AudioTrack& track = state.tracks[tIdx];
			if (iIdx < (int)track.blocks.size())
				track.blocks.erase(track.blocks.begin() + iIdx);
		}
	}
	m_selection.selectedInstances.clear();
}

const char* SongTimeline::GetSnapName(int snap)
{
	switch (snap)
	{
		case 0: return "Off";
		case 4: return "1/4";
		case 8: return "1/8";
		case 16: return "1/16";
		case 32: return "1/32";
		default: return "Unknown";
	}
}

