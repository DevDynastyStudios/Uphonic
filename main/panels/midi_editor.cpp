#include "panel_manager.h"
#include "sound_device.h"
#include "types.h"

#include "utils/lerp.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

struct UphMidiEditor
{
    // --- Scroll & zoom state ---
    float target_scroll_x       = 0.0f;
    float target_scroll_y       = 1000.0f;
    float smooth_scroll_x       = 0.0f;
    float smooth_scroll_y       = 10000.0f;

    float target_zoom_x         = 20.0f;
    float target_zoom_y         = 2.0f;
    float smooth_zoom_x         = target_zoom_x;

    // --- Grid & note state ---
    int   grid_size             = 1;
    float prev_length           = 1.0f;

    int   selected_note_index   = -1;
    bool  dragging_playhead     = false;
    bool  dragging_note         = false;
    bool  resizing_note         = false;

    ImVec2 drag_start_mouse     = ImVec2(0,0);
    float  drag_start_note_start  = 0.0f;
    float  drag_start_note_length = 0.0f;
    int    drag_start_note_pitch  = 0;

    // --- Configurable constants ---
    float default_key_width     = 60.0f;
    float default_key_height    = 10.0f;
    float scroll_step           = 30.0f;

    float zoom_x_min            = 10.0f;
    float zoom_x_max            = 60.0f;
    float zoom_y_min            = 1.0f;
    float zoom_y_max            = 5.0f;
    float zoom_sensitivity      = 0.01f;

    int   num_keys              = 128;
    int   max_midi_note         = 127;
    int   grid_highlight_step   = 16;

    float min_note_length       = 0.1f;
    float min_note_text_width   = 16.0f;
    float resize_edge_threshold = 8.0f;

	bool fancy_piano_keys = false;
	float black_key_scale = 0.6f;

    // --- Colors ---
    ImU32 white_key_color       	= IM_COL32(220,220,220,255);
    ImU32 black_key_color       	= IM_COL32(40,40,40,255);
    ImU32 key_border_color      	= IM_COL32(0,0,0,255);
    ImU32 note_text_color       	= IM_COL32(0,0,0,255);
	ImU32 grid_step_line_color		= IM_COL32(30,30,30,255);
	ImU32 grid_beat_line_color		= IM_COL32(90,90,90,255);
	ImU32 grid_measure_line_color	= IM_COL32(150,150,150,255);

    // --- Drag-drop ---
    const char* pattern_payload_type = "PATTERN";

};

static UphMidiEditor editor_data {};

static void uph_midi_editor_init(UphPanel* panel)
{
	panel->category = UPH_CATEGORY_EDITOR;
	panel->window_flags = ImGuiWindowFlags_NoScrollbar;
}

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
    if (ImGui::Button(app->is_midi_editor_playing ? "Stop" : "Play"))
    {
        if (app->is_midi_editor_playing)
        {
            app->is_midi_editor_playing = false;
            uph_sound_device_all_notes_off();
        }
        else
        {
            app->is_midi_editor_playing = true;
            app->is_song_timeline_playing = false;
            app->midi_editor_song_position = 0.0f;
        }
    }
    ImGui::SameLine();
    ImGui::Text("%s", app->project.patterns[app->current_pattern_index].name);
}

void midi_editor_draw_and_handle_playhead(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size, float key_width)
{
    float x = canvas_pos.x + key_width + app->midi_editor_song_position * editor_data.smooth_zoom_x - editor_data.smooth_scroll_x;
    draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y),
        IM_COL32(0, 255, 0, 255), 2.0f);
}

struct UphNoteRect 
{
    float x, y, w, h;
};

static UphNoteRect uph_get_note_rect(const UphNote& note, const ImVec2& canvas_pos, float key_width, float key_height, const UphMidiEditor& ed)
{
    UphNoteRect r;
    r.x = canvas_pos.x + key_width + note.start * ed.smooth_zoom_x - ed.smooth_scroll_x;
    r.w = note.length * ed.smooth_zoom_x;
    r.y = canvas_pos.y + (editor_data.max_midi_note - note.key) * key_height - ed.smooth_scroll_y;
    r.h = key_height;
    return r;
}

static bool uph_point_in_rect(const ImVec2& p, const UphNoteRect& r) {
    return p.x >= r.x && p.x <= r.x + r.w &&
           p.y >= r.y && p.y <= r.y + r.h;
}

