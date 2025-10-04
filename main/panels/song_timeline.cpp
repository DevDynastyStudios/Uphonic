#include "panel_manager.h"
#include "sound_device.h"
#include "types.h"

#include "utils/lerp.h"

#include <imgui_internal.h>
#include <font_awesome.h>

#include <algorithm>
#include <cmath>

enum class ResizeSide
{
    Left,
    Right
};

struct UphSongTimeline
{
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    float smooth_scroll_y = 0.0f;
    float zoom_x  = 16.0f;
    float zoom_y  = 1.0f;

    bool dragging = false;
    bool resizing = false;
    ResizeSide resizeSide;
    UphTimelineBlock* draggedBlock = nullptr;
    UphTrack* draggedTrack = nullptr;
    float dragOffset = 0.0f;
};

static UphSongTimeline timeline_data {};

static constexpr float k_track_height           = 60.0f;
static constexpr float k_beat_size              = 1.0f;
static constexpr float k_track_menu_width       = 150.0f;
static constexpr float k_resize_handle_width    = 8.0f;
static constexpr float k_min_pattern_length     = 1.0f;

static void uph_song_timeline_init(UphPanel* panel)
{
	panel->window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
}

static float quantizeToBeat(float time, float beatSize)
{
    return std::round(time / beatSize) * beatSize;
}

static void uph_song_timeline_handle_pattern_interaction(
    UphTrack& track,
    int trackIndex,
    UphTimelineBlock& pattern,
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
    bool isActive  = (timeline_data.draggedBlock == &pattern);

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
            timeline_data.draggedBlock = &pattern;
            timeline_data.draggedTrack = &track;
            ImGui::SetActiveID(ImGui::GetID(&pattern), ImGui::GetCurrentWindow());
        }
        else if (hoverRight && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            timeline_data.resizing = true;
            timeline_data.resizeSide = ResizeSide::Right;
            timeline_data.draggedBlock = &pattern;
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
                timeline_data.draggedBlock = &pattern;
                timeline_data.draggedTrack = &track;
                timeline_data.dragOffset = (io.MousePos.x - px) / timeline_data.zoom_x;
                ImGui::SetActiveID(ImGui::GetID(&pattern), ImGui::GetCurrentWindow());
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                track.timeline_blocks.erase(
                    std::remove_if(track.timeline_blocks.begin(),
                        track.timeline_blocks.end(),
                        [&](UphTimelineBlock &p){ return &p == &pattern; }),
                    track.timeline_blocks.end());
            }
        }
    }

    if (timeline_data.resizing && isActive)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            float mouseTime = (io.MousePos.x - timelineX + timeline_data.scroll_x) / timeline_data.zoom_x;
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
            timeline_data.draggedBlock = nullptr;
            timeline_data.draggedTrack   = nullptr;
            ImGui::ClearActiveID();
        }
    }

    if (timeline_data.dragging && isActive)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            float newStart = (io.MousePos.x - timelineX + timeline_data.scroll_x) / timeline_data.zoom_x - timeline_data.dragOffset;
            newStart = quantizeToBeat(newStart, k_beat_size);
            newStart = std::max(0.0f, newStart);
            pattern.start_time = newStart;

            int targetTrackIdx = (int)((io.MousePos.y + timeline_data.smooth_scroll_y - canvasPos.y) / (k_track_height * timeline_data.zoom_y));
            auto& tracks = app->project.tracks;
            if (targetTrackIdx >= 0 && targetTrackIdx < (int)tracks.size())
            {
                UphTrack &targetTrack = tracks[targetTrackIdx];
                if (&targetTrack != timeline_data.draggedTrack && targetTrack.track_type == timeline_data.draggedBlock->track_type)
                {
                    auto& src = timeline_data.draggedTrack->timeline_blocks;

                    auto it = std::find_if(src.begin(), src.end(),
                        [&](UphTimelineBlock &p){ return &p == timeline_data.draggedBlock; });

                    if (it != src.end())
                    {
                        UphTimelineBlock moved = std::move(*it);
                        src.erase(it);

                        targetTrack.timeline_blocks.push_back(std::move(moved));
                        timeline_data.draggedTrack   = &targetTrack;
                        timeline_data.draggedBlock = &targetTrack.timeline_blocks.back();
                    }
                }
            }
        }
        else {
            timeline_data.dragging = false;
            timeline_data.draggedBlock = nullptr;
            timeline_data.draggedTrack   = nullptr;
            ImGui::ClearActiveID();
        }
    }

    if (hoverLeft || hoverRight || timeline_data.resizing)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    else if (isActive || isHovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
}

