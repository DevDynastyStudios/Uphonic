#include "panel_manager.h"
#include "imgui_internal.h"
#include "types.h"

#include <algorithm>
#include <cmath>

enum class ResizeSide
{
    Left,
    Right
};

struct UphSongTimeline
{
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    float zoomX  = 16.0f;
    float zoomY  = 1.0f;

    float song_position = 0.0f;

    // Drag state
    bool dragging = false;
    bool resizing = false;
    bool is_playing = false;
    ResizeSide resizeSide;
    UphMidiPatternInstance* draggedPattern = nullptr;
    UphTrack* draggedTrack = nullptr;
    float dragOffset = 0.0f;
};

static UphSongTimeline timeline_data {};

static constexpr float k_track_height           = 60.0f;
static constexpr float k_beat_size              = 1.0f;
static constexpr float k_track_menu_width       = 150.0f;
static constexpr float k_resize_handle_width    = 8.0f;
static constexpr float k_min_pattern_length     = 8.0f;

static float quantizeToBeat(float time, float beatSize)
{
    return std::round(time / beatSize) * beatSize;
}

static void uph_song_timeline_handle_pattern_interaction(
    UphTrack& track,
    int trackIndex,
    UphMidiPatternInstance& pattern,
    ImVec2 canvasPos,
    float timelineX,
    ImVec2 canvasSize,
    ImVec2 patternRectMin,
    ImVec2 patternRectMax)
{
    ImGuiIO& io = ImGui::GetIO();

    float px = patternRectMin.x;
    float pw = patternRectMax.x - patternRectMin.x;
    float y  = patternRectMin.y;
    float h  = patternRectMax.y - patternRectMin.y;

    bool isHovered = ImGui::IsMouseHoveringRect(patternRectMin, patternRectMax) && ImGui::IsWindowHovered();
    bool isActive  = (timeline_data.draggedPattern == &pattern);

    ImVec2 leftHandleMin (px - k_resize_handle_width, y);
    ImVec2 leftHandleMax (px + k_resize_handle_width, y + h);
    ImVec2 rightHandleMin(px + pw - k_resize_handle_width, y);
    ImVec2 rightHandleMax(px + pw + k_resize_handle_width, y + h);

    bool hoverLeft  = ImGui::IsMouseHoveringRect(leftHandleMin,  leftHandleMax);
    bool hoverRight = ImGui::IsMouseHoveringRect(rightHandleMin, rightHandleMax);

    if (!timeline_data.dragging && !timeline_data.resizing)
    {
        if (hoverLeft && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            timeline_data.resizing = true;
            timeline_data.resizeSide = ResizeSide::Left;
            timeline_data.draggedPattern = &pattern;
            timeline_data.draggedTrack = &track;
            ImGui::SetActiveID(ImGui::GetID(&pattern), ImGui::GetCurrentWindow());
        }
        else if (hoverRight && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            timeline_data.resizing = true;
            timeline_data.resizeSide = ResizeSide::Right;
            timeline_data.draggedPattern = &pattern;
            timeline_data.draggedTrack = &track;
            ImGui::SetActiveID(ImGui::GetID(&pattern), ImGui::GetCurrentWindow());
        }
        else if (isHovered)
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                app->current_pattern_index = pattern.pattern_index;
                app->current_track_index = trackIndex;
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                timeline_data.dragging = true;
                timeline_data.draggedPattern = &pattern;
                timeline_data.draggedTrack = &track;
                timeline_data.dragOffset = (io.MousePos.x - px) / timeline_data.zoomX;
                ImGui::SetActiveID(ImGui::GetID(&pattern), ImGui::GetCurrentWindow());
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                track.midi_track.pattern_instances.erase(
                    std::remove_if(track.midi_track.pattern_instances.begin(),
                        track.midi_track.pattern_instances.end(),
                        [&](UphMidiPatternInstance &p){ return &p == &pattern; }),
                    track.midi_track.pattern_instances.end());
            }
        }
    }

    if (timeline_data.resizing && isActive)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            float mouseTime = (io.MousePos.x - timelineX + timeline_data.scrollX) / timeline_data.zoomX;
            float qMouseTime = quantizeToBeat(mouseTime, k_beat_size);

            if (timeline_data.resizeSide == ResizeSide::Left)
            {
                float oldStart = pattern.start_time;
                float oldLen   = pattern.length;
                float maxStart = oldStart + oldLen - k_min_pattern_length;
                float newStart = std::clamp(qMouseTime, 0.0f, maxStart);

                float delta = newStart - oldStart;
                pattern.start_time = newStart;
                pattern.length     = oldLen - delta;

                if (pattern.length < k_min_pattern_length) {
                    pattern.length = k_min_pattern_length;
                    pattern.start_time = oldStart + (oldLen - pattern.length);
                }

                pattern.start_offset += newStart - oldStart;
            }
            else
            {
                float start  = pattern.start_time;
                float newLen = std::max(k_min_pattern_length, qMouseTime - start);
                pattern.length = newLen;
            }
        }
        else {
            timeline_data.resizing = false;
            timeline_data.draggedPattern = nullptr;
            timeline_data.draggedTrack   = nullptr;
            ImGui::ClearActiveID();
        }
    }

    if (timeline_data.dragging && isActive)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            float newStart = (io.MousePos.x - timelineX + timeline_data.scrollX) / timeline_data.zoomX - timeline_data.dragOffset;
            newStart = quantizeToBeat(newStart, k_beat_size);
            newStart = std::max(0.0f, newStart);
            pattern.start_time = newStart;

            int targetTrackIdx = (int)((io.MousePos.y + timeline_data.scrollY - canvasPos.y) / (k_track_height * timeline_data.zoomY));
            auto& tracks = app->project.tracks;
            if (targetTrackIdx >= 0 && targetTrackIdx < (int)tracks.size())
            {
                UphTrack &targetTrack = tracks[targetTrackIdx];
                if (&targetTrack != timeline_data.draggedTrack && targetTrack.track_type == UphTrackType::Midi)
                {
                    auto& src = timeline_data.draggedTrack->midi_track.pattern_instances;

                    auto it = std::find_if(src.begin(), src.end(),
                        [&](UphMidiPatternInstance &p){ return &p == timeline_data.draggedPattern; });

                    if (it != src.end())
                    {
                        UphMidiPatternInstance moved = std::move(*it);
                        src.erase(it);

                        targetTrack.midi_track.pattern_instances.push_back(std::move(moved));
                        timeline_data.draggedTrack   = &targetTrack;
                        timeline_data.draggedPattern = &targetTrack.midi_track.pattern_instances.back();
                    }
                }
            }
        }
        else {
            timeline_data.dragging = false;
            timeline_data.draggedPattern = nullptr;
            timeline_data.draggedTrack   = nullptr;
            ImGui::ClearActiveID();
        }
    }

    if (hoverLeft || hoverRight || timeline_data.resizing)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    else if (isHovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
}