static bool uph_point_on_right_edge(const ImVec2& p, const UphNoteRect& r) {
    return uph_point_in_rect(p, r) && (p.x >= r.x + r.w - editor_data.resize_edge_threshold);
}

static bool uph_is_black_key(int midi_note) {
    static const bool black_keys[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
    return black_keys[midi_note % 12];
}

static void uph_draw_note(ImDrawList* dl, const UphNote& note, const ImVec2& canvas_pos, float key_width, float key_height, const UphMidiEditor& ed, ImU32 color)
{
    UphNoteRect r = uph_get_note_rect(note, canvas_pos, key_width, key_height, ed);
    dl->AddRectFilled(ImVec2(r.x, r.y), ImVec2(r.x + r.w, r.y + r.h), color);

    char buf[8]; uph_key_to_name(note.key, buf, sizeof(buf));
    ImVec2 text_size = ImGui::CalcTextSize(buf);
    if (r.w > editor_data.min_note_text_width) {
        float tx = r.x + (r.w - text_size.x) * 0.5f;
        float ty = r.y + (r.h - text_size.y) * 0.5f;
        dl->AddText(ImVec2(tx, ty), editor_data.note_text_color, buf);
    }
}

static void uph_draw_piano_key(ImDrawList* dl, int midi_note, const ImVec2& canvas_pos, float key_width, float key_height, float scroll_y, const ImGuiStyle& style)
{
    float y = canvas_pos.y + (editor_data.max_midi_note - midi_note) * key_height - scroll_y;
    ImU32 key_color = uph_is_black_key(midi_note) ? editor_data.black_key_color : editor_data.white_key_color;

    dl->AddRectFilled(ImVec2(canvas_pos.x - style.WindowPadding.x, y), ImVec2(canvas_pos.x + key_width, y + key_height), key_color);
    dl->AddRect(ImVec2(canvas_pos.x - style.WindowPadding.x, y), ImVec2(canvas_pos.x + key_width, y + key_height), editor_data.key_border_color);

    char buf[8]; uph_key_to_name(midi_note, buf, sizeof(buf));
    ImVec2 text_size = ImGui::CalcTextSize(buf);
    float tx = canvas_pos.x - style.WindowPadding.x + (key_width - text_size.x) * 0.5f;
    float ty = y + (key_height - text_size.y) * 0.5f;
    dl->AddText(ImVec2(tx, ty), editor_data.note_text_color, buf);
}

static void uph_midi_editor_draw_piano_keys(ImDrawList* dl, const ImVec2& canvas_pos, const ImVec2& canvas_size, float key_width, float key_height, float black_key_scale, const UphMidiEditor& ed)
{
    int min_note = ed.max_midi_note - (ed.num_keys - 1);

    // --- Pass 1: draw white keys ---
    int white_row = 0;
    for (int n = ed.max_midi_note; n >= min_note; --n) { // iterate downward so octaves increase upward
        if (uph_is_black_key(n)) continue;

        float y = canvas_pos.y + white_row * key_height - ed.smooth_scroll_y;
        ImVec2 key_min(canvas_pos.x, y);
        ImVec2 key_max(canvas_pos.x + key_width, y + key_height);

        dl->AddRectFilled(key_min, key_max, IM_COL32(240,240,240,255));
        dl->AddRect(key_min, key_max, IM_COL32(0,0,0,255));

        // Label C keys on the right edge
        if (n % 12 == 0) {
            int octave = (n / 12);
            char buf[8];
            snprintf(buf, sizeof(buf), "C%d", octave);
            ImVec2 text_pos(key_max.x - ImGui::CalcTextSize(buf).x - 4, key_min.y + 2);
            dl->AddText(text_pos, IM_COL32(0,0,0,255), buf);
        }

        white_row++;
    }

    // --- Pass 2: draw black keys ---
    float black_key_width  = key_width * black_key_scale;
    float black_key_height = key_height * black_key_scale;
    float black_y_offset   = black_key_height * 0.5f;

    white_row = 0;
    for (int n = ed.max_midi_note; n >= min_note; --n) {
        if (uph_is_black_key(n)) {
            float y = canvas_pos.y + white_row * key_height - ed.smooth_scroll_y;

            ImVec2 black_min(canvas_pos.x, y - black_y_offset);
            ImVec2 black_max(canvas_pos.x + black_key_width, y + black_key_height - black_y_offset);

            dl->AddRectFilled(black_min, black_max, IM_COL32(20,20,20,255));
            dl->AddRect(black_min, black_max, IM_COL32(0,0,0,255));
        } else {
            white_row++;
        }
    }
}

// Smooth lerp utility (already existed in your codebase)
static inline float uph_smooth_to(float current, float target, float speed, float dt) {
    return uph_smooth_lerp(current, target, speed, dt);
}

// Handle scroll/zoom input
static void uph_midi_editor_handle_input(UphMidiEditor* ed, const ImGuiIO& io, const ImVec2& canvas_pos, float key_width, float dt)
{
    // Always smooth
    ed->smooth_scroll_x = uph_smooth_to(ed->smooth_scroll_x, ed->target_scroll_x, 15.0f, dt);
    ed->smooth_scroll_y = uph_smooth_to(ed->smooth_scroll_y, ed->target_scroll_y, 15.0f, dt);
    ed->smooth_zoom_x   = uph_smooth_to(ed->smooth_zoom_x,   ed->target_zoom_x,   10.0f, dt);

    // Only process input if hovered
    if (!ImGui::IsWindowHovered()) return;

    if (io.MouseWheel != 0 && io.KeyCtrl) {
        ed->target_zoom_x *= (io.MouseWheel > 0) ? 1.1f : 0.9f;
        ed->target_zoom_x = std::clamp(ed->target_zoom_x, ed->zoom_x_min, ed->zoom_x_max);
    }

    if (io.MouseWheel != 0 && !io.KeyCtrl) {
        ed->target_scroll_y -= io.MouseWheel * ed->scroll_step;
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        if (io.KeyCtrl) {
            float mouse_y_in_canvas = io.MousePos.y - canvas_pos.y;
            float prev_zoom_y = ed->target_zoom_y;
            ed->target_zoom_y *= 1.0f - io.MouseDelta.y * ed->zoom_sensitivity;
            ed->target_zoom_y = std::clamp(ed->target_zoom_y, ed->zoom_y_min, ed->zoom_y_max);
            ed->target_scroll_y += mouse_y_in_canvas * (ed->target_zoom_y - prev_zoom_y);
        } else {
            ed->target_scroll_x -= io.MouseDelta.x;
            ed->target_scroll_y -= io.MouseDelta.y;
        }
    }

    ed->target_scroll_x = std::max<float>(0.0f, ed->target_scroll_x);
}

// Clamp scroll so it doesn’t go out of bounds
static void uph_midi_editor_clamp_scroll(UphMidiEditor* ed, const ImVec2& canvas_size, float key_height)
{
    float max_scroll_y = editor_data.num_keys * key_height - canvas_size.y;
    ed->target_scroll_y = std::clamp<float>(ed->target_scroll_y, 0.0f, max_scroll_y);
}

// Cursor feedback
static void uph_update_cursor_feedback(const std::vector<UphNote>& notes, const ImVec2& mouse_pos, const ImVec2& canvas_pos, float key_width, float key_height, const UphMidiEditor& ed)
{
    bool over_edge = false;
    for (const auto& note : notes) {
        UphNoteRect r = uph_get_note_rect(note, canvas_pos, key_width, key_height, ed);
        if (uph_point_on_right_edge(mouse_pos, r)) over_edge = true;
        else if (uph_point_in_rect(mouse_pos, r)) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    if (over_edge) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
}

// Left click: select or create note
static void uph_handle_left_click(UphMidiEditor& ed, std::vector<UphNote>& notes, const ImVec2& mouse_pos, const ImVec2& canvas_pos, float key_width, float key_height)
{
    ed.selected_note_index = -1;
    for (size_t i = 0; i < notes.size(); ++i) {
        UphNoteRect r = uph_get_note_rect(notes[i], canvas_pos, key_width, key_height, ed);
        if (uph_point_in_rect(mouse_pos, r)) {
            ed.selected_note_index = (int)i;
            ed.drag_start_mouse = mouse_pos;
            ed.drag_start_note_start = notes[i].start;
            ed.drag_start_note_length = notes[i].length;
            ed.drag_start_note_pitch = notes[i].key;
            if (uph_point_on_right_edge(mouse_pos, r)) ed.resizing_note = true;
            else { ed.dragging_note = true; ed.prev_length = notes[i].length; }
            return;
        }
    }

    // Create new note if none selected
    if (ed.selected_note_index == -1) {
        UphNote n;
        n.key = std::clamp(editor_data.max_midi_note - int((mouse_pos.y - canvas_pos.y + ed.smooth_scroll_y) / key_height), 0, editor_data.max_midi_note);
        float raw_start = (mouse_pos.x - canvas_pos.x - key_width + ed.smooth_scroll_x) / ed.smooth_zoom_x;
        n.start = std::clamp<float>(round(raw_start / ed.grid_size) * ed.grid_size, 0.0f, (float)INT32_MAX);
        n.length = ed.prev_length;
        notes.push_back(n);
        ed.selected_note_index = (int)notes.size() - 1;
        ed.drag_start_mouse = mouse_pos;
        ed.drag_start_note_start = n.start;
        ed.drag_start_note_length = n.length;
        ed.drag_start_note_pitch = n.key;
        ed.dragging_note = true;
        ed.prev_length = n.length;
    }
}

// Right click: delete notes
static void uph_handle_right_click(UphMidiEditor& ed, std::vector<UphNote>& notes, const ImVec2& mouse_pos, const ImVec2& canvas_pos, float key_width, float key_height)
{
    std::vector<size_t> remove_index;
    for (size_t i = 0; i < notes.size(); ++i) {
        UphNoteRect r = uph_get_note_rect(notes[i], canvas_pos, key_width, key_height, ed);
        if (uph_point_in_rect(mouse_pos, r)) {
            remove_index.push_back(i);
        }
    }
    std::sort(remove_index.rbegin(), remove_index.rend());
    for (size_t idx : remove_index) notes.erase(notes.begin() + idx);
}

// Dragging/resizing
static void uph_handle_drag(UphMidiEditor& ed, std::vector<UphNote>& notes, const ImVec2& mouse_pos, float key_height)
{
    if (ed.selected_note_index < 0 || ed.selected_note_index >= (int)notes.size())
        return;

    UphNote& sel = notes[ed.selected_note_index];

    // Dragging a note (move in time and pitch)
    if (ed.dragging_note && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float delta_x = (mouse_pos.x - ed.drag_start_mouse.x) / ed.smooth_zoom_x;
        float delta_y = (mouse_pos.y - ed.drag_start_mouse.y) / key_height;

        sel.start = std::clamp<float>(
            round((ed.drag_start_note_start + delta_x) / ed.grid_size) * ed.grid_size,
            0.0f,
            (float)INT32_MAX
        );

        sel.key = std::clamp(ed.drag_start_note_pitch - (int)delta_y, 0, editor_data.max_midi_note);
    }

    // Resizing a note (change length)
    if (ed.resizing_note && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float delta_x = (mouse_pos.x - ed.drag_start_mouse.x) / ed.smooth_zoom_x;

        sel.length = std::max<float>(
            editor_data.min_note_length,
            round((ed.drag_start_note_length + delta_x) / ed.grid_size) * ed.grid_size
        );

        ed.prev_length = sel.length;
    }

    // Release mouse: stop dragging/resizing
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        ed.dragging_note = false;
        ed.resizing_note = false;
        ed.selected_note_index = -1;
    }
}

static void uph_midi_editor_draw_grid(ImDrawList* dl, const ImVec2& canvas_pos, const ImVec2& canvas_size, float key_width, float key_height, const UphMidiEditor& ed, int time_sig_num, int time_sig_den, int steps_per_beat)
{
    // --- Horizontal rows (keys) ---
    for (int i = 0; i < ed.num_keys; ++i) {
        float y = canvas_pos.y + (ed.max_midi_note - i) * key_height - ed.smooth_scroll_y;
        bool is_black = uph_is_black_key(i);
        ImU32 row_color = is_black ? IM_COL32(60,60,60,50) : IM_COL32(80,80,80,50);

        dl->AddRectFilled(ImVec2(canvas_pos.x + key_width, y), ImVec2(canvas_pos.x + canvas_size.x, y + key_height), row_color);

        dl->AddLine(ImVec2(canvas_pos.x + key_width, y), ImVec2(canvas_pos.x + canvas_size.x, y), ed.grid_step_line_color);
    }

    // --- Vertical grid lines ---
    float spacing = ed.smooth_zoom_x; // pixels per "step"
    int start = (int)((ed.smooth_scroll_x - key_width) / spacing) - 1;
    int end   = (int)((ed.smooth_scroll_x + canvas_size.x) / spacing) + 1;

    float beat_length_quarters = 4.0f / (float)time_sig_den;
    float steps_per_quarter = steps_per_beat / beat_length_quarters;
    int steps_per_beat_norm = (int)(steps_per_quarter * beat_length_quarters);
    int steps_per_measure = (int)(time_sig_num * steps_per_beat);

    for (int i = start; i <= end; i++) {
        float x = canvas_pos.x + key_width + (i * spacing - ed.smooth_scroll_x);

        ImU32 color;
        float thickness = 1.0f;

        if (i % steps_per_measure == 0) {
            // End of measure
            color = ed.grid_measure_line_color;
            thickness = 3.0f;
        } else if (i % steps_per_beat == 0) {
            // Beat line
            color = ed.grid_beat_line_color;
            thickness = 2.0f;
        } else {
            // Subdivision
            color = ed.grid_step_line_color;
            thickness = 1.0f;
        }

        dl->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y), color, thickness);
    }
}