static void uph_song_timeline_draw_block(ImDrawList* drawList, ImVec2 rectMin, ImVec2 rectMax, float py, ImU32 fill_color, const char *name)
{
    constexpr ImU32 textCol = IM_COL32(0, 0, 0, 255);

    drawList->AddRectFilled(rectMin, rectMax, fill_color);
    //drawList->AddRect(rectMin, rectMax, borderCol);

    drawList->PushClipRect(ImVec2(rectMin.x, rectMin.y), ImVec2(rectMax.x, py), true);
    drawList->AddText(rectMin, textCol, name);
    drawList->PopClipRect();

    drawList->AddLine(ImVec2(rectMin.x, py), ImVec2(rectMax.x, py), IM_COL32(0, 0, 0, 255));
}

static void uph_song_timeline_draw_pattern_block(
    ImDrawList* drawList,
    const UphTimelineBlock& block,
    const UphMidiPattern& patternData,
    ImVec2 rectMin, ImVec2 rectMax,
    ImU32 fill_color,
    float title_height, float zoomX, float zoomY
)
{
    constexpr ImU32 textCol = IM_COL32(0, 0, 0, 255);

    const float py = rectMin.y + title_height;
    uph_song_timeline_draw_block(drawList, rectMin, rectMax, py, fill_color, patternData.name);

    auto& notes = patternData.notes;
    if (notes.empty()) return;

    int minKey = 127, maxKey = 0;
    for (auto& note : notes)
    {
        minKey = std::min<int>(minKey, note.key);
        maxKey = std::max<int>(maxKey, note.key);
    }

    int keyRange = std::max(1, maxKey - minKey + 1);
    float ph = (rectMax.y - rectMin.y) - title_height;
    float note_h = ph / (float)keyRange;

    drawList->PushClipRect(ImVec2(rectMin.x, py), rectMax, true);

    for (auto& note : notes)
    {
        float note_x = rectMin.x + (note.start - block.start_offset) * zoomX;
        float note_w = note.length * zoomX;

        int relativeKey = note.key - minKey;
        float note_y = py + ph - (relativeKey + 1) * note_h;

        drawList->AddRectFilled(ImVec2(note_x, note_y), ImVec2(note_x + note_w, note_y + note_h),
            textCol);
    }

    drawList->PopClipRect();
}

static void uph_song_timeline_draw_sample_block(
    ImDrawList* drawList,
    const UphTimelineBlock& sample,
    const UphSample& sample_data,
    ImVec2 rectMin, ImVec2 rectMax,
    ImU32 fill_color,
    float title_height, float zoomX, float zoomY
)
{
    const float py = rectMin.y + title_height;
    uph_song_timeline_draw_block(drawList, rectMin, rectMax, py, fill_color, sample_data.name);

    if (!sample_data.frames || sample_data.frame_count == 0) return;

    const uint8_t stride = sample_data.type == UphSampleType_Stereo ? 2 : 1;

    const float scale_x = 1.0f / (60.0f * sample_data.sample_rate) * zoomX * sample.stretch_scale * app->project.bpm;
    const float frames_per_pixel = 1.0f / scale_x;

    ImGui::PushClipRect(ImVec2(rectMin.x, py + 1), rectMax, true);

    drawList->AddLine(ImVec2(rectMin.x, rectMin.y + (title_height + k_track_height * zoomY) * 0.5f), ImVec2(rectMax.x, rectMin.y + (title_height + k_track_height * zoomY) * 0.5f), IM_COL32(0, 0, 0, 255));

    for (float px = rectMin.x; px < rectMax.x; px += 1.0f)
    {
        const float frame_f = (px - rectMin.x + sample.start_offset * zoomX) / scale_x;
        uint64_t start_frame = (uint64_t)std::max<int64_t>(0, (int64_t)frame_f);
        uint64_t end_frame   = std::min<uint64_t>(sample_data.frame_count, start_frame + (uint64_t)frames_per_pixel);

        if (start_frame >= end_frame)
            continue;

        float min_val = 1.0f, max_val = -1.0f;
        for (uint64_t i = start_frame; i < end_frame; i += stride)
        {
            float v = sample_data.frames[i * stride] * 0.5f;
            min_val = std::min(min_val, v);
            max_val = std::max(max_val, v);
        }

        drawList->AddLine(
            ImVec2(px, rectMax.y - k_track_height * zoomY * max_val - zoomY * k_track_height * 0.5f + title_height * 0.5f),
            ImVec2(px, rectMax.y - k_track_height * zoomY * min_val - zoomY * k_track_height * 0.5f + title_height * 0.5f),
            IM_COL32(0, 0, max_val * 255, 255)
        );
    }

    ImGui::PopClipRect();
}

