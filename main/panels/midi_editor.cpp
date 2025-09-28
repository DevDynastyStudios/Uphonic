#include "panel_manager.h"
#include "types.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

struct UphMidiEditor
{
    float scroll_x = 0.0f;
    float scroll_y = 800.0f;
    float zoom_x  = 20.0f;
    float zoom_y  = 2.0f;
    int   grid_size = 1;
    float prev_length = 1.0f;

    bool is_playing = false;
    float song_position = 0.0f;

    bool dragging_playhead = false;
    int  selected_note_index = -1;
    bool dragging_note = false;
    bool resizing_note = false;
    ImVec2 drag_start_mouse = ImVec2(0,0);
    float drag_start_note_start = 0.0f;
    float drag_start_note_length = 0.0f;
    int   drag_start_note_pitch = 0;

    UphMidiPattern *pattern = nullptr;
    AEffect *effect = nullptr;
};

static UphMidiEditor editor_data {};

static inline void uph_key_to_name(int key, char* out, size_t out_size)
{
    static const char* names[12] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    int octave = key / 12;
    int note = key % 12;
    snprintf(out, out_size, "%s%d", names[note], octave);
}

static void midi_editor_draw_transport(void)
{
    if (ImGui::Button(editor_data.is_playing ? "Stop" : "Play"))
    {
        if (editor_data.is_playing)
        {
            editor_data.is_playing = false;
            //effect_all_notes_off(effect);
        }
        else
        {
            editor_data.is_playing = true;
            editor_data.is_playing = 0.0f;
        }
    }
    ImGui::SameLine();
    ImGui::Text("Pos: %.2f beats", editor_data.song_position);
}

void midi_editor_draw_and_handle_playhead(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size, float key_width)
{
    float x = canvas_pos.x + key_width + editor_data.song_position * editor_data.zoom_x - editor_data.scroll_x;
    draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y),
        IM_COL32(0, 255, 0, 255), 2.0f);
}