static void uph_midi_editor_render(UphPanel* panel)
{
    if (app->project.patterns.empty()) {
        ImGui::Text("No pattern selected!");
        return;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;

    midi_editor_draw_transport();

    ImVec2 child_size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("MidiEditorCanvas", child_size);

    const UphTrack& track = app->project.tracks[app->current_track_index];
    std::vector<UphNote>& notes = app->project.patterns[app->current_pattern_index].notes;

    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const float key_width = editor_data.default_key_width;
    const float key_height = editor_data.default_key_height * editor_data.target_zoom_y;

    // input handling
    uph_midi_editor_handle_input(&editor_data, io, canvas_pos, key_width, dt);
    uph_midi_editor_clamp_scroll(&editor_data, canvas_size, key_height);

    ImVec2 mouse_pos = io.MousePos;
    bool click_inside = mouse_pos.x > canvas_pos.x + key_width &&
                        mouse_pos.x < canvas_pos.x + canvas_size.x &&
                        mouse_pos.y > canvas_pos.y &&
                        mouse_pos.y < canvas_pos.y + canvas_size.y;

    // cursor feedback & note interaction
    if (editor_data.dragging_note) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    else if (editor_data.resizing_note) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    else if (click_inside) uph_update_cursor_feedback(notes, mouse_pos, canvas_pos, key_width, key_height, editor_data);

    if (click_inside && ImGui::IsWindowHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            uph_handle_left_click(editor_data, notes, mouse_pos, canvas_pos, key_width, key_height);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            uph_handle_right_click(editor_data, notes, mouse_pos, canvas_pos, key_width, key_height);
    }

    uph_handle_drag(editor_data, notes, mouse_pos, key_height);
	float grid_note_height = editor_data.fancy_piano_keys ? key_height * editor_data.black_key_scale : key_height;
	uph_midi_editor_draw_grid(draw_list, canvas_pos, canvas_size, key_width, grid_note_height, editor_data, app->project.time_sig_numerator, app->project.time_sig_denominator, app->project.steps_per_beat);

    // draw notes
    for (auto& note : notes)
        uph_draw_note(draw_list, note, canvas_pos, key_width, key_height, editor_data, track.color);

    // playhead
    if (app->is_midi_editor_playing)
        midi_editor_draw_and_handle_playhead(draw_list, canvas_pos, canvas_size, key_width);

    // piano keys
	if(editor_data.fancy_piano_keys)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		uph_midi_editor_draw_piano_keys(dl, canvas_pos, canvas_size, key_width, key_height, editor_data.black_key_scale, editor_data);

	} else
	{
    for (int i = 0; i <= editor_data.max_midi_note; ++i)
        uph_draw_piano_key(draw_list, i, canvas_pos, key_width, key_height, editor_data.smooth_scroll_y, style);
	}

    // drag-drop target
    ImGui::InvisibleButton("canvas", canvas_size);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(editor_data.pattern_payload_type)) {
            size_t src_index = *(const size_t*)payload->Data;
            app->current_pattern_index = (int)src_index;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();
}

UPH_REGISTER_PANEL("Midi Editor", UphPanelFlags_Panel, uph_midi_editor_render, uph_midi_editor_init);