static void uph_song_timeline_draw_pattern_block(
    ImDrawList* drawList,
    const UphMidiPatternInstance& pattern,
    const UphMidiPattern& patternData,
    ImVec2 rectMin, ImVec2 rectMax,
    float titleHeight, float zoomX, float zoomY
)
{
    bool active = false;

    ImU32 borderCol = active ? IM_COL32(200, 200, 50, 255) : IM_COL32(0, 0, 0, 255);
    ImU32 fillCol   = IM_COL32(240, 117, 138, 255);
    ImU32 textCol   = IM_COL32(0, 0, 0, 255);

    drawList->AddRect(rectMin, rectMax, borderCol);
    drawList->AddRectFilled(rectMin, rectMax, fillCol);

    drawList->PushClipRect(ImVec2(rectMin.x, rectMin.y), ImVec2(rectMax.x, rectMin.y + titleHeight), true);
    drawList->AddText(rectMin, textCol, patternData.name);
    drawList->PopClipRect();

    float py = rectMin.y + titleHeight;
    drawList->AddLine(ImVec2(rectMin.x, py), ImVec2(rectMax.x, py), IM_COL32(0, 0, 0, 255));

    auto& notes = patternData.notes;
    if (notes.empty()) return;

    int minKey = 127, maxKey = 0;
    for (auto& note : notes)
    {
        minKey = std::min<int>(minKey, note.key);
        maxKey = std::max<int>(maxKey, note.key);
    }

    int keyRange = std::max(1, maxKey - minKey + 1);
    float ph = (rectMax.y - rectMin.y) - titleHeight;
    float note_h = ph / (float)keyRange;

    drawList->PushClipRect(ImVec2(rectMin.x, py), rectMax, true);

    for (auto& note : notes)
    {
        float note_x = rectMin.x + (note.start - pattern.start_offset) * zoomX;
        float note_w = note.length * zoomX;

        int relativeKey = note.key - minKey;
        float note_y = py + ph - (relativeKey + 1) * note_h;

        drawList->AddRectFilled(ImVec2(note_x, note_y), ImVec2(note_x + note_w, note_y + note_h),
            textCol);
    }

    drawList->PopClipRect();
}

static void uph_song_timeline_draw_track_menu(UphTrack& track, size_t trackIndex, float y, float h, ImVec2 canvasPos)
{
    ImVec2 menuPos(canvasPos.x, y);
    ImGui::SetCursorScreenPos(menuPos);
    ImGui::PushID((int)trackIndex);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("TrackMenuRow", ImVec2(k_track_menu_width, h), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::Text("Track %d", (int)trackIndex); // replace with track.name if available

    ImGui::Checkbox("Mute", &track.muted);
    ImGui::Button("...");

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopID();
}

static void uph_song_timeline_draw_and_handle_playhead(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float keyWidth)
{
    const float x = canvasPos.x + keyWidth + timeline_data.song_position * timeline_data.zoomX - timeline_data.scrollX;
    drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasSize.y),
        IM_COL32(0, 255, 0, 255), 2.0f);
}