static void uph_midi_editor_render(UphPanel* panel)
{
    const bool is_hovered = ImGui::IsWindowHovered();
    midi_editor_draw_transport();

    ImVec2 child_size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("MidiEditorCanvas", child_size);

    if (!editor_data.pattern)
    {
        ImGui::Text("No pattern loaded.");
        ImGui::EndChild();
        return;
    }

    std::vector<UphNote>& notes = editor_data.pattern->notes;

    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();

    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const float key_width = 60.0f;
    const float key_height = 10.0f * editor_data.zoom_y;

    if (ImGui::IsWindowHovered())
    {
        if (io.MouseWheel != 0 && io.KeyCtrl)
        {
            float mouse_x_in_canvas = io.MousePos.x - canvas_pos.x - key_width;
            float prev_zoom_x = editor_data.zoom_x;
            editor_data.zoom_x *= (io.MouseWheel > 0) ? 1.1f : 0.9f;
            editor_data.zoom_x = std::clamp(editor_data.zoom_x, 0.5f, 10.0f);
            editor_data.scroll_x += mouse_x_in_canvas * (editor_data.zoom_x - prev_zoom_x);
        }
        if (io.MouseWheel != 0 && !io.KeyCtrl) editor_data.scroll_y -= io.MouseWheel * 10.0f;
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            if (io.KeyCtrl)
            {
                float mouse_y_in_canvas = io.MousePos.y - canvas_pos.y;
                float prevZoomY = editor_data.zoom_y;
                editor_data.zoom_y *= 1.0f - io.MouseDelta.y * 0.01f;
                editor_data.zoom_y = std::clamp(editor_data.zoom_y, 1.0f, 5.0f);
                editor_data.scroll_y += mouse_y_in_canvas * (editor_data.zoom_y - prevZoomY);
            }
            else
            {
                editor_data.scroll_x -= io.MouseDelta.x;
                editor_data.scroll_y -= io.MouseDelta.y;
            }
        }
        editor_data.scroll_x = std::max(0.0f, editor_data.scroll_x);
        editor_data.scroll_y = std::max(0.0f, editor_data.scroll_y);
    }

    for (int i = 0; i < 128; ++i)
    {
        float y = canvas_pos.y + (127 - i) * key_height - editor_data.scroll_y;
        bool is_black = (i % 12 == 1 || i % 12 == 3 || i % 12 == 6 || i % 12 == 8 || i % 12 == 10);
        ImU32 row_color = is_black ? IM_COL32(60,60,60,50) : IM_COL32(80,80,80,50);
        draw_list->AddRectFilled(ImVec2(canvas_pos.x+key_width, y),
            ImVec2(canvas_pos.x+canvas_size.x, y+key_height),
            row_color);
        draw_list->AddLine(ImVec2(canvas_pos.x+key_width, y),
            ImVec2(canvas_pos.x+canvas_size.x, y),
            IM_COL32(60,60,60,255));
    }

    {
        float spacing = 20.0f * editor_data.zoom_x;
        int start = (int)((editor_data.scroll_x - key_width) / spacing) - 1;
        int end = (int)((editor_data.scroll_x + canvas_size.x) / spacing) + 1;

        for (int i = start; i <= end; i++)
        {
            float x = canvas_pos.x + key_width + (i * spacing - editor_data.scroll_x);

            draw_list->AddLine(
                ImVec2(x, canvas_pos.y),
                ImVec2(x, canvas_pos.y + canvas_size.y),
                (i % 16 == 0) ? IM_COL32(100,100,100,255) : IM_COL32(60,60,60,255)
            );
        }
    }

    ImVec2 mouse_pos = io.MousePos;
    bool click_inside = mouse_pos.x > canvas_pos.x + key_width &&
        mouse_pos.x < canvas_pos.x + canvas_size.x &&
        mouse_pos.y > canvas_pos.y &&
        mouse_pos.y < canvas_pos.y + canvas_size.y;

    if (editor_data.dragging_note)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    else if (editor_data.resizing_note)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    else if (click_inside)
    {
        bool over_note_edge = false;
        for (size_t i = 0; i < notes.size(); ++i)
        {
            UphNote &note = notes[i];
            float note_x = canvas_pos.x + key_width + note.start*editor_data.zoom_x - editor_data.scroll_x;
            float note_w = note.length*editor_data.zoom_x;
            float note_y = canvas_pos.y + (127 - note.key) * key_height - editor_data.scroll_y;
            float note_h = key_height;
            bool inside_note = mouse_pos.x >= note_x && mouse_pos.x <= note_x + note_w &&
                mouse_pos.y >= note_y && mouse_pos.y <= note_y + note_h;
            bool on_right_edge = inside_note && (mouse_pos.x >= note_x + note_w - 8);
            if (on_right_edge) over_note_edge = true;
            if (inside_note && !on_right_edge) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        if (over_note_edge)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    if (click_inside && ImGui::IsWindowHovered())
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            editor_data.selected_note_index = -1;
            for (size_t i = 0; i < notes.size(); ++i)
            {
                UphNote &note = notes[i];
                float note_x = canvas_pos.x + key_width + note.start*editor_data.zoom_x - editor_data.scroll_x;
                float note_w = note.length*editor_data.zoom_x;
                float note_y = canvas_pos.y + (127 - note.key) * key_height - editor_data.scroll_y;
                float note_h = key_height;
                bool inside_note = mouse_pos.x >= note_x && mouse_pos.x <= note_x + note_w &&
                    mouse_pos.y >= note_y && mouse_pos.y <= note_y + note_h;
                bool on_right_edge = inside_note && (mouse_pos.x >= note_x + note_w - 8);
                if (inside_note)
                {
                    editor_data.selected_note_index = (int)i;
                    editor_data.drag_start_mouse = mouse_pos;
                    editor_data.drag_start_note_start = note.start;
                    editor_data.drag_start_note_length = note.length;
                    editor_data.drag_start_note_pitch = note.key;
                    if (on_right_edge) editor_data.resizing_note = true;
                    else {
                        editor_data.dragging_note = true;
                        editor_data.prev_length = note.length;
                    }
                    break;
                }
            }

            if (editor_data.selected_note_index == -1)
            {
                UphNote n;
                n.key = std::clamp(127 - int((mouse_pos.y - canvas_pos.y + editor_data.scroll_y)/key_height), 0, 127);
                float raw_start = (mouse_pos.x - canvas_pos.x - key_width + editor_data.scroll_x) / (20.0f*editor_data.zoom_x);
                n.start = std::clamp<float>(round(raw_start / editor_data.grid_size) * editor_data.grid_size, 0.0f, (float)INT32_MAX);
                n.length = editor_data.prev_length;
                notes.push_back(n);
                editor_data.selected_note_index = (int)notes.size() - 1;
                editor_data.drag_start_mouse = mouse_pos;
                editor_data.drag_start_note_start = n.start;
                editor_data.drag_start_note_length = n.length;
                editor_data.drag_start_note_pitch = n.key;
                editor_data.dragging_note = true;
                editor_data.prev_length = n.length;
            }
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            static std::vector<int> removed_pitches;
            removed_pitches.reserve(4);

            std::vector<size_t> remove_index;
            for (size_t i = 0; i < notes.size(); ++i)
            {
                UphNote &n = notes[i];
                float note_x = canvas_pos.x + key_width + n.start*editor_data.zoom_x - editor_data.scroll_x;
                float note_w = n.length*editor_data.zoom_x;
                float note_y = canvas_pos.y + (127 - n.key) * key_height - editor_data.scroll_y;
                float note_h = key_height;
                if (mouse_pos.x >= note_x && mouse_pos.x <= note_x + note_w &&
                    mouse_pos.y >= note_y && mouse_pos.y <= note_y + note_h)
                {
                    remove_index.push_back(i);
                    removed_pitches.push_back(n.key);
                }
            }

            std::sort(remove_index.rbegin(), remove_index.rend());
            for (size_t idx : remove_index) notes.erase(notes.begin() + idx);

            if (editor_data.is_playing && editor_data.effect)
            {
                for (int pitch : removed_pitches)
                {
                    bool still_playing_somewhere = false;
                    for (auto &n : notes) {
                        if (n.key != pitch) continue;
                        float note_on_beat = n.start;
                        float note_off_beat = n.start + n.length;
                        if (note_on_beat <= editor_data.song_position && editor_data.song_position < note_off_beat)
                        {
                            still_playing_somewhere = true;
                            break;
                        }
                    }
                    if (!still_playing_somewhere)
                    {
                        VstMidiEvent ev; memset(&ev, 0, sizeof(ev));
                        ev.type = kVstMidiType; ev.byteSize = sizeof(ev);
                        ev.midiData[0] = 0x80; ev.midiData[1] = pitch; ev.midiData[2] = 0;
                        VstEvents events{}; events.numEvents = 1; events.events[0] = (VstEvent*)&ev;
                        editor_data.effect->dispatcher(editor_data.effect, effProcessEvents, 0, 0, &events, 0.0f);
                    }
                }
            }
        }
    }

    if (editor_data.dragging_note && editor_data.selected_note_index >= 0 && editor_data.selected_note_index < (int)notes.size() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        UphNote &sel = notes[editor_data.selected_note_index];
        float delta_x = (mouse_pos.x - editor_data.drag_start_mouse.x) / (20.0f * editor_data.zoom_x);
        float delta_y = (mouse_pos.y - editor_data.drag_start_mouse.y) / key_height;
        sel.start = std::clamp<float>(round((editor_data.drag_start_note_start + delta_x)/editor_data.grid_size) * editor_data.grid_size, 0.0f, (float)INT32_MAX);
        sel.key = std::clamp(editor_data.drag_start_note_pitch - int(delta_y), 0, 127);
    }
    if (editor_data.resizing_note && editor_data.selected_note_index >= 0 && editor_data.selected_note_index < (int)notes.size() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        UphNote &sel = notes[editor_data.selected_note_index];
        float delta_x = (mouse_pos.x - editor_data.drag_start_mouse.x) / (20.0f * editor_data.zoom_x);
        sel.length = std::max<float>(0.1f, round((editor_data.drag_start_note_length + delta_x)/editor_data.grid_size) * editor_data.grid_size);
        editor_data.prev_length = sel.length;
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        editor_data.dragging_note = false;
        editor_data.resizing_note = false;
        editor_data.selected_note_index = -1;
    }

    for (size_t i = 0; i < notes.size(); ++i)
    {
        UphNote &note = notes[i];
        ImVec2 note_pos(canvas_pos.x + key_width + note.start*editor_data.zoom_x - editor_data.scroll_x,
            canvas_pos.y + (127 - note.key) * key_height - editor_data.scroll_y);
        ImVec2 note_end(note_pos.x + note.length*editor_data.zoom_x, note_pos.y + key_height);
        draw_list->AddRectFilled(note_pos, note_end, IM_COL32(200,100,100,255));

        char buf[8]; uph_key_to_name(note.key, buf, sizeof(buf));
        ImVec2 text_size = ImGui::CalcTextSize(buf);
        float text_x = note_pos.x + (note_end.x - note_pos.x - text_size.x) * 0.5f;
        float text_y = note_pos.y + (key_height - text_size.y) * 0.5f;
        if (note_end.x - note_pos.x > 16)
            draw_list->AddText(ImVec2(text_x, text_y), IM_COL32(255,255,255,255), buf);
    }

    midi_editor_draw_and_handle_playhead(draw_list, canvas_pos, canvas_size, key_width);

    for (int i = 0; i < 128; ++i)
    {
        float y = canvas_pos.y + (127 - i) * key_height - editor_data.scroll_y;
        bool is_black = (i % 12 == 1 || i % 12 == 3 || i % 12 == 6 || i % 12 == 8 || i % 12 == 10);
        ImU32 key_color = is_black ? IM_COL32(40,40,40,255) : IM_COL32(220,220,220,255);

        draw_list->AddRectFilled(ImVec2(canvas_pos.x - style.WindowPadding.x, y),
            ImVec2(canvas_pos.x + key_width, y + key_height),
            key_color);

        draw_list->AddRect(ImVec2(canvas_pos.x - style.WindowPadding.x, y),
            ImVec2(canvas_pos.x + key_width, y + key_height),
            IM_COL32(0,0,0,255));

        char buf[8];
        uph_key_to_name(i, buf, sizeof(buf));

        ImVec2 text_size = ImGui::CalcTextSize(buf);
        float text_x = canvas_pos.x - style.WindowPadding.x + (key_width - text_size.x) * 0.5f;
        float text_y = y + (key_height - text_size.y) * 0.5f;
        draw_list->AddText(ImVec2(text_x, text_y), IM_COL32(0,0,0,255), buf);
    }

    ImGui::InvisibleButton("canvas", canvas_size);
    ImGui::EndChild();
}

UPH_REGISTER_PANEL("Midi Editor", ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, uph_midi_editor_render);