static void uph_song_timeline_draw_track_menu(UphTrack& track, size_t trackIndex, float y, float h, ImVec2 canvasPos)
{
    ImVec2 menuPos(canvasPos.x, y);
    ImGui::SetCursorScreenPos(menuPos);
    ImGui::PushID((int)trackIndex);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("TrackMenuRow", ImVec2(k_track_menu_width, h + 1), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::Text("Track %d", (int)trackIndex); // replace with track.name if available

    ImGui::Checkbox("Mute", &track.muted);
    ImGui::Button("...");

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopID();
}

static void uph_song_timeline_draw_and_handle_playhead(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float keyWidth)
{
    const float x = canvasPos.x + keyWidth + app->song_timeline_song_position * timeline_data.zoom_x - timeline_data.scroll_x;
    if (x < canvasPos.x + k_track_menu_width) return;
    drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasSize.y),
        IM_COL32(0, 255, 0, 255), 2.0f);
}

static void uph_song_timeline_render(UphPanel* panel)
{
    auto& tracks = app->project.tracks;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    if (ImGui::Button(app->is_song_timeline_playing ? "Stop" : "Play"))
    {
        app->is_song_timeline_playing = !app->is_song_timeline_playing;
        app->song_timeline_song_position = 0.0f;
        app->is_midi_editor_playing = false;
        if (!app->is_song_timeline_playing)
            uph_sound_device_all_notes_off();
    }

    ImVec2 fullSize = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("SongTimelineCanvas", fullSize, ImGuiChildFlags_AlwaysAutoResize);

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
            timeline_data.zoom_x *= (1.0f + io.MouseWheel * 0.1f);
            timeline_data.zoom_x = std::clamp(timeline_data.zoom_x, 4.0f, 128.0f);

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
            {
                timeline_data.zoom_y *= (1.0f - io.MouseDelta.y * 0.03f);
            }
        }
        else
        {
            timeline_data.scroll_y -= io.MouseWheel * 30.0f;
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                timeline_data.scroll_x -= io.MouseDelta.x;
                timeline_data.scroll_y -= io.MouseDelta.y;
            }
        }

        timeline_data.scroll_x = std::max(0.0f, timeline_data.scroll_x);
        timeline_data.scroll_y = std::max(0.0f, timeline_data.scroll_y);
    }

    timeline_data.smooth_scroll_y = uph_smooth_lerp(timeline_data.smooth_scroll_y, timeline_data.scroll_y, 15.0f, io.DeltaTime);

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        UphTrack& track = tracks[i];
        float y = canvasPos.y + i * k_track_height * timeline_data.zoom_y - timeline_data.smooth_scroll_y;
        float h = k_track_height * timeline_data.zoom_y;
        float timelineX = canvasPos.x + k_track_menu_width;

        for (auto& pattern : track.timeline_blocks)
        {
            float px = timelineX + pattern.start_time * timeline_data.zoom_x - timeline_data.scroll_x;
            float pw = pattern.length * timeline_data.zoom_x;

            ImVec2 rectMin(px, y);
            ImVec2 rectMax(px + pw, y + h);

            uph_song_timeline_handle_pattern_interaction(track, i, pattern, canvasPos, timelineX, canvasSize, rectMin, rectMax);
        }
    
        ImVec2 trackMin(canvasPos.x + k_track_menu_width + 4, y);
        ImVec2 trackMax(canvasPos.x + canvasSize.x, y + h);

        char buf[64];
        snprintf(buf, sizeof(buf), "Track %zu Drop Zone", i);
        ImGui::SetCursorScreenPos(trackMin);
        ImGui::InvisibleButton(buf, ImVec2(canvasSize.x, h));
            
        if (ImGui::BeginDragDropTarget())
        {
            if (track.track_type == UphTrackType_Midi)
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PATTERN"))
                {
                    size_t src_index = *(const size_t*)payload->Data;
                    if (track.track_type == UphTrackType_Midi)
                    {
                        UphTimelineBlock newInstance;
                        newInstance.pattern_index = (uint16_t)src_index;

                        float localX = io.MousePos.x - canvasPos.x - k_track_menu_width + timeline_data.scroll_x;
                        newInstance.start_time = quantizeToBeat(localX / timeline_data.zoom_x, k_beat_size);

                        newInstance.start_offset = 0.0f;
                        newInstance.length = 8.0f;

                        track.timeline_blocks.push_back(newInstance);
                    }
                }
            }
            else if (track.track_type == UphTrackType_Sample)
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAMPLE"))
                {
                    size_t src_index = *(const size_t*)payload->Data;
                    if (track.track_type == UphTrackType_Sample)
                    {
                        UphTimelineBlock newInstance;

                        newInstance.sample_index = (uint16_t)src_index;

                        float localX = io.MousePos.x - canvasPos.x - k_track_menu_width + timeline_data.scroll_x;
                        newInstance.start_time = quantizeToBeat(localX / timeline_data.zoom_x, k_beat_size);

                        newInstance.start_offset = 0.0f;
                        newInstance.length = 8.0f;
                        newInstance.stretch_scale = 1.0f;

                        track.timeline_blocks.push_back(newInstance);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        UphTrack& track = tracks[i];
        float y = canvasPos.y + i * k_track_height * timeline_data.zoom_y - timeline_data.smooth_scroll_y;
        float h = k_track_height * timeline_data.zoom_y;

        drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + canvasSize.x, y),
            IM_COL32(100, 100, 100, 255));

        float timelineX = canvasPos.x + k_track_menu_width;

        for (auto& pattern : track.timeline_blocks)
        {
            const UphMidiPattern& patternData = app->project.patterns[pattern.pattern_index];
            float px = timelineX + pattern.start_time * timeline_data.zoom_x - timeline_data.scroll_x;
            float pw = pattern.length * timeline_data.zoom_x;
            ImVec2 rectMin(px, y);
            ImVec2 rectMax(px + pw, y + h);

            if (track.track_type == UphTrackType_Midi)
                uph_song_timeline_draw_pattern_block(drawList, pattern, app->project.patterns[pattern.pattern_index], rectMin, rectMax,
                    track.color, titleHeight, timeline_data.zoom_x, timeline_data.zoom_y);
            else if (track.track_type == UphTrackType_Sample)
                uph_song_timeline_draw_sample_block(drawList, pattern, app->project.samples[pattern.sample_index], rectMin, rectMax,
                    track.color, titleHeight, timeline_data.zoom_x, timeline_data.zoom_y);
        }

        uph_song_timeline_draw_track_menu(track, i, y, h, canvasPos);
    }

    if (app->is_song_timeline_playing)
        uph_song_timeline_draw_and_handle_playhead(drawList, canvasPos, canvasSize, k_track_menu_width);

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

UPH_REGISTER_PANEL("Song Timeline", UphPanelFlags_Panel, uph_song_timeline_render, uph_song_timeline_init);