static void uph_song_timeline_render(UphPanel* panel)
{
    auto& tracks = app->project.tracks;

    ImVec2 fullSize = ImGui::GetContentRegionAvail();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    if (ImGui::Button(timeline_data.is_playing ? "Stop" : "Play"))
    {
        timeline_data.is_playing = !timeline_data.is_playing;
        timeline_data.song_position = 0.0f;
        /*if (!timeline_data.is_playing)
            for (auto &track : tracks)
            {
                if (track.track_type == UphTrackType::Midi && track.midi_track.instrument.effect)
                    effect_all_notes_off(track.midi_track.instrument.effect);
            }*/
    }
    ImGui::LabelText("Song Position", "%f", timeline_data.song_position);

    ImGui::BeginChild("SongTimelineCanvas", fullSize, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const float titleHeight = ImGui::GetTextLineHeight();

    if (ImGui::IsWindowHovered())
    {
        if (io.KeyCtrl)
        {
            timeline_data.zoomX *= (1.0f + io.MouseWheel * 0.1f);
            timeline_data.zoomX = std::clamp(timeline_data.zoomX, 4.0f, 128.0f);
        }
        else
        {
            timeline_data.scrollY -= io.MouseWheel * 20.0f;
        }
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            timeline_data.scrollX -= io.MouseDelta.x;
            timeline_data.scrollY -= io.MouseDelta.y;
        }

        timeline_data.scrollX = std::max(0.0f, timeline_data.scrollX);
        timeline_data.scrollY = std::max(0.0f, timeline_data.scrollY);
    }

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        UphTrack& track = tracks[i];
        float y = canvasPos.y + i * k_track_height * timeline_data.zoomY - timeline_data.scrollY;
        float h = k_track_height * timeline_data.zoomY;
        float timelineX = canvasPos.x + k_track_menu_width;

        if (track.track_type == UphTrackType::Midi)
        {
            for (auto& pattern : track.midi_track.pattern_instances)
            {
                float px = timelineX + pattern.start_time * timeline_data.zoomX - timeline_data.scrollX;
                float pw = pattern.length * timeline_data.zoomX;

                ImVec2 rectMin(px, y);
                ImVec2 rectMax(px + pw, y + h);

                uph_song_timeline_handle_pattern_interaction(track, i, pattern, canvasPos, timelineX, canvasSize, rectMin, rectMax);
            }
        }
    
        ImVec2 trackMin(canvasPos.x + k_track_menu_width + 4, y);
        ImVec2 trackMax(canvasPos.x + canvasSize.x, y + h);

        char buf[64];
        snprintf(buf, sizeof(buf), "Track %zu Drop Zone", i);
        ImGui::SetCursorScreenPos(trackMin);
        ImGui::InvisibleButton(buf, ImVec2(canvasSize.x, h));

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PATTERN"))
            {
                size_t src_index = *(const size_t*)payload->Data;
                if (track.track_type == UphTrackType::Midi)
                {
                    UphMidiPatternInstance newInstance;
                    newInstance.pattern_index = (uint16_t)src_index;

                    float localX = io.MousePos.x - canvasPos.x - k_track_menu_width + timeline_data.scrollX;
                    newInstance.start_time = quantizeToBeat(localX / timeline_data.zoomX, k_beat_size);

                    newInstance.start_offset = 0.0f;
                    newInstance.length = 8.0f;

                    track.midi_track.pattern_instances.push_back(newInstance);
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        UphTrack& track = tracks[i];
        float y = canvasPos.y + i * k_track_height * timeline_data.zoomY - timeline_data.scrollY;
        float h = k_track_height * timeline_data.zoomY;

        drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + canvasSize.x, y),
            IM_COL32(100, 100, 100, 255));

        float timelineX = canvasPos.x + k_track_menu_width;

        if (track.track_type == UphTrackType::Midi)
        {
            for (auto& pattern : track.midi_track.pattern_instances)
            {
                const UphMidiPattern& patternData = app->project.patterns[pattern.pattern_index];
                float px = timelineX + pattern.start_time * timeline_data.zoomX - timeline_data.scrollX;
                float pw = pattern.length * timeline_data.zoomX;
                ImVec2 rectMin(px, y);
                ImVec2 rectMax(px + pw, y + h);

                uph_song_timeline_draw_pattern_block(drawList, pattern, patternData, rectMin, rectMax,
                    titleHeight, timeline_data.zoomX, timeline_data.zoomY);
            }
        }
        uph_song_timeline_draw_track_menu(track, i, y, h, canvasPos);
    }

    if (timeline_data.is_playing)
        uph_song_timeline_draw_and_handle_playhead(drawList, canvasPos, canvasSize, k_track_menu_width);

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

UPH_REGISTER_PANEL("Song Timeline", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse, ImGuiDockNodeFlags_None, uph_song_timeline_render);