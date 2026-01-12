#include "Core.h"
#include "PluginLoader.h"
#include "SoundDevice.h"

#include <vector>
#include <algorithm>
#include <cmath>

using namespace ImGui;



// Initialize static members

Settings Core::settings{};
MasterTrack Core::masterTrack{};
std::vector<Track> Core::tracks{};
std::vector<MidiPattern> Core::patterns{};
std::vector<Sample> Core::samples{};
uint16_t Core::currentMidiPattern = 0;
float Core::bpm = 120.0f;
float Core::volume = 0.8f;
double Core::timelinePosition = 0.0;
bool Core::isPlayingTimeline = false;
bool Core::isDraggingHandle = false;
Naui::PlatformWindow *Core::mainWindow = nullptr;


class MidiEditor : public Naui::Panel
{
public:
    MidiEditor(void) : Naui::Panel("Midi Editor")
    {
        m_zoom = 1.0f;
        m_scrollX = 0.0f;
        m_scrollY = 60.0f;
        m_snap = 16;
        m_verticalZoom = 1.0f;
    }

protected:
    void OnRender(void) override
    {
        if (Core::patterns.empty())
        {
            Text("No patterns found");
            return;
        }
        RenderToolbar();
        ImGui::Separator();
        
        ImGui::BeginChild("##PianoRoll", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
        RenderPianoRoll();
        ImGui::EndChild();
    }

private:
    std::vector<int> m_selectedNotes;
    
    // View state
    float m_zoom;
    float m_scrollX;
    float m_scrollY;
    int m_snap;
    float m_verticalZoom;
    float m_lastNoteLength = 1.0f;
    
    // Interaction state
    enum class Tool { Select, Draw, Erase };
    Tool m_currentTool = Tool::Draw;
    
    bool m_isDragging = false;
    bool m_isResizing = false;
    bool m_isSelecting = false;
    int m_draggedNote = -1;
    ImVec2 m_dragStartPos;
    ImVec2 m_selectionStart;
    ImVec2 m_selectionEnd;
    
    struct OriginalPosition {
        double start;
        int pitch;
    };
    std::vector<OriginalPosition> m_originalPositions;
    
    static constexpr float NOTE_HEIGHT = 16.0f;
    static constexpr float BEAT_WIDTH_BASE = 40.0f;
    static constexpr int TOTAL_KEYS = 128;
    static constexpr float PIANO_WIDTH = 60.0f;
    
    MidiPattern& GetCurrentPattern()
    {
        if (Core::currentMidiPattern >= Core::patterns.size()) {
            Core::currentMidiPattern = 0;
        }
        return Core::patterns[Core::currentMidiPattern];
    }
    
    void RenderToolbar()
    {
        // Pattern selector
        ImGui::Text("Pattern:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        int currentPattern = Core::currentMidiPattern;
        if (ImGui::InputInt("##Pattern", &currentPattern, 1, 1)) {
            Core::currentMidiPattern = std::clamp(currentPattern, 0, (int)Core::patterns.size() - 1);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("New Pattern")) {
            Core::patterns.push_back(MidiPattern());
            Core::patterns.back().name = "Pattern " + std::to_string(Core::patterns.size());
            Core::currentMidiPattern = Core::patterns.size() - 1;
        }
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Tool selection
        if (ImGui::RadioButton("Select", m_currentTool == Tool::Select)) {
            m_currentTool = Tool::Select;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Draw", m_currentTool == Tool::Draw)) {
            m_currentTool = Tool::Draw;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Erase", m_currentTool == Tool::Erase)) {
            m_currentTool = Tool::Erase;
        }
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Snap settings
        ImGui::Text("Snap:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        const char* snap_preview = GetSnapName(m_snap);
        if (ImGui::BeginCombo("##Snap", snap_preview, ImGuiComboFlags_NoArrowButton)) {
            if (ImGui::Selectable("1/4", m_snap == 4)) m_snap = 4;
            if (ImGui::Selectable("1/8", m_snap == 8)) m_snap = 8;
            if (ImGui::Selectable("1/16", m_snap == 16)) m_snap = 16;
            if (ImGui::Selectable("1/32", m_snap == 32)) m_snap = 32;
            ImGui::EndCombo();
        }
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Zoom controls
        ImGui::Text("Zoom:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        ImGui::SliderFloat("##Zoom", &m_zoom, 0.25f, 4.0f, "%.2fx");
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        if (ImGui::Button("Delete Selected")) {
            DeleteSelectedNotes();
        }
        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
            SelectAllNotes();
        }
    }
    
    void RenderPianoRoll()
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        
        float beat_width = BEAT_WIDTH_BASE * m_zoom;
        float note_height = NOTE_HEIGHT * m_verticalZoom;
        
        // Handle scrolling
        if (ImGui::IsWindowHovered()) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (ImGui::GetIO().KeyCtrl && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                m_verticalZoom += wheel * 0.1f;
                m_verticalZoom = std::clamp(m_verticalZoom, 0.5f, 3.0f);
            } else if (ImGui::GetIO().KeyCtrl) {
                m_zoom += wheel * 0.1f;
                m_zoom = std::clamp(m_zoom, 0.25f, 4.0f);
            } else if (ImGui::GetIO().KeyShift) {
                m_scrollX -= wheel * 20.0f;
            } else {
                m_scrollY -= wheel * 2.0f;
            }
            m_scrollY = std::clamp(m_scrollY, 0.0f, (float)(TOTAL_KEYS - 20));
        }
        
        draw->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);
        
        draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                           IM_COL32(30, 30, 35, 255));
        
        RenderPianoKeys(draw, canvas_pos, canvas_size, note_height);
        RenderGrid(draw, canvas_pos, canvas_size, beat_width, note_height);
        RenderNotes(draw, canvas_pos, canvas_size, beat_width, note_height);
        
        if (m_isSelecting) {
            ImVec2 p1 = m_selectionStart;
            ImVec2 p2 = ImGui::GetMousePos();
            draw->AddRect(p1, p2, IM_COL32(100, 150, 255, 200), 0.0f, 0, 2.0f);
            draw->AddRectFilled(p1, p2, IM_COL32(100, 150, 255, 30));
        }
        
        HandleInput(canvas_pos, canvas_size, beat_width, note_height);
        
        draw->PopClipRect();
    }
    
    void RenderPianoKeys(ImDrawList* draw, ImVec2 canvas_pos, ImVec2 canvas_size, float note_height)
    {
        int start_key = (int)m_scrollY;
        int end_key = std::min((int)(m_scrollY + canvas_size.y / note_height) + 1, TOTAL_KEYS);
        
        // White keys
        for (int i = start_key; i < end_key; i++) {
            int note = TOTAL_KEYS - 1 - i;
            float y = canvas_pos.y + (i - m_scrollY) * note_height;
            
            if (!IsBlackKey(note % 12)) {
                draw->AddRectFilled(ImVec2(canvas_pos.x, y), 
                                   ImVec2(canvas_pos.x + PIANO_WIDTH, y + note_height), 
                                   IM_COL32(45, 45, 48, 255));
                draw->AddRect(ImVec2(canvas_pos.x, y), 
                             ImVec2(canvas_pos.x + PIANO_WIDTH, y + note_height), 
                             IM_COL32(25, 25, 28, 255));
            }
        }
        
        // Black keys
        float black_key_width = PIANO_WIDTH * 0.6f;
        for (int i = start_key; i < end_key; i++) {
            int note = TOTAL_KEYS - 1 - i;
            float y = canvas_pos.y + (i - m_scrollY) * note_height;
            
            if (IsBlackKey(note % 12)) {
                draw->AddRectFilled(ImVec2(canvas_pos.x, y), 
                                   ImVec2(canvas_pos.x + black_key_width, y + note_height), 
                                   IM_COL32(20, 20, 22, 255));
                draw->AddRect(ImVec2(canvas_pos.x, y), 
                             ImVec2(canvas_pos.x + black_key_width, y + note_height), 
                             IM_COL32(10, 10, 12, 255));
            }
        }
        
        // C notes labels
        for (int i = start_key; i < end_key; i++) {
            int note = TOTAL_KEYS - 1 - i;
            float y = canvas_pos.y + (i - m_scrollY) * note_height;
            
            if (note % 12 == 0) {
                char buf[8];
                snprintf(buf, sizeof(buf), "C%d", (note / 12) - 1);
                draw->AddText(ImVec2(canvas_pos.x + black_key_width + 3, y + 2), 
                             IM_COL32(120, 120, 125, 255), buf);
            }
        }
        
        draw->AddLine(ImVec2(canvas_pos.x + PIANO_WIDTH, canvas_pos.y),
                     ImVec2(canvas_pos.x + PIANO_WIDTH, canvas_pos.y + canvas_size.y),
                     IM_COL32(60, 60, 65, 255), 2.0f);
    }
    
    void RenderGrid(ImDrawList* draw, ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width, float note_height)
    {
        float grid_start_x = canvas_pos.x + PIANO_WIDTH;
        float grid_width = canvas_size.x - PIANO_WIDTH;
        
        int start_key = (int)m_scrollY;
        int end_key = std::min((int)(m_scrollY + canvas_size.y / note_height) + 1, TOTAL_KEYS);
        
        // Draw alternating row backgrounds based on key type
        for (int i = start_key; i < end_key; i++) {
            int note = TOTAL_KEYS - 1 - i;
            float y = canvas_pos.y + (i - m_scrollY) * note_height;
            
            bool is_black = IsBlackKey(note % 12);
            ImU32 bg_col = is_black ? IM_COL32(26, 26, 30, 255) : IM_COL32(30, 30, 35, 255);
            
            draw->AddRectFilled(ImVec2(grid_start_x, y), 
                               ImVec2(canvas_pos.x + canvas_size.x, y + note_height), 
                               bg_col);
        }
        
        // Horizontal lines
        for (int i = start_key; i < end_key; i++) {
            int note = TOTAL_KEYS - 1 - i;
            float y = canvas_pos.y + (i - m_scrollY) * note_height;
            
            bool is_black = IsBlackKey(note % 12);
            ImU32 line_col = is_black ? IM_COL32(35, 35, 38, 255) : IM_COL32(38, 38, 42, 255);
            
            draw->AddLine(ImVec2(grid_start_x, y), 
                         ImVec2(canvas_pos.x + canvas_size.x, y), 
                         line_col);
        }
        
        // Vertical lines
        int visible_beats = (int)((grid_width + m_scrollX) / beat_width) + 2;
        int start_beat = (int)(m_scrollX / beat_width);
        
        for (int i = 0; i < visible_beats; i++) {
            int beat = start_beat + i;
            float x = grid_start_x + beat * beat_width - m_scrollX;
            
            if (x < grid_start_x) continue;
            
            ImU32 line_col = (beat % 4 == 0) ? IM_COL32(55, 55, 60, 255) : IM_COL32(42, 42, 46, 255);
            float thickness = (beat % 4 == 0) ? 1.5f : 1.0f;
            
            draw->AddLine(ImVec2(x, canvas_pos.y), 
                         ImVec2(x, canvas_pos.y + canvas_size.y), 
                         line_col, thickness);
        }
    }
    
    void RenderNotes(ImDrawList* draw, ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width, float note_height)
    {
        float grid_start_x = canvas_pos.x + PIANO_WIDTH;
        MidiPattern& pattern = GetCurrentPattern();
        
        ImVec4 button_col = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        ImVec4 button_active_col = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        ImVec4 button_hovered_col = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
        
        const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        
        for (size_t i = 0; i < pattern.notes.size(); i++) {
            const auto& note = pattern.notes[i];
            bool selected = std::find(m_selectedNotes.begin(), m_selectedNotes.end(), i) != m_selectedNotes.end();
            
            float note_y = canvas_pos.y + (TOTAL_KEYS - 1 - note.key - m_scrollY) * note_height;
            float note_x = grid_start_x + note.start * beat_width - m_scrollX;
            float note_w = note.length * beat_width;
            
            if (note_x + note_w < grid_start_x || note_x > canvas_pos.x + canvas_size.x) continue;
            if (note_y + note_height < canvas_pos.y || note_y > canvas_pos.y + canvas_size.y) continue;
            
            float draw_x = std::max(note_x, grid_start_x);
            float draw_w = std::min(note_x + note_w, canvas_pos.x + canvas_size.x) - draw_x;
            
            if (draw_w <= 0) continue;
            
            ImVec4 col = selected ? button_active_col : button_col;
            ImU32 note_col = ImGui::ColorConvertFloat4ToU32(col);
            ImU32 border_col = ImGui::ColorConvertFloat4ToU32(button_hovered_col);
            
            ImVec2 p1(draw_x, note_y + 1);
            ImVec2 p2(draw_x + draw_w, note_y + note_height - 1);
            
            draw->AddRectFilled(p1, p2, note_col, 2.0f);
            draw->AddRect(p1, p2, border_col, 2.0f, 0, 1.5f);
            
            if (note_w > 25 && note_height >= 12.0f) {
                int note_in_octave = note.key % 12;
                int octave = (note.key / 12) - 1;
                char buf[8];
                snprintf(buf, sizeof(buf), "%s%d", note_names[note_in_octave], octave);
                
                ImVec2 text_size = ImGui::CalcTextSize(buf);
                float text_x = std::max(draw_x + 4, note_x + 4);
                float text_y = note_y + (note_height - text_size.y) * 0.5f;
                
                if (text_x + text_size.x < draw_x + draw_w) {
                    draw->AddText(ImVec2(text_x, text_y), IM_COL32(200, 200, 205, 255), buf);
                }
            }
            
            if (selected && note_x + note_w >= grid_start_x && note_height >= 8.0f) {
                float handle_x = std::max(note_x + note_w - 4, grid_start_x);
                ImVec2 handle_p1(handle_x, note_y + 3);
                ImVec2 handle_p2(note_x + note_w - 1, note_y + note_height - 3);
                ImU32 handle_col = ImGui::ColorConvertFloat4ToU32(button_hovered_col);
                draw->AddRectFilled(handle_p1, handle_p2, handle_col);
            }
        }
    }
    
    void HandleInput(ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width, float note_height)
    {
        // IMPORTANT: Always check for mouse release first, regardless of hover state
        // This prevents the selection box from getting stuck when mouse leaves the window
        if (ImGui::IsMouseReleased(0)) {
            m_isDragging = false;
            m_isResizing = false;
            m_isSelecting = false;
            m_draggedNote = -1;
            m_originalPositions.clear();
        }
        
        // Allow continued interaction during dragging/resizing/selecting even if not hovered
        if (!ImGui::IsWindowHovered() && !m_isDragging && !m_isResizing && !m_isSelecting)
            return;

        ImVec2 mouse_pos = ImGui::GetMousePos();
        float grid_start_x = canvas_pos.x + PIANO_WIDTH;
        
        bool in_grid = mouse_pos.x >= grid_start_x && 
                    mouse_pos.x < canvas_pos.x + canvas_size.x &&
                    mouse_pos.y >= canvas_pos.y && 
                    mouse_pos.y < canvas_pos.y + canvas_size.y;
        
        // Handle middle mouse button dragging (panning)
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            
            if (ImGui::GetIO().KeyCtrl) {
                m_verticalZoom += delta.y * -0.01f;
                m_verticalZoom = std::clamp(m_verticalZoom, 0.5f, 3.0f);
            } else {
                m_scrollX -= delta.x;
                m_scrollY -= delta.y / note_height;
            }
            
            m_scrollX = std::max(0.0f, m_scrollX);
            m_scrollY = std::clamp(m_scrollY, 0.0f, (float)(TOTAL_KEYS - 20));
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            return; // Exit early to prevent other cursor changes
        }
        
        float relative_x = mouse_pos.x - grid_start_x + m_scrollX;
        float relative_y = mouse_pos.y - canvas_pos.y;
        
        double beat_pos = relative_x / beat_width;
        int note_num = TOTAL_KEYS - 1 - (int)((relative_y + m_scrollY * note_height) / note_height);
        
        // Update cursor based on current state and hover
        UpdateCursor(canvas_pos, canvas_size, beat_width, note_height, beat_pos, note_num, in_grid);
        
        if (ImGui::IsMouseClicked(0) && in_grid) {
            if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
                return;
            }
            
            int clicked = FindNoteAt(beat_pos, note_num);
            
            if (m_currentTool == Tool::Draw) {
                if (clicked == -1) {
                    ClearSelection();
                    CreateNote(beat_pos, note_num);
                    m_selectedNotes.push_back(GetCurrentPattern().notes.size() - 1);
                    m_isDragging = true;
                    m_draggedNote = m_selectedNotes[0];
                    m_dragStartPos = mouse_pos;
                    StoreOriginalPositions();
                } else {
                    MidiPattern& pattern = GetCurrentPattern();
                    float note_end = pattern.notes[clicked].start + pattern.notes[clicked].length;
                    float end_x = grid_start_x + note_end * beat_width - m_scrollX;
                    
                    if (mouse_pos.x > end_x - 6 && mouse_pos.x < end_x + 2) {
                        m_isResizing = true;
                        m_draggedNote = clicked;
                    } else {
                        if (!ImGui::GetIO().KeyCtrl) {
                            ClearSelection();
                        }
                        m_selectedNotes.push_back(clicked);
                        m_isDragging = true;
                        m_draggedNote = clicked;
                        m_dragStartPos = mouse_pos;
                        StoreOriginalPositions();
                    }
                }
            } else if (m_currentTool == Tool::Select) {
                if (clicked != -1) {
                    MidiPattern& pattern = GetCurrentPattern();
                    float note_end = pattern.notes[clicked].start + pattern.notes[clicked].length;
                    float end_x = grid_start_x + note_end * beat_width - m_scrollX;
                    
                    if (mouse_pos.x > end_x - 6 && mouse_pos.x < end_x + 2) {
                        m_isResizing = true;
                        m_draggedNote = clicked;
                    } else {
                        bool already_selected = std::find(m_selectedNotes.begin(), m_selectedNotes.end(), clicked) != m_selectedNotes.end();
                        if (!ImGui::GetIO().KeyCtrl && !already_selected) {
                            ClearSelection();
                        }
                        if (!already_selected) {
                            m_selectedNotes.push_back(clicked);
                        }
                        m_isDragging = true;
                        m_draggedNote = clicked;
                        m_dragStartPos = mouse_pos;
                        StoreOriginalPositions();
                    }
                } else {
                    if (!ImGui::GetIO().KeyCtrl) {
                        ClearSelection();
                    }
                    m_isSelecting = true;
                    m_selectionStart = mouse_pos;
                }
            } else if (m_currentTool == Tool::Erase) {
                if (clicked != -1) {
                    MidiPattern& pattern = GetCurrentPattern();
                    pattern.notes.erase(pattern.notes.begin() + clicked);
                }
            }
        }
        
        if (m_isDragging && ImGui::IsMouseDragging(0)) {
            MidiPattern& pattern = GetCurrentPattern();
            ImVec2 mouse_delta = ImVec2(mouse_pos.x - m_dragStartPos.x, mouse_pos.y - m_dragStartPos.y);
            double delta_beat = mouse_delta.x / beat_width;
            int delta_pitch = -(int)(mouse_delta.y / note_height);
            
            double min_original_start = DBL_MAX;
            for (int idx : m_selectedNotes) {
                if (idx < (int)m_originalPositions.size()) {
                    min_original_start = std::min(min_original_start, m_originalPositions[idx].start);
                }
            }
            
            if (min_original_start + delta_beat < 0.0) {
                delta_beat = -min_original_start;
            }
            
            for (int idx : m_selectedNotes) {
                if (idx >= 0 && idx < (int)pattern.notes.size() && idx < (int)m_originalPositions.size()) {
                    double new_start = m_originalPositions[idx].start + delta_beat;
                    pattern.notes[idx].start = SnapToGrid(new_start);
                    pattern.notes[idx].key = std::clamp(m_originalPositions[idx].pitch + delta_pitch, 0, 127);
                }
            }
        }
        
        if (m_isResizing && ImGui::IsMouseDragging(0)) {
            MidiPattern& pattern = GetCurrentPattern();
            if (m_draggedNote >= 0 && m_draggedNote < (int)pattern.notes.size()) {
                double new_length = beat_pos - pattern.notes[m_draggedNote].start;
                double snapped_length = SnapToGrid(new_length);
                
                double min_length = 4.0 / 32.0;
                snapped_length = std::max(min_length, snapped_length);
                
                pattern.notes[m_draggedNote].length = snapped_length;
                m_lastNoteLength = snapped_length;
            }
        }
        
        if (m_isSelecting && ImGui::IsMouseDragging(0)) {
            m_selectionEnd = mouse_pos;
            
            MidiPattern& pattern = GetCurrentPattern();
            ImVec2 box_min(std::min(m_selectionStart.x, m_selectionEnd.x),
                        std::min(m_selectionStart.y, m_selectionEnd.y));
            ImVec2 box_max(std::max(m_selectionStart.x, m_selectionEnd.x),
                        std::max(m_selectionStart.y, m_selectionEnd.y));
            
            for (size_t i = 0; i < pattern.notes.size(); i++) {
                const auto& note = pattern.notes[i];
                float note_x1 = grid_start_x + note.start * beat_width - m_scrollX;
                float note_x2 = note_x1 + note.length * beat_width;
                float note_y = canvas_pos.y + (TOTAL_KEYS - 1 - note.key - m_scrollY) * note_height;
                
                bool in_box = note_x2 >= box_min.x && note_x1 <= box_max.x &&
                            note_y + note_height >= box_min.y && note_y <= box_max.y;
                
                if (in_box) {
                    if (std::find(m_selectedNotes.begin(), m_selectedNotes.end(), i) == m_selectedNotes.end()) {
                        m_selectedNotes.push_back(i);
                    }
                }
            }
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
            DeleteSelectedNotes();
        }
        
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
            SelectAllNotes();
        }
    }
    
    void UpdateCursor(ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width, float note_height, 
                     double beat_pos, int note_num, bool in_grid)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

        // Active interactions override hover cursors
        if (m_isDragging) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            return;
        }
        
        if (m_isResizing) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            return;
        }
        
        if (m_isSelecting) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            return;
        }
        
        // Hover cursors when not actively interacting
        if (!in_grid) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            return;
        }
        
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float grid_start_x = canvas_pos.x + PIANO_WIDTH;
        int hovered_note = FindNoteAt(beat_pos, note_num);
        
        if (hovered_note != -1) {
            // Check if hovering over resize handle
            MidiPattern& pattern = GetCurrentPattern();
            float note_end = pattern.notes[hovered_note].start + pattern.notes[hovered_note].length;
            float end_x = grid_start_x + note_end * beat_width - m_scrollX;
            
            if (mouse_pos.x > end_x - 6 && mouse_pos.x < end_x + 2) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            } else {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
        }
    }
    
    void CreateNote(double beat_pos, int note_num)
    {
        double snapped_start = SnapToGrid(beat_pos);
        
        MidiNote note;
        note.key = std::clamp(note_num, 0, 127);
        note.start = snapped_start;
        note.length = m_lastNoteLength;
        note.velocity = 100;
        
        GetCurrentPattern().notes.push_back(note);
    }
    
    int FindNoteAt(double beat_pos, int note_num)
    {
        MidiPattern& pattern = GetCurrentPattern();
        for (int i = pattern.notes.size() - 1; i >= 0; i--) {
            const auto& note = pattern.notes[i];
            if (note.key == note_num &&
                beat_pos >= note.start && 
                beat_pos <= note.start + note.length) {
                return i;
            }
        }
        return -1;
    }
    
    double SnapToGrid(double value)
    {
        double snap_size = 4.0 / m_snap;
        return std::round(value / snap_size) * snap_size;
    }
    
    void StoreOriginalPositions()
    {
        m_originalPositions.clear();
        MidiPattern& pattern = GetCurrentPattern();
        m_originalPositions.resize(pattern.notes.size());
        for (size_t i = 0; i < pattern.notes.size(); i++) {
            m_originalPositions[i] = {pattern.notes[i].start, (int)pattern.notes[i].key};
        }
    }
    
    void ClearSelection()
    {
        m_selectedNotes.clear();
    }
    
    void SelectAllNotes()
    {
        MidiPattern& pattern = GetCurrentPattern();
        m_selectedNotes.clear();
        for (size_t i = 0; i < pattern.notes.size(); i++) {
            m_selectedNotes.push_back(i);
        }
    }
    
    void DeleteSelectedNotes()
    {
        MidiPattern& pattern = GetCurrentPattern();
        std::sort(m_selectedNotes.rbegin(), m_selectedNotes.rend());
        for (int idx : m_selectedNotes) {
            if (idx >= 0 && idx < (int)pattern.notes.size()) {
                pattern.notes.erase(pattern.notes.begin() + idx);
            }
        }
        m_selectedNotes.clear();
    }
    
    bool IsBlackKey(int note_in_octave)
    {
        return note_in_octave == 1 || note_in_octave == 3 || 
               note_in_octave == 6 || note_in_octave == 8 || note_in_octave == 10;
    }
    
    const char* GetSnapName(int snap)
    {
        switch (snap) {
            case 4: return "1/4";
            case 8: return "1/8";
            case 16: return "1/16";
            case 32: return "1/32";
            default: return "Unknown";
        }
    }
};

class SongTimeline : public Naui::Panel
{
public:
    SongTimeline(void) : Naui::Panel("Song Timeline")
    {
        m_zoom = 1.0f;
        m_scrollX = 0.0f;
        m_scrollY = 0.0f;
        m_snap = 16;
        
        // Ensure we have at least one track
        if (Core::tracks.empty()) {
            Track track{};
            track.type = TrackType_Midi;
            track.color = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
            track.name = "MIDI 1";
            Core::tracks.push_back(track);
        }
    } 

protected:
    void OnRender(void) override
    {
        RenderToolbar();
        ImGui::Separator();
        
        ImGui::BeginChild("##TimelineMain", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderTimeline();
        ImGui::EndChild();
    }

private:
    struct TrackUIState {
        float height;
        bool collapsed;
    };
    
    std::vector<TrackUIState> m_trackUIStates;
    
    // View state
    float m_zoom;
    float m_scrollX;
    float m_scrollY;
    int m_snap;
    
    // Interaction state
    enum class Tool { Select, Draw, Cut, Delete };
    Tool m_currentTool = Tool::Select;
    
    bool m_isDragging = false;
    bool m_isResizing = false;
    bool m_isResizingLeft = false;
    bool m_isSelecting = false;
    int m_draggedTrack = -1;
    int m_draggedInstance = -1;
    ImVec2 m_dragStartPos;
    ImVec2 m_selectionStart;
    ImVec2 m_selectionEnd;

    struct OriginalInstancePosition {
        double start;
        int trackIndex;
    };
    std::vector<OriginalInstancePosition> m_originalInstancePositions;
    std::vector<std::pair<int, int>> m_selectedInstances; // track index, instance index
    
    // Constants
    static constexpr float TRACK_HEIGHT_DEFAULT = 80.0f;
    static constexpr float TRACK_HEIGHT_MIN = 40.0f;
    static constexpr float TRACK_HEIGHT_MAX = 200.0f;
    static constexpr float TRACK_HEADER_WIDTH = 200.0f;
    static constexpr float RULER_HEIGHT = 30.0f;
    static constexpr float BEAT_WIDTH_BASE = 50.0f;
    
    void EnsureUIStates()
    {
        while (m_trackUIStates.size() < Core::tracks.size()) {
            TrackUIState state;
            state.height = TRACK_HEIGHT_DEFAULT;
            state.collapsed = false;
            m_trackUIStates.push_back(state);
        }
    }
    
    void RenderToolbar()
    {
        // Transport controls
        if (ImGui::Button(Core::isPlayingTimeline ? "Stop" : "Play")) {
            SoundDevice::StopAllNotes();
            Core::isPlayingTimeline  = !Core::isPlayingTimeline ;
        }
        ImGui::SameLine();
        if (ImGui::Button("<<")) {
            Core::timelinePosition = 0.0;
            SoundDevice::StopAllNotes();
        }
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // BPM
        ImGui::Text("BPM:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        if (ImGui::DragFloat("##BPM", &Core::bpm, 0.1f, 20.0f, 999.0f, "%.1f"))
        {
            for (auto& track : Core::tracks)
            {
                for (auto& block : track.blocks)
                {
                    if (track.type != TrackType_Sample)
                        continue;
                    const Sample& sample = Core::samples[block.sampleIndex];
                    const double sec_per_beat = 60.0 / Core::bpm;
                    const double sample_duration_sec = (double)sample.frameCount / sample.sampleRate;
                    const double sample_duration_beats = sample_duration_sec / sec_per_beat;
                    const double available_duration = (sample_duration_beats - block.startOffset) * block.stretchScale;
                    block.length = std::min(block.length, available_duration);
                }
            }
        }        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Tool selection
        if (ImGui::RadioButton("Select", m_currentTool == Tool::Select)) {
            m_currentTool = Tool::Select;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Draw", m_currentTool == Tool::Draw)) {
            m_currentTool = Tool::Draw;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Cut", m_currentTool == Tool::Cut)) {
            m_currentTool = Tool::Cut;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Delete", m_currentTool == Tool::Delete)) {
            m_currentTool = Tool::Delete;
        }
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Snap
        ImGui::Text("Snap:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        const char* snap_preview = GetSnapName(m_snap);
        if (ImGui::BeginCombo("##Snap", snap_preview, ImGuiComboFlags_NoArrowButton)) {
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
        
        // Zoom
        ImGui::Text("Zoom:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        ImGui::SliderFloat("##Zoom", &m_zoom, 0.25f, 4.0f, "%.2fx");
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Add track
        if (ImGui::Button("+ Audio Track")) {
            Track track{};
            track.type = TrackType_Sample;
            track.color = ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
            track.name = "Audio " + std::to_string(Core::tracks.size() + 1);
            Core::tracks.push_back(track);
        }
        ImGui::SameLine();
        if (ImGui::Button("+ MIDI Track")) {
            Track track{};
            track.type = TrackType_Midi;
            track.color = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
            track.name = "MIDI " + std::to_string(Core::tracks.size() + 1);
            Core::tracks.push_back(track);
        }
    }
    
    void RenderTimeline()
    {
        EnsureUIStates();
        
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        
        float beat_width = BEAT_WIDTH_BASE * m_zoom;
        
        // Handle scrolling
        if (ImGui::IsWindowHovered()) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (ImGui::GetIO().KeyCtrl) {
                m_zoom += wheel * 0.1f;
                m_zoom = std::clamp(m_zoom, 0.25f, 4.0f);
            } else if (ImGui::GetIO().KeyShift) {
                m_scrollX -= wheel * 30.0f;
            } else {
                m_scrollY -= wheel * 20.0f;
            }
            m_scrollX = std::max(0.0f, m_scrollX);
            m_scrollY = std::max(0.0f, m_scrollY);
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            m_scrollX -= delta.x;
            m_scrollY -= delta.y;
            m_scrollX = std::max(0.0f, m_scrollX);
            m_scrollY = std::max(0.0f, m_scrollY);
        }
        
        // Background
        draw->AddRectFilled(canvas_pos, 
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                           IM_COL32(25, 25, 28, 255));
        
        // Draw track headers and lanes
        float timeline_start_y = canvas_pos.y;
        
        ImGui::PushClipRect(canvas_pos, 
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                           true);
        
        float current_y = timeline_start_y - m_scrollY + RULER_HEIGHT;
        for (size_t i = 0; i < Core::tracks.size(); i++) {
            if (current_y > canvas_pos.y + canvas_size.y) break;
            if (current_y + m_trackUIStates[i].height < timeline_start_y) {
                current_y += m_trackUIStates[i].height;
                continue;
            }
            
            RenderTrack(draw, canvas_pos, canvas_size, beat_width, i, current_y);
            current_y += m_trackUIStates[i].height;
        }

        ImGui::PopClipRect();

        ImGui::PushClipRect(ImVec2(canvas_pos.x + TRACK_HEADER_WIDTH, canvas_pos.y), 
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                           true);
        // Draw ruler
        RenderRuler(draw, canvas_pos, canvas_size, beat_width);
        RenderPlayhead(draw, canvas_pos, canvas_size, beat_width);

        // Draw playhead
        ImGui::PopClipRect();

        // Handle input
        HandleInput(canvas_pos, canvas_size, beat_width);

        if (m_isSelecting) {
            ImVec2 p1 = m_selectionStart;
            ImVec2 p2 = ImGui::GetMousePos();
            draw->AddRect(p1, p2, IM_COL32(100, 150, 255, 200), 0.0f, 0, 2.0f);
            draw->AddRectFilled(p1, p2, IM_COL32(100, 150, 255, 30));
        }
        
        // Update playback
        /*if (m_isPlaying) {
            Core::timelinePosition += ImGui::GetIO().DeltaTime * (Core::bpm / 60.0);
        }*/
    }
    
    void RenderRuler(ImDrawList* draw, ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width)
    {
        float ruler_start_x = canvas_pos.x + TRACK_HEADER_WIDTH;
        float ruler_width = canvas_size.x - TRACK_HEADER_WIDTH;
        
        draw->AddRectFilled(ImVec2(canvas_pos.x, canvas_pos.y),
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + RULER_HEIGHT),
                           IM_COL32(35, 35, 38, 255));
        
        draw->AddRectFilled(ImVec2(canvas_pos.x, canvas_pos.y),
                           ImVec2(canvas_pos.x + TRACK_HEADER_WIDTH, canvas_pos.y + RULER_HEIGHT),
                           IM_COL32(30, 30, 33, 255));
        
        draw->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + RULER_HEIGHT),
                     ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + RULER_HEIGHT),
                     IM_COL32(60, 60, 65, 255), 1.0f);
        
        int visible_beats = (int)((ruler_width + m_scrollX) / beat_width) + 2;
        int start_beat = (int)(m_scrollX / beat_width);
        
        for (int i = 0; i < visible_beats; i++) {
            int beat = start_beat + i;
            float x = ruler_start_x + beat * beat_width - m_scrollX;
            
            if (x < ruler_start_x - beat_width) continue;
            if (x > canvas_pos.x + canvas_size.x) break;
            
            bool is_measure = (beat % 4 == 0);
            ImU32 line_col = is_measure ? IM_COL32(100, 100, 105, 255) : IM_COL32(60, 60, 65, 255);
            float line_height = is_measure ? RULER_HEIGHT * 0.6f : RULER_HEIGHT * 0.3f;
            
            draw->AddLine(ImVec2(x, canvas_pos.y + RULER_HEIGHT - line_height),
                         ImVec2(x, canvas_pos.y + RULER_HEIGHT),
                         line_col, 1.0f);
            
            if (is_measure) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", (beat / 4) + 1);
                draw->AddText(ImVec2(x + 3, canvas_pos.y + 5), 
                             IM_COL32(180, 180, 185, 255), buf);
            }
        }
    }
    
    void RenderTrack(ImDrawList* draw, ImVec2 canvas_pos, ImVec2 canvas_size, 
                    float beat_width, size_t track_index, float y_pos)
    {
        Track& track = Core::tracks[track_index];
        TrackUIState& ui = m_trackUIStates[track_index];
        float track_start_x = canvas_pos.x + TRACK_HEADER_WIDTH;
        
        // Track lane background
        ImU32 lane_color = (track_index % 2 == 0) ? 
                          IM_COL32(28, 28, 31, 255) : IM_COL32(25, 25, 28, 255);
        draw->AddRectFilled(ImVec2(track_start_x, y_pos),
                           ImVec2(canvas_pos.x + canvas_size.x, y_pos + ui.height),
                           lane_color);
        
        draw->AddLine(ImVec2(track_start_x, y_pos + ui.height),
                     ImVec2(canvas_pos.x + canvas_size.x, y_pos + ui.height),
                     IM_COL32(40, 40, 45, 255), 1.0f);
        
        // Grid lines
        int visible_beats = (int)((canvas_size.x - TRACK_HEADER_WIDTH + m_scrollX) / beat_width) + 2;
        int start_beat = (int)(m_scrollX / beat_width);
        
        for (int i = 0; i < visible_beats; i++) {
            int beat = start_beat + i;
            float x = track_start_x + beat * beat_width - m_scrollX;
            
            if (beat % 4 == 0) {
                draw->AddLine(ImVec2(x, y_pos),
                             ImVec2(x, y_pos + ui.height),
                             IM_COL32(40, 40, 45, 100), 1.0f);
            }
        }
        
        // Draw instances
        if (track.type == TrackType_Midi) {
            for (size_t i = 0; i < track.blocks.size(); i++) {
                RenderMidiInstance(draw, canvas_pos, canvas_size, beat_width, 
                                  track_index, i, y_pos, track_start_x, ui);
            }
        } else {
            for (size_t i = 0; i < track.blocks.size(); i++) {
                RenderSampleInstance(draw, canvas_pos, canvas_size, beat_width, 
                                    track_index, i, y_pos, track_start_x, ui);
            }
        }

        if (ImGui::GetDragDropPayload())
        {
            ImGui::SetCursorScreenPos(ImVec2(track_start_x, y_pos));
            ImGui::InvisibleButton(("##track_drop_" + std::to_string(track_index)).c_str(), 
                                ImVec2(canvas_size.x - TRACK_HEADER_WIDTH, ui.height));

            if (ImGui::BeginDragDropTarget())
            {
                if (track.type == TrackType_Sample)
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAMPLE_INDEX"))
                    {
                        uint16_t sample_idx = *(const uint16_t*)payload->Data;
                        
                        ImVec2 mouse_pos = ImGui::GetMousePos();
                        double beat_pos = (mouse_pos.x - track_start_x + m_scrollX) / beat_width;
                        double snapped_start = SnapToGrid(beat_pos);
                        
                        if (sample_idx < Core::samples.size())
                        {
                            TimelineBlock instance;
                            instance.start = snapped_start;
                            
                            const Sample& sample = Core::samples[sample_idx];
                            double sec_per_beat = 60.0 / Core::bpm;
                            double sample_duration_sec = (double)sample.frameCount / sample.sampleRate;
                            instance.length = sample_duration_sec / sec_per_beat;
                            
                            instance.startOffset = 0.0;
                            instance.stretchScale = 1.0;
                            instance.sampleIndex = sample_idx;
                            
                            track.blocks.push_back(instance);
                        }
                    }
                }
                else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PATTERN_INDEX"))
                {
                    uint16_t pattern_idx = *(const uint16_t*)payload->Data;
                    
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    double beat_pos = (mouse_pos.x - track_start_x + m_scrollX) / beat_width;
                    double snapped_start = SnapToGrid(beat_pos);
                    
                    if (pattern_idx < Core::patterns.size())
                    {
                        TimelineBlock instance;
                        instance.start = snapped_start;
                        
                        instance.length = 4.0;
                        instance.startOffset = 0.0;
                        instance.patternIndex = pattern_idx;
                        
                        track.blocks.push_back(instance);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        // Track header
        RenderTrackHeader(draw, canvas_pos, track_index, y_pos);
    }

    void RenderTrackContextMenu(size_t track_index)
    {
        Track& track = Core::tracks[track_index];
        
        ImGui::Text("Track Properties");
        ImGui::Separator();
        
        // Track name
        char nameBuffer[256];
        strncpy(nameBuffer, track.name.c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            track.name = nameBuffer;
        }
        
        // Track color
        ImGui::SetNextItemWidth(200);
        ImGui::ColorEdit4("Color", (float*)&track.color, 
                         ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
        
        ImGui::Separator();
        
        // Track height
        ImGui::SetNextItemWidth(200);
        float height = m_trackUIStates[track_index].height;
        if (ImGui::SliderFloat("Height", &height, TRACK_HEIGHT_MIN, TRACK_HEIGHT_MAX, "%.0f px")) {
            m_trackUIStates[track_index].height = height;
        }
        
        ImGui::Separator();
        
        // Track type (read-only)
        const char* type_str = (track.type == TrackType_Sample) ? "Audio Track" : "MIDI Track";
        ImGui::TextDisabled("Type: %s", type_str);
        
        // Instrument selection for MIDI tracks
        if (track.type == TrackType_Midi) {
            ImGui::Separator();
            ImGui::Text("Instrument");
            
            // Display current instrument
            if (track.instrument.plugin) {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", track.instrument.plugin->GetName());
                
                if (ImGui::Button("Open Editor", ImVec2(200, 0))) {
                    PluginLoader::OpenEffect(track.instrument);
                }
                
                if (ImGui::Button("Remove Instrument", ImVec2(200, 0))) {
                    PluginLoader::UnloadEffect(track.instrument);
                }
            } else {
                ImGui::TextDisabled("No instrument loaded");
            }
            
            // Plugin browser
            if (ImGui::BeginMenu("Load Instrument")) {
                int plugin_id = 0;
                for (const auto& path : Core::settings.pluginPaths) {
                    if (!std::filesystem::exists(path)) continue;
                    
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                        if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                            ImGui::PushID(plugin_id++);
                            if (ImGui::MenuItem(entry.path().filename().replace_extension().string().c_str())) {
                                Effect &effect = track.instrument;
                                if (effect.plugin)
                                    PluginLoader::UnloadEffect(effect);
                                PluginLoader::LoadEffect(track.instrument, entry.path());
                            }
                            ImGui::PopID();
                        }
                    }
                }
                ImGui::EndMenu();
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Delete Track", nullptr, false, Core::tracks.size() > 1)) {
            DeleteTrack(track_index);
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Clear All Blocks")) {
            track.blocks.clear();
        }
    }
    
    void DeleteTrack(size_t track_index)
    {
        if (track_index >= Core::tracks.size() || Core::tracks.size() <= 1) return;
        
        Track& track = Core::tracks[track_index];
        
        if (track.instrument.plugin) {
            PluginLoader::UnloadEffect(track.instrument);
        }
        
        for (auto& effect : track.effects) {
            if (effect.plugin) {
                PluginLoader::UnloadEffect(effect);
            }
        }
        
        Core::tracks.erase(Core::tracks.begin() + track_index);
        
        if (track_index < m_trackUIStates.size()) {
            m_trackUIStates.erase(m_trackUIStates.begin() + track_index);
        }
        
        m_selectedInstances.erase(
            std::remove_if(m_selectedInstances.begin(), m_selectedInstances.end(),
                [track_index](const std::pair<int, int>& sel) {
                    return sel.first == (int)track_index;
                }),
            m_selectedInstances.end()
        );
        
        for (auto& [t_idx, i_idx] : m_selectedInstances) {
            if (t_idx > (int)track_index) {
                t_idx--;
            }
        }
    }
    
    void RenderTrackHeader(ImDrawList* draw, ImVec2 canvas_pos, size_t track_index, float y_pos)
    {
        TrackUIState& ui = m_trackUIStates[track_index];
        Track& track = Core::tracks[track_index];
        
        draw->AddRectFilled(ImVec2(canvas_pos.x, y_pos),
                           ImVec2(canvas_pos.x + TRACK_HEADER_WIDTH, y_pos + ui.height),
                           IM_COL32(35, 35, 38, 255));
        
        draw->AddLine(ImVec2(canvas_pos.x + TRACK_HEADER_WIDTH, y_pos),
                     ImVec2(canvas_pos.x + TRACK_HEADER_WIDTH, y_pos + ui.height),
                     IM_COL32(50, 50, 55, 255), 2.0f);
        
        ImU32 track_col = ImGui::ColorConvertFloat4ToU32(track.color);
        draw->AddRectFilled(ImVec2(canvas_pos.x + 5, y_pos + 8),
                           ImVec2(canvas_pos.x + 10, y_pos + ui.height - 8),
                           track_col);
        
        if (ImGui::IsMouseHoveringRect(ImVec2(canvas_pos.x, y_pos), ImVec2(canvas_pos.x + TRACK_HEADER_WIDTH, y_pos + ui.height))) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("TrackContextMenu");
            else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                PluginLoader::OpenEffect(track.instrument);
            }
        }
        if (ImGui::BeginPopupContextItem("TrackContextMenu")) {
            RenderTrackContextMenu(track_index);
            ImGui::EndPopup();
        }

        ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x + 15, y_pos + 10));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 225, 255));
        ImGui::Text("%s", track.name.c_str());
        ImGui::PopStyleColor();
        
        const char* type_str = (track.type == TrackType_Sample) ? "Audio" : "MIDI";
        ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x + 15, y_pos + 28));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(140, 140, 145, 255));
        ImGui::TextUnformatted(type_str);
        ImGui::PopStyleColor();
        
        if (ui.height >= 60.0f) {
            float button_y = y_pos + 50;
            
            ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x + 10, button_y));
            ImGui::PushID(track_index * 1000);
            
            ImGui::PushStyleColor(ImGuiCol_Button, track.muted ? 
                                 IM_COL32(180, 100, 50, 180) : IM_COL32(60, 60, 65, 180));
            if (ImGui::SmallButton("M")) {
                track.muted = !track.muted;
            }
            ImGui::PopStyleColor();
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, track.solo ? 
                                 IM_COL32(100, 180, 100, 180) : IM_COL32(60, 60, 65, 180));
            if (ImGui::SmallButton("S")) {
                track.solo = !track.solo;
            }
            ImGui::PopStyleColor();
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, track.armed ? 
                                 IM_COL32(200, 80, 80, 180) : IM_COL32(60, 60, 65, 180));
            if (ImGui::SmallButton("R")) {
                track.armed = !track.armed;
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            
            ImGui::PopID();
        }
    }
    
    void RenderMidiInstance(ImDrawList* draw, ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width,
                           size_t track_index, size_t instance_index, float y_pos, float track_start_x,
                           TrackUIState& ui)
    {
        Track& track = Core::tracks[track_index];
        TimelineBlock& instance = track.blocks[instance_index];
        
        float inst_x = track_start_x + instance.start * beat_width - m_scrollX;
        float inst_width = instance.length * beat_width;
        float inst_y = y_pos + 5;
        float inst_height = ui.height - 10;
        float inst_start_offset = instance.startOffset * beat_width;

        if (inst_x + inst_width < track_start_x || inst_x > canvas_pos.x + canvas_size.x) return;
        
        bool selected = IsInstanceSelected(track_index, instance_index);
        
        ImVec4 base_color = track.color;
        if (track.muted) {
            base_color.x *= 0.4f;
            base_color.y *= 0.4f;
            base_color.z *= 0.4f;
        }
        if (selected) {
            base_color.x = std::min(1.0f, base_color.x * 1.3f);
            base_color.y = std::min(1.0f, base_color.y * 1.3f);
            base_color.z = std::min(1.0f, base_color.z * 1.3f);
        }
        
        ImU32 inst_col = ImGui::ColorConvertFloat4ToU32(base_color);
        ImU32 border_col = ImGui::ColorConvertFloat4ToU32(
            ImVec4(base_color.x * 1.5f, base_color.y * 1.5f, base_color.z * 1.5f, 1.0f));
        
        draw->AddRectFilled(ImVec2(inst_x, inst_y),
                           ImVec2(inst_x + inst_width, inst_y + inst_height),
                           inst_col, 3.0f);
        
        draw->AddRect(ImVec2(inst_x, inst_y),
                     ImVec2(inst_x + inst_width, inst_y + inst_height),
                     border_col, 3.0f, 0, selected ? 2.5f : 1.5f);
        
        // Pattern name
        ImGui::PushClipRect(ImVec2(inst_x, inst_y), ImVec2(inst_x + inst_width, inst_y + inst_height), true);
        if (inst_width > 40.0f && inst_height > 20.0f) {
            ImVec2 text_pos(inst_x + 8, inst_y + 6);
            draw->AddText(text_pos, IM_COL32(255, 255, 255, 255), Core::patterns[instance.patternIndex].name.c_str());
        }
        ImGui::PopClipRect();
        
        if (inst_width > 20.0f && inst_height > 25.0f && 
            instance.patternIndex < (int)Core::patterns.size()) {
            
            MidiPattern& pattern = Core::patterns[instance.patternIndex];
            
            if (!pattern.notes.empty()) {
                // Find min and max pitch in the pattern
                int min_pitch = 127;
                int max_pitch = 0;
                for (const auto& note : pattern.notes) {
                    min_pitch = std::min(min_pitch, (int)note.key);
                    max_pitch = std::max(max_pitch, (int)note.key + 1);
                }
                
                // Add padding to pitch range
                int pitch_range = max_pitch - min_pitch;
                if (pitch_range < 8) pitch_range = 8; // Minimum range of one octave
                
                // Calculate note rendering area (leave space for text at top)
                float note_area_y = inst_y + 20;
                float note_area_height = inst_height - 22;
                
                if (note_area_height > 10.0f) {
                    // Render notes
                    ImU32 note_col = IM_COL32(255, 255, 255, 180);
                    
                    for (const auto& note : pattern.notes) {
                        // Calculate note position relative to instance
                        float note_x = inst_x - inst_start_offset + (note.start / instance.length) * inst_width;
                        float note_w = (note.length / instance.length) * inst_width;
                        
                        // Clamp to instance bounds
                        if (note_x < inst_x) {
                            note_w -= (inst_x - note_x);
                            note_x = inst_x;
                        }
                        if (note_x + note_w > inst_x + inst_width) {
                            note_w = inst_x + inst_width - note_x;
                        }
                        
                        if (note_w < 1.0f) continue;
                        
                        // Calculate vertical position (inverted so higher notes are at top)
                        float pitch_normalized = (float)(note.key - min_pitch) / (float)pitch_range;
                        float note_height = std::max(2.0f, note_area_height / (pitch_range + 1));
                        float note_y = note_area_y + note_area_height - (pitch_normalized * note_area_height) - note_height;
                        
                        // Clamp to instance bounds
                        if (note_y < note_area_y) note_y = note_area_y;
                        if (note_y + note_height > note_area_y + note_area_height) {
                            note_height = note_area_y + note_area_height - note_y;
                        }
                        
                        if (note_height < 1.0f) continue;
                        
                        // Draw the note
                        draw->AddRectFilled(ImVec2(note_x, note_y),
                                        ImVec2(note_x + note_w, note_y + note_height),
                                        note_col, 1.0f);
                    }
                }
            }
        }

        // Resize handles
        if (selected && inst_height > 20.0f) {
            draw->AddRectFilled(ImVec2(inst_x, inst_y + 5),
                               ImVec2(inst_x + 4, inst_y + inst_height - 5),
                               IM_COL32(255, 255, 255, 200));
            draw->AddRectFilled(ImVec2(inst_x + inst_width - 4, inst_y + 5),
                               ImVec2(inst_x + inst_width, inst_y + inst_height - 5),
                               IM_COL32(255, 255, 255, 200));
        }
    }
    
    void RenderSampleInstance(ImDrawList* draw, ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width,
                         size_t track_index, size_t instance_index, float y_pos, float track_start_x,
                         TrackUIState& ui)
    {
        Track& track = Core::tracks[track_index];
        TimelineBlock& instance = track.blocks[instance_index];
        
        float inst_x = track_start_x + instance.start * beat_width - m_scrollX;
        float inst_width = instance.length * beat_width;
        float inst_y = y_pos + 5;
        float inst_height = ui.height - 10;
        
        if (inst_x + inst_width < track_start_x || inst_x > canvas_pos.x + canvas_size.x) return;
        
        bool selected = IsInstanceSelected(track_index, instance_index);
        
        ImVec4 base_color = track.color;
        if (track.muted) {
            base_color.x *= 0.4f;
            base_color.y *= 0.4f;
            base_color.z *= 0.4f;
        }
        if (selected) {
            base_color.x = std::min(1.0f, base_color.x * 1.3f);
            base_color.y = std::min(1.0f, base_color.y * 1.3f);
            base_color.z = std::min(1.0f, base_color.z * 1.3f);
        }
        
        ImU32 inst_col = ImGui::ColorConvertFloat4ToU32(base_color);
        ImU32 border_col = ImGui::ColorConvertFloat4ToU32(
            ImVec4(base_color.x * 1.5f, base_color.y * 1.5f, base_color.z * 1.5f, 1.0f));
        
        draw->AddRectFilled(ImVec2(inst_x, inst_y),
                        ImVec2(inst_x + inst_width, inst_y + inst_height),
                        inst_col, 3.0f);
        
        draw->AddRect(ImVec2(inst_x, inst_y),
                    ImVec2(inst_x + inst_width, inst_y + inst_height),
                    border_col, 3.0f, 0, selected ? 2.5f : 1.5f);
        
        // Render actual waveform
        if (inst_width > 20.0f && inst_height > 20.0f && 
            instance.sampleIndex < Core::samples.size()) {
            
            const Sample& sample = Core::samples[instance.sampleIndex];
            
            if (sample.frames && sample.frameCount > 0) {
                // Calculate waveform rendering parameters
                const float sec_per_beat = 60.0f / Core::bpm;
                const double sample_duration_sec = (double)sample.frameCount / sample.sampleRate;
                const double sample_duration_beats = sample_duration_sec / sec_per_beat;
                
                // Calculate which part of the sample is visible in this instance
                const double start_offset_beats = instance.startOffset;
                const double visible_duration_beats = instance.length;
                
                // Convert to sample positions
                const double start_offset_sec = start_offset_beats * sec_per_beat / instance.stretchScale;
                const double visible_duration_sec = visible_duration_beats * sec_per_beat / instance.stretchScale;
                
                const uint64_t start_frame = (uint64_t)(start_offset_sec * sample.sampleRate);
                const uint64_t end_frame = std::min(
                    (uint64_t)((start_offset_sec + visible_duration_sec) * sample.sampleRate),
                    sample.frameCount
                );
                
                if (start_frame < sample.frameCount && end_frame > start_frame) {
                    // Waveform rendering area
                    const float waveform_y = inst_y + 22;
                    const float waveform_height = inst_height - 24;
                    const float waveform_center = waveform_y + waveform_height / 2.0f;
                    
                    // Calculate how many pixels per sample
                    const uint64_t frame_count = end_frame - start_frame;
                    const int num_pixels = (int)inst_width;
                    
                    ImU32 wave_col = IM_COL32(255, 255, 255, 180);
                    ImU32 wave_fill_col = IM_COL32(255, 255, 255, 40);
                    
                    // Use different rendering strategies based on zoom level
                    if (frame_count < (uint64_t)num_pixels * 2) {
                        // High zoom: draw individual samples with interpolation
                        float prev_x = inst_x;
                        float prev_y_top = waveform_center;
                        float prev_y_bottom = waveform_center;
                        
                        for (int px = 0; px < num_pixels; px++) {
                            const float x = std::floor(inst_x + px);
                            if (x < track_start_x || x > canvas_pos.x + canvas_size.x) continue;
                            
                            // Calculate sample position for this pixel
                            const double t = (double)px / num_pixels;
                            const uint64_t frame_idx = start_frame + (uint64_t)(t * frame_count);
                            
                            if (frame_idx >= sample.frameCount) continue;
                            
                            float sample_value = 0.0f;
                            
                            if (sample.type == SampleType::Stereo) {
                                // Average both channels
                                const uint64_t offset = frame_idx * 2;
                                sample_value = (sample.frames[offset] + sample.frames[offset + 1]) * 0.5f;
                            } else {
                                sample_value = sample.frames[frame_idx];
                            }
                            
                            // Clamp and scale
                            sample_value = std::clamp(sample_value, -1.0f, 1.0f);
                            const float y_offset = sample_value * (waveform_height * 0.45f);
                            const float y_top = waveform_center - std::abs(y_offset);
                            const float y_bottom = waveform_center + std::abs(y_offset);
                            
                            // Draw waveform shape
                            if (px > 0) {
                                draw->AddLine(ImVec2(prev_x, prev_y_top), ImVec2(x, y_top), wave_col, 1.0f);
                                draw->AddLine(ImVec2(prev_x, prev_y_bottom), ImVec2(x, y_bottom), wave_col, 1.0f);
                            }
                            
                            // Fill between top and bottom
                            if (y_bottom - y_top > 1.0f) {
                                draw->AddLine(ImVec2(x, y_top), ImVec2(x, y_bottom), wave_fill_col, 1.0f);
                            }
                            
                            prev_x = x;
                            prev_y_top = y_top;
                            prev_y_bottom = y_bottom;
                        }
                    } else {
                        // Low zoom: draw min/max peaks per pixel
                        for (int px = 0; px < num_pixels; px++) {
                            const float x = std::floor(inst_x + px);
                            if (x < track_start_x || x > canvas_pos.x + canvas_size.x) continue;
                            
                            // Calculate sample range for this pixel
                            const double t_start = (double)px / num_pixels;
                            const double t_end = (double)(px + 1) / num_pixels;
                            const uint64_t frame_start = start_frame + (uint64_t)(t_start * frame_count);
                            const uint64_t frame_end = std::min(
                                start_frame + (uint64_t)(t_end * frame_count),
                                end_frame
                            );
                            
                            // Find min and max in this range
                            float min_val = 0.0f;
                            float max_val = 0.0f;
                            
                            for (uint64_t f = frame_start; f < frame_end && f < sample.frameCount; f++) {
                                float val = 0.0f;
                                
                                if (sample.type == SampleType::Stereo) {
                                    const uint64_t offset = f * 2;
                                    val = (sample.frames[offset] + sample.frames[offset + 1]) * 0.5f;
                                } else {
                                    val = sample.frames[f];
                                }
                                
                                min_val = std::min(min_val, val);
                                max_val = std::max(max_val, val);
                            }
                            
                            // Clamp values
                            min_val = std::clamp(min_val, -1.0f, 1.0f);
                            max_val = std::clamp(max_val, -1.0f, 1.0f);
                            
                            // Calculate y positions
                            const float y_min = waveform_center - (min_val * waveform_height * 0.45f);
                            const float y_max = waveform_center - (max_val * waveform_height * 0.45f);
                            
                            // Draw vertical line from min to max
                            if (std::abs(y_max - y_min) > 0.5f) {
                                draw->AddLine(ImVec2(x, y_max), ImVec2(x, y_min), wave_col, 1.0f);
                            } else {
                                // Single point for very quiet sections
                                draw->AddLine(ImVec2(x, waveform_center), ImVec2(x, waveform_center), wave_col, 1.0f);
                            }
                        }
                    }
                    
                    // Draw center line
                    draw->AddLine(ImVec2(inst_x, waveform_center), 
                                ImVec2(inst_x + inst_width, waveform_center),
                                IM_COL32(255, 255, 255, 60), 1.0f);
                }
            }
        }
        
        // Sample name

        ImGui::PushClipRect(ImVec2(inst_x, inst_y), ImVec2(inst_x + inst_width, inst_y + inst_height), true);
        if (inst_width > 40.0f && inst_height > 20.0f && 
            instance.sampleIndex < Core::samples.size()) {
            ImVec2 text_pos(inst_x + 8, inst_y + 6);
            draw->AddText(text_pos, IM_COL32(255, 255, 255, 255), 
                        Core::samples[instance.sampleIndex].name.c_str());
        }
        ImGui::PopClipRect();
        
        // Resize handles
        if (selected && inst_height > 20.0f) {
            draw->AddRectFilled(ImVec2(inst_x, inst_y + 5),
                            ImVec2(inst_x + 4, inst_y + inst_height - 5),
                            IM_COL32(255, 255, 255, 200));
            draw->AddRectFilled(ImVec2(inst_x + inst_width - 4, inst_y + 5),
                            ImVec2(inst_x + inst_width, inst_y + inst_height - 5),
                            IM_COL32(255, 255, 255, 200));
        }
    }
    
    void RenderPlayhead(ImDrawList* draw, ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width)
    {
        float playhead_x = canvas_pos.x + TRACK_HEADER_WIDTH + 
                          Core::timelinePosition * beat_width - m_scrollX;
        draw->AddLine(ImVec2(playhead_x, canvas_pos.y),
                        ImVec2(playhead_x, canvas_pos.y + canvas_size.y),
                        IM_COL32(255, 200, 100, 255), 2.0f);
        
        ImVec2 tri[3] = {
            ImVec2(playhead_x, canvas_pos.y + RULER_HEIGHT),
            ImVec2(playhead_x - 6, canvas_pos.y + RULER_HEIGHT - 10),
            ImVec2(playhead_x + 6, canvas_pos.y + RULER_HEIGHT - 10)
        };
        draw->AddTriangleFilled(tri[0], tri[1], tri[2], IM_COL32(255, 200, 100, 255));
    }
    

    void HandleInput(ImVec2 canvas_pos, ImVec2 canvas_size, float beat_width)
    {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float track_start_x = canvas_pos.x + TRACK_HEADER_WIDTH;
        float timeline_start_y = canvas_pos.y + RULER_HEIGHT;
        
        // ========== MOUSE RELEASE (always check this first) ==========
        if (ImGui::IsMouseReleased(0)) {
            ResetDragState();
        }
        
        // ========== RULER CLICK (timeline scrubbing) ==========
        static bool is_scrubbing = false;
        if (HandleRulerClick(mouse_pos, canvas_pos, track_start_x, beat_width, is_scrubbing)) {
            return; // Early exit if we're scrubbing
        }
        
        // ========== EARLY EXIT CHECKS ==========
        bool is_interacting = m_isDragging || m_isResizing || m_isResizingLeft || m_isSelecting;
        if (!ImGui::IsWindowHovered() && !is_interacting) {
            return;
        }
        
        bool in_timeline = IsInTimelineArea(mouse_pos, canvas_pos, canvas_size, track_start_x, timeline_start_y);
        if (!in_timeline && !is_interacting) {
            return;
        }
        
        // ========== CALCULATE MOUSE POSITION ==========
        double beat_pos = (mouse_pos.x - track_start_x + m_scrollX) / beat_width;
        int track_idx = GetTrackAtY(mouse_pos.y, timeline_start_y);
        
        // ========== UPDATE CURSOR ==========
        UpdateCursor(mouse_pos, beat_pos, track_idx, track_start_x, beat_width);
        
        // ========== KEYBOARD SHORTCUTS ==========
        if (HandleKeyboardShortcuts()) {
            return;
        }
        
        // ========== MOUSE INTERACTIONS ==========
        if (ImGui::IsMouseClicked(0)) {
            HandleMouseClick(mouse_pos, beat_pos, track_idx, track_start_x, beat_width);
        }
        
        if (ImGui::IsMouseDragging(0)) {
            HandleMouseDrag(mouse_pos, beat_pos, track_idx, timeline_start_y, track_start_x, beat_width);
        }
    }

    void ResetDragState()
    {
        m_isDragging = false;
        m_isResizing = false;
        m_isResizingLeft = false;
        m_isSelecting = false;
        m_draggedTrack = -1;
        m_draggedInstance = -1;
        m_originalInstancePositions.clear();
    }
    
    bool HandleRulerClick(ImVec2 mouse_pos, ImVec2 canvas_pos, float track_start_x, 
                         float beat_width, bool& is_scrubbing)
    {
        bool in_ruler = mouse_pos.y >= canvas_pos.y && 
                       mouse_pos.y < canvas_pos.y + RULER_HEIGHT &&
                       mouse_pos.x >= track_start_x;
        
        if (ImGui::IsMouseClicked(0) && in_ruler && ImGui::IsWindowHovered()) {
            is_scrubbing = true;
            Core::isDraggingHandle = true;
            SoundDevice::StopAllNotes();
        }
        
        if (is_scrubbing) {
            if (ImGui::IsMouseReleased(0)) {
                is_scrubbing = false;
                Core::isDraggingHandle = false;
                return false;
            }
            
            double beat_pos = (mouse_pos.x - track_start_x + m_scrollX) / beat_width;
            Core::timelinePosition = SnapToGrid(std::max(0.0, beat_pos));
            return true;
        }
        
        return false;
    }
    
    bool IsInTimelineArea(ImVec2 mouse_pos, ImVec2 canvas_pos, ImVec2 canvas_size,
                         float track_start_x, float timeline_start_y)
    {
        return mouse_pos.x >= track_start_x && 
               mouse_pos.x < canvas_pos.x + canvas_size.x &&
               mouse_pos.y >= timeline_start_y && 
               mouse_pos.y < canvas_pos.y + canvas_size.y;
    }
    
    void UpdateCursor(ImVec2 mouse_pos, double beat_pos, int track_idx,
                     float track_start_x, float beat_width)
    {
        // Default cursor
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        
        // Override based on current state
        if (m_isResizing || m_isResizingLeft) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            return;
        }
        
        if (m_isDragging) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            return;
        }
        
        if (m_isSelecting) {
            return; // Keep arrow
        }
        
        // Check hover state
        auto [inst_track, inst_idx] = FindInstanceAt(beat_pos, track_idx);
        
        if (inst_track >= 0 && inst_idx >= 0) {
            Track& track = Core::tracks[inst_track];
            TimelineBlock& block = track.blocks[inst_idx];
            
            float inst_start_x = track_start_x + block.start * beat_width - m_scrollX;
            float inst_end_x = inst_start_x + block.length * beat_width;
            
            // Check resize handles
            if ((mouse_pos.x >= inst_start_x - 2 && mouse_pos.x <= inst_start_x + 6) ||
                (mouse_pos.x >= inst_end_x - 6 && mouse_pos.x <= inst_end_x + 2)) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
            else {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
        }
    }
    
    bool HandleKeyboardShortcuts()
    {
        // Delete selected instances
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
            DeleteSelectedInstances();
            return true;
        }
        
        // Select all
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
            SelectAllInstances();
            return true;
        }
        
        // Play/Stop
        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            Core::isPlayingTimeline = !Core::isPlayingTimeline;
            SoundDevice::StopAllNotes();
            return true;
        }
        
        return false;
    }
    
    void HandleMouseClick(ImVec2 mouse_pos, double beat_pos, int track_idx,
                         float track_start_x, float beat_width)
    {
        // Ignore if popup is open
        if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
            return;
        }
        
        auto [inst_track, inst_idx] = FindInstanceAt(beat_pos, track_idx);
        
        switch (m_currentTool) {
            case Tool::Draw:
                HandleDrawToolClick(inst_track, inst_idx, track_idx, beat_pos, mouse_pos, track_start_x, beat_width);
                break;
            case Tool::Select:
                HandleSelectToolClick(inst_track, inst_idx, track_idx, mouse_pos, track_start_x, beat_width);
                break;
            case Tool::Delete:
                HandleDeleteToolClick(inst_track, inst_idx);
                break;
            case Tool::Cut:
                // TODO: Implement cut tool
                break;
        }
    }
    
    void HandleDrawToolClick(int inst_track, int inst_idx, int track_idx, double beat_pos,
                            ImVec2 mouse_pos, float track_start_x, float beat_width)
    {
        // Clicking empty space - create new instance
        if (inst_idx == -1 && track_idx >= 0) {

            if (Core::tracks[track_idx].type == TrackType_Sample && Core::samples.empty())
                return;
            ClearSelection();
            CreateInstance(track_idx, beat_pos);
            
            // Start dragging the new instance
            Track& track = Core::tracks[track_idx];
            int new_idx = track.blocks.size() - 1;
            StartDragging(track_idx, new_idx, mouse_pos);
            return;
        }
        
        // Clicking on instance
        if (inst_track >= 0 && inst_idx >= 0) {
            if (TryStartResize(inst_track, inst_idx, mouse_pos, track_start_x, beat_width)) {
                return; // Started resizing
            }
            
            // Handle instance selection and dragging
            bool already_selected = IsInstanceSelected(inst_track, inst_idx);
            
            if (already_selected) {
                // Start dragging all selected instances
                StartDragging(inst_track, inst_idx, mouse_pos);
                StoreOriginalInstancePositions();
            } else {
                // New selection
                if (!ImGui::GetIO().KeyCtrl) {
                    ClearSelection();
                }
                if (ImGui::GetIO().KeyCtrl) {
                    m_selectedInstances.push_back({inst_track, inst_idx});
                }
                StartDragging(inst_track, inst_idx, mouse_pos);
            }
        }
    }
    
    void HandleSelectToolClick(int inst_track, int inst_idx, int track_idx,
                              ImVec2 mouse_pos, float track_start_x, float beat_width)
    {
        // Clicking on instance
        if (inst_track >= 0 && inst_idx >= 0) {
            if (TryStartResize(inst_track, inst_idx, mouse_pos, track_start_x, beat_width)) {
                return; // Started resizing
            }
            
            // Handle selection
            bool already_selected = IsInstanceSelected(inst_track, inst_idx);
            
            if (!ImGui::GetIO().KeyCtrl && !already_selected) {
                ClearSelection();
            }
            
            if (!already_selected) {
                m_selectedInstances.push_back({inst_track, inst_idx});
            }
            
            StartDragging(inst_track, inst_idx, mouse_pos);
            StoreOriginalInstancePositions();
        }
        // Clicking empty space - start box selection
        else {
            if (!ImGui::GetIO().KeyCtrl) {
                ClearSelection();
            }
            m_isSelecting = true;
            m_selectionStart = mouse_pos;
        }
    }
    
    void HandleDeleteToolClick(int inst_track, int inst_idx)
    {
        if (inst_track >= 0 && inst_idx >= 0) {
            Track& track = Core::tracks[inst_track];
            track.blocks.erase(track.blocks.begin() + inst_idx);
        }
    }
    
    bool TryStartResize(int track_idx, int inst_idx, ImVec2 mouse_pos,
                       float track_start_x, float beat_width)
    {
        Track& track = Core::tracks[track_idx];
        TimelineBlock& block = track.blocks[inst_idx];
        
        float inst_start_x = track_start_x + block.start * beat_width - m_scrollX;
        float inst_end_x = inst_start_x + block.length * beat_width;
        
        // Check left resize handle
        if (mouse_pos.x >= inst_start_x - 2 && mouse_pos.x <= inst_start_x + 6) {
            m_isResizingLeft = true;
            m_draggedTrack = track_idx;
            m_draggedInstance = inst_idx;
            return true;
        }
        
        // Check right resize handle
        if (mouse_pos.x >= inst_end_x - 6 && mouse_pos.x <= inst_end_x + 2) {
            m_isResizing = true;
            m_draggedTrack = track_idx;
            m_draggedInstance = inst_idx;
            return true;
        }
        
        return false;
    }
    
    void StartDragging(int track_idx, int inst_idx, ImVec2 mouse_pos)
    {
        m_isDragging = true;
        m_draggedTrack = track_idx;
        m_draggedInstance = inst_idx;
        m_dragStartPos = mouse_pos;
        
        // Store original position if not in selection
        if (m_selectedInstances.empty()) {
            m_originalInstancePositions.clear();
            m_originalInstancePositions.push_back({
                Core::tracks[track_idx].blocks[inst_idx].start,
                track_idx
            });
        }
    }
    
    void HandleMouseDrag(ImVec2 mouse_pos, double beat_pos, int track_idx,
                        float timeline_start_y, float track_start_x, float beat_width)
    {
        if (m_isResizing) {
            HandleRightResize(beat_pos);
        }
        else if (m_isResizingLeft) {
            HandleLeftResize(beat_pos);
        }
        else if (m_isDragging) {
            HandleInstanceDrag(mouse_pos, track_idx, timeline_start_y, beat_width);
        }
        else if (m_isSelecting) {
            HandleBoxSelection(mouse_pos, timeline_start_y, track_start_x, beat_width);
        }
    }
    
    void HandleRightResize(double beat_pos)
    {
        if (m_draggedTrack < 0 || m_draggedInstance < 0 || 
            m_draggedTrack >= (int)Core::tracks.size()) {
            return;
        }
        
        Track& track = Core::tracks[m_draggedTrack];
        if (m_draggedInstance >= (int)track.blocks.size()) {
            return;
        }
        
        TimelineBlock& block = track.blocks[m_draggedInstance];
        double new_length = beat_pos - block.start;
        double snapped_length = SnapToGrid(new_length);
        
        if (track.type == TrackType_Sample && block.sampleIndex < Core::samples.size()) {
            const Sample& sample = Core::samples[block.sampleIndex];
            const double sec_per_beat = 60.0 / Core::bpm;
            const double sample_duration_sec = (double)sample.frameCount / sample.sampleRate;
            const double sample_duration_beats = sample_duration_sec / sec_per_beat;
            
            // Calculate max length considering start offset and stretch
            const double available_duration = (sample_duration_beats - block.startOffset) * block.stretchScale;
            const double max_length = std::max(SnapToGrid(0.25), available_duration);
            
            snapped_length = std::min(snapped_length, max_length);
        }
        
        block.length = std::max(SnapToGrid(0.25), snapped_length);
    }
    
    void HandleLeftResize(double beat_pos)
    {
        if (m_draggedTrack < 0 || m_draggedInstance < 0 || 
            m_draggedTrack >= (int)Core::tracks.size()) {
            return;
        }
        
        Track& track = Core::tracks[m_draggedTrack];
        if (m_draggedInstance >= (int)track.blocks.size()) {
            return;
        }
        
        TimelineBlock& block = track.blocks[m_draggedInstance];
        double original_end = block.start + block.length;
        double new_start = SnapToGrid(beat_pos);
        
        // Clamp new start
        new_start = std::max(0.0, new_start);
        new_start = std::min(original_end - SnapToGrid(0.25), new_start);
        
        // Calculate offset delta
        double delta = new_start - block.start;
        double new_offset = block.startOffset + delta;
        
        // Prevent negative offset
        if (new_offset < 0.0) {
            delta = -block.startOffset;
            new_start = block.start + delta;
        }
        
        // Apply changes
        block.start = new_start;
        block.length = original_end - new_start;
        block.startOffset = std::max(0.0, block.startOffset + delta);
    }
    
    void HandleInstanceDrag(ImVec2 mouse_pos, int hover_track, 
                           float timeline_start_y, float beat_width)
    {
        ImVec2 delta = ImVec2(mouse_pos.x - m_dragStartPos.x, mouse_pos.y - m_dragStartPos.y);
        double delta_beat = delta.x / beat_width;
        
        // Single instance drag (not in selection)
        if (m_selectedInstances.empty()) {
            HandleSingleInstanceDrag(hover_track, delta_beat, timeline_start_y);
            return;
        }
        
        // Multi-selection drag
        HandleMultiInstanceDrag(hover_track, delta_beat, timeline_start_y);
    }
    
    void HandleSingleInstanceDrag(int hover_track, double delta_beat, float timeline_start_y)
    {
        if (m_draggedTrack < 0 || m_draggedInstance < 0) {
            return;
        }
        
        // Try to move to different track
        if (hover_track >= 0 && hover_track != m_draggedTrack) {
            Track& src_track = Core::tracks[m_draggedTrack];
            Track& dst_track = Core::tracks[hover_track];
            
            if (src_track.type == dst_track.type && m_draggedInstance < (int)src_track.blocks.size()) {
                TimelineBlock block = src_track.blocks[m_draggedInstance];
                src_track.blocks.erase(src_track.blocks.begin() + m_draggedInstance);
                dst_track.blocks.push_back(block);
                m_draggedTrack = hover_track;
                m_draggedInstance = dst_track.blocks.size() - 1;
                m_originalInstancePositions[0].trackIndex = hover_track;
            }
        }
        
        // Update position
        if (!m_originalInstancePositions.empty()) {
            double new_start = m_originalInstancePositions[0].start + delta_beat;
            new_start = SnapToGrid(std::max(0.0, new_start));
            
            Track& track = Core::tracks[m_draggedTrack];
            if (m_draggedInstance < (int)track.blocks.size()) {
                track.blocks[m_draggedInstance].start = new_start;
            }
        }
    }
    
    void HandleMultiInstanceDrag(int hover_track, double delta_beat, float timeline_start_y)
    {
        // Try to move all instances to different track
        TryMoveSelectionToTrack(hover_track);
        
        // Clamp delta to prevent negative positions
        double min_original_start = DBL_MAX;
        for (const auto& orig : m_originalInstancePositions) {
            min_original_start = std::min(min_original_start, orig.start);
        }
        
        if (min_original_start + delta_beat < 0.0) {
            delta_beat = -min_original_start;
        }
        
        // Update all selected instances
        for (size_t i = 0; i < m_selectedInstances.size(); i++) {
            if (i >= m_originalInstancePositions.size()) continue;
            
            const auto& [t_idx, i_idx] = m_selectedInstances[i];
            Track& track = Core::tracks[t_idx];
            
            if (i_idx >= (int)track.blocks.size()) continue;
            
            double new_start = m_originalInstancePositions[i].start + delta_beat;
            new_start = SnapToGrid(std::max(0.0, new_start));
            track.blocks[i_idx].start = new_start;
        }
    }
    
    void TryMoveSelectionToTrack(int hover_track)
    {
        if (hover_track < 0 || m_selectedInstances.empty()) {
            return;
        }
        
        // Check if all selected instances are from same track
        int first_track = m_selectedInstances[0].first;
        for (const auto& [t_idx, _] : m_selectedInstances) {
            if (t_idx != first_track) {
                return; // Mixed tracks, don't move
            }
        }
        
        if (hover_track == first_track) {
            return; // Same track, no move needed
        }
        
        Track& src_track = Core::tracks[first_track];
        Track& dst_track = Core::tracks[hover_track];
        
        if (src_track.type != dst_track.type) {
            return; // Different track types
        }
        
        // Sort by index (descending) to remove from back
        std::vector<std::pair<int, int>> sorted = m_selectedInstances;
        std::sort(sorted.begin(), sorted.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Move all instances
        std::vector<std::pair<int, int>> new_selection;
        for (const auto& [t_idx, i_idx] : sorted) {
            if (i_idx >= (int)src_track.blocks.size()) continue;
            
            TimelineBlock block = src_track.blocks[i_idx];
            src_track.blocks.erase(src_track.blocks.begin() + i_idx);
            dst_track.blocks.push_back(block);
            new_selection.push_back({hover_track, (int)dst_track.blocks.size() - 1});
        }
        
        // Update selection and positions
        m_selectedInstances = new_selection;
        m_draggedTrack = hover_track;
        for (auto& orig : m_originalInstancePositions) {
            orig.trackIndex = hover_track;
        }
    }
    
    void HandleBoxSelection(ImVec2 mouse_pos, float timeline_start_y,
                           float track_start_x, float beat_width)
    {
        m_selectionEnd = mouse_pos;
        
        ImVec2 box_min(std::min(m_selectionStart.x, m_selectionEnd.x),
                      std::min(m_selectionStart.y, m_selectionEnd.y));
        ImVec2 box_max(std::max(m_selectionStart.x, m_selectionEnd.x),
                      std::max(m_selectionStart.y, m_selectionEnd.y));
        
        float current_y = timeline_start_y - m_scrollY;
        
        for (size_t t = 0; t < Core::tracks.size(); t++) {
            Track& track = Core::tracks[t];
            float track_height = m_trackUIStates[t].height;
            
            // Skip if track not in selection box
            if (current_y + track_height < box_min.y || current_y > box_max.y) {
                current_y += track_height;
                continue;
            }
            
            // Check each instance in this track
            for (size_t i = 0; i < track.blocks.size(); i++) {
                const TimelineBlock& block = track.blocks[i];
                
                float inst_x1 = track_start_x + block.start * beat_width - m_scrollX;
                float inst_x2 = inst_x1 + block.length * beat_width;
                float inst_y = current_y + 5;
                float inst_height = track_height - 10;
                
                bool in_box = inst_x2 >= box_min.x && inst_x1 <= box_max.x &&
                             inst_y + inst_height >= box_min.y && inst_y <= box_max.y;
                
                if (in_box && !IsInstanceSelected(t, i)) {
                    m_selectedInstances.push_back({(int)t, (int)i});
                }
            }
            
            current_y += track_height;
        }
    }

    // Also add this new method to select all instances:
    void SelectAllInstances()
    {
        m_selectedInstances.clear();
        for (size_t t = 0; t < Core::tracks.size(); t++) {
            Track& track = Core::tracks[t];
            if (track.type == TrackType_Midi) {
                for (size_t i = 0; i < track.blocks.size(); i++) {
                    m_selectedInstances.push_back({(int)t, (int)i});
                }
            } else {
                for (size_t i = 0; i < track.blocks.size(); i++) {
                    m_selectedInstances.push_back({(int)t, (int)i});
                }
            }
        }
    }
    
    void CreateInstance(int track_idx, double beat_pos)
    {
        if (track_idx < 0 || track_idx >= (int)Core::tracks.size()) return;
        
        double snapped_start = SnapToGrid(beat_pos);
        Track& track = Core::tracks[track_idx];
        
        if (track.type == TrackType_Midi) {
            TimelineBlock instance;
            instance.start = snapped_start;
            instance.length = 4.0;
            instance.startOffset = 0.0;
            instance.patternIndex = Core::currentMidiPattern;
            track.blocks.push_back(instance);
        } else {
            TimelineBlock instance;
            instance.start = snapped_start;
            instance.length = 4.0;
            instance.startOffset = 0.0;
            instance.stretchScale = 1.0;
            instance.sampleIndex = 0;
            track.blocks.push_back(instance);
        }
    }
    
    std::pair<int, int> FindInstanceAt(double beat_pos, int track_idx)
    {
        if (track_idx < 0 || track_idx >= (int)Core::tracks.size()) {
            return {-1, -1};
        }
        
        Track& track = Core::tracks[track_idx];
        
        if (track.type == TrackType_Midi) {
            for (int i = track.blocks.size() - 1; i >= 0; i--) {
                const auto& inst = track.blocks[i];
                if (beat_pos >= inst.start && beat_pos <= inst.start + inst.length) {
                    return {track_idx, i};
                }
            }
        } else {
            for (int i = track.blocks.size() - 1; i >= 0; i--) {
                const auto& inst = track.blocks[i];
                if (beat_pos >= inst.start && beat_pos <= inst.start + inst.length) {
                    return {track_idx, i};
                }
            }
        }
        return {-1, -1};
    }
    
    int GetTrackAtY(float mouse_y, float timeline_start_y)
    {
        float current_y = timeline_start_y - m_scrollY;
        for (size_t i = 0; i < Core::tracks.size(); i++) {
            if (mouse_y >= current_y && mouse_y < current_y + m_trackUIStates[i].height) {
                return i;
            }
            current_y += m_trackUIStates[i].height;
        }
        return -1;
    }
    
    double SnapToGrid(double value)
    {
        if (m_snap == 0) return value;
        double snap_size = 4.0 / m_snap;
        return std::round(value / snap_size) * snap_size;
    }
    
    void StoreOriginalInstancePositions()
    {
        m_originalInstancePositions.clear();
        for (const auto& [t_idx, i_idx] : m_selectedInstances) {
            Track& track = Core::tracks[t_idx];
            double start = 0.0;
            
            if (track.type == TrackType_Midi && i_idx < (int)track.blocks.size()) {
                start = track.blocks[i_idx].start;
            } else if (track.type == TrackType_Sample && i_idx < (int)track.blocks.size()) {
                start = track.blocks[i_idx].start;
            }
            
            m_originalInstancePositions.push_back({start, t_idx});
        }
    }
    
    void ClearSelection()
    {
        m_selectedInstances.clear();
    }
    
    bool IsInstanceSelected(int track_idx, int instance_idx)
    {
        for (const auto& [t_idx, i_idx] : m_selectedInstances) {
            if (t_idx == track_idx && i_idx == instance_idx) {
                return true;
            }
        }
        return false;
    }
    
    void DeleteSelectedInstances()
    {
        // Sort in reverse order to delete from back to front
        std::vector<std::pair<int, int>> sorted = m_selectedInstances;
        std::sort(sorted.rbegin(), sorted.rend());
        
        for (const auto& [t_idx, i_idx] : sorted) {
            if (t_idx >= 0 && t_idx < (int)Core::tracks.size()) {
                Track& track = Core::tracks[t_idx];
                if (track.type == TrackType_Midi && i_idx < (int)track.blocks.size()) {
                    track.blocks.erase(track.blocks.begin() + i_idx);
                } else if (track.type == TrackType_Sample && i_idx < (int)track.blocks.size()) {
                    track.blocks.erase(track.blocks.begin() + i_idx);
                }
            }
        }
        m_selectedInstances.clear();
    }
    
    const char* GetSnapName(int snap)
    {
        switch (snap) {
            case 0: return "Off";
            case 4: return "1/4";
            case 8: return "1/8";
            case 16: return "1/16";
            case 32: return "1/32";
            default: return "Unknown";
        }
    }
};

class PatternRack : public Naui::Panel
{
public:
    PatternRack(void) : Naui::Panel("Pattern Rack")
    {
        m_renamingIndex = -1;
        memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
    }

protected:
    void OnRender(void) override
    {
        for (uint16_t i = 0; i < Core::patterns.size(); i++) {
            MidiPattern& pattern = Core::patterns[i];
            
            ImGui::PushID(i);
            
            // Check if this item is being renamed
            if (m_renamingIndex == (int)i) {
                ImGui::SetNextItemWidth(-60.0f);
                if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), 
                                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                    pattern.name = m_renameBuffer;
                    m_renamingIndex = -1;
                }
                
                // Auto-focus on the input
                if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive()) {
                    m_renamingIndex = -1;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("OK", ImVec2(50, 0))) {
                    pattern.name = m_renameBuffer;
                    m_renamingIndex = -1;
                }
            } else {
                // Normal display with drag-drop
                if (Selectable(pattern.name.c_str())) {
                    Core::currentMidiPattern = i;
                }
                
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    ImGui::SetDragDropPayload("PATTERN_INDEX", &i, sizeof(uint16_t));
                    ImGui::Text("%s", pattern.name.c_str());
                    ImGui::EndDragDropSource();
                }
                
                // Right-click context menu
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Rename")) {
                        m_renamingIndex = i;
                        strncpy(m_renameBuffer, pattern.name.c_str(), sizeof(m_renameBuffer) - 1);
                        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
                    }
                    
                    if (ImGui::MenuItem("Delete", nullptr, false, Core::patterns.size() > 1)) {
                        DeletePattern(i);
                        ImGui::EndPopup();
                        ImGui::PopID();
                        break; // Exit loop after deletion
                    }
                    
                    if (ImGui::MenuItem("Duplicate")) {
                        DuplicatePattern(i);
                    }
                    
                    ImGui::EndPopup();
                }
            }
            
            ImGui::PopID();
        }
        
        if (ImGui::Button("+")) {
            MidiPattern pattern;
            pattern.name = "Pattern " + std::to_string(Core::patterns.size() + 1);
            Core::patterns.push_back(pattern);
        }
    }

private:
    int m_renamingIndex;
    char m_renameBuffer[128];
    
    void DeletePattern(uint16_t index)
    {
        if (Core::patterns.size() <= 1) {
            return; // Don't delete the last pattern
        }
        
        // Remove the pattern
        Core::patterns.erase(Core::patterns.begin() + index);
        
        // Update all track blocks that reference patterns
        for (auto& track : Core::tracks) {
            if (track.type == TrackType_Midi) {
                // Remove blocks that used the deleted pattern
                track.blocks.erase(
                    std::remove_if(track.blocks.begin(), track.blocks.end(),
                        [index](const TimelineBlock& block) {
                            return block.patternIndex == index;
                        }),
                    track.blocks.end()
                );
                
                // Adjust indices for patterns that came after the deleted one
                for (auto& block : track.blocks) {
                    if (block.patternIndex > index) {
                        block.patternIndex--;
                    }
                }
            }
        }
        
        // Adjust current pattern if needed
        if (Core::currentMidiPattern == index) {
            Core::currentMidiPattern = std::min(Core::currentMidiPattern, 
                                               (uint16_t)(Core::patterns.size() - 1));
        } else if (Core::currentMidiPattern > index) {
            Core::currentMidiPattern--;
        }
    }
    
    void DuplicatePattern(uint16_t index)
    {
        if (index >= Core::patterns.size()) return;
        
        MidiPattern newPattern = Core::patterns[index];
        newPattern.name = Core::patterns[index].name + " (Copy)";
        Core::patterns.push_back(newPattern);
    }
};

class SampleRack : public Naui::Panel
{
public:
    SampleRack(void) : Naui::Panel("Sample Rack")
    {
        m_renamingIndex = -1;
        memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
    }

protected:
    void OnRender(void) override
    {
        for (uint16_t i = 0; i < Core::samples.size(); i++) {
            Sample& sample = Core::samples[i];

            ImGui::PushID(i);
            
            // Check if this item is being renamed
            if (m_renamingIndex == (int)i) {
                ImGui::SetNextItemWidth(-60.0f);
                if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), 
                                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                    sample.name = m_renameBuffer;
                    m_renamingIndex = -1;
                }
                
                // Auto-focus and handle deactivation
                if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive()) {
                    m_renamingIndex = -1;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("OK", ImVec2(50, 0))) {
                    sample.name = m_renameBuffer;
                    m_renamingIndex = -1;
                }
            } else {
                // Normal display with drag-drop
                if (Selectable(sample.name.c_str())) {
                    // Handle selection if needed
                }
                
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    ImGui::SetDragDropPayload("SAMPLE_INDEX", &i, sizeof(uint16_t));
                    ImGui::Text("%s", sample.name.c_str());
                    ImGui::EndDragDropSource();
                }
                
                // Right-click context menu
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Rename")) {
                        m_renamingIndex = i;
                        strncpy(m_renameBuffer, sample.name.c_str(), sizeof(m_renameBuffer) - 1);
                        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
                    }
                    
                    if (ImGui::MenuItem("Delete")) {
                        DeleteSample(i);
                        ImGui::EndPopup();
                        ImGui::PopID();
                        break; // Exit loop after deletion
                    }
                    
                    ImGui::Separator();
                    
                    // Display sample info
                    ImGui::TextDisabled("Sample Rate: %d Hz", sample.sampleRate);
                    ImGui::TextDisabled("Channels: %s", 
                                       sample.type == SampleType::Mono ? "Mono" : "Stereo");
                    ImGui::TextDisabled("Frames: %llu", sample.frameCount);
                    
                    ImGui::EndPopup();
                }
            }
            
            ImGui::PopID();
        }
    }

private:
    int m_renamingIndex;
    char m_renameBuffer[128];
    
    void DeleteSample(uint16_t index)
    {
        if (index >= Core::samples.size()) return;
        
        // Free sample data
        Sample& sample = Core::samples[index];
        if (sample.frames)
            SoundDevice::UnloadSample(sample);
        
        // Remove the sample
        Core::samples.erase(Core::samples.begin() + index);
        
        // Update all track blocks that reference samples
        for (auto& track : Core::tracks) {
            if (track.type == TrackType_Sample) {
                // Remove blocks that used the deleted sample
                track.blocks.erase(
                    std::remove_if(track.blocks.begin(), track.blocks.end(),
                        [index](const TimelineBlock& block) {
                            return block.sampleIndex == index;
                        }),
                    track.blocks.end()
                );
                
                // Adjust indices for samples that came after the deleted one
                for (auto& block : track.blocks) {
                    if (block.sampleIndex > index) {
                        block.sampleIndex--;
                    }
                }
            }
        }
    }
};

#include <imgui-knobs.h>

class MixerRack : public Naui::Panel
{
public:
    MixerRack() : Naui::Panel("Mixer Rack"), m_selectedTrack(-1)
    {
        SetMinSize(0.0f, 250.0f);
    }

protected:
    void OnRender() override
    {
        // Calculate strip area width (leave room for effects panel)
        float stripAreaWidth = ImGui::GetContentRegionAvail().x - 250.0f;
        
        ImGui::BeginChild("##MixerStrips", ImVec2(stripAreaWidth, 0), false, 
            ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        
        RenderMasterStrip(m_selectedTrack == -1);

        for (size_t i = 0; i < Core::tracks.size(); i++) {
            ImGui::SameLine();
            RenderChannelStrip(i, i == m_selectedTrack);
        }
        
        ImGui::EndChild();
        
        // Effects panel on the right
        ImGui::SameLine();
        ImGui::BeginChild("##EffectsPanel", ImVec2(0, 0), true);
        RenderEffectsPanel();
        ImGui::EndChild();
    }

private:
    int m_selectedTrack; // Changed to int to support -1 for master
    
    static constexpr float STRIP_WIDTH = 80.0f;
    static constexpr float VU_METER_WIDTH = 24.0f;
    static constexpr float VU_METER_HEIGHT = 150.0f;
    static constexpr int VU_SEGMENTS = 30;

    void RenderMasterStrip(bool isSelected)
    {
        ImGui::PushID(-1); // Master track ID

        ImGui::BeginChild("##MasterStrip", ImVec2(STRIP_WIDTH, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Selection logic
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
            m_selectedTrack = -1;

        MasterTrack& master = Core::masterTrack;

        // Master color bar (distinctive color)
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImU32 masterColor = IM_COL32(200, 100, 50, 255); // Orange/brown for master
        draw->AddRectFilled(pos, ImVec2(pos.x + STRIP_WIDTH - 16, pos.y + 3), masterColor);
        ImGui::Dummy(ImVec2(0, 5));
        
        // Master label (non-editable)
        ImGui::SetNextItemWidth(-1);
        static char masterName[] = "Master";
        ImGui::InputText("##name", masterName, 7, ImGuiInputTextFlags_ReadOnly);
        
        ImGui::Spacing();

        // Pan knob
        ImGui::SetCursorPosX((STRIP_WIDTH - 40) * 0.5f);
        ImGuiKnobs::Knob("Pan", &master.pan, 0.0f, 1.0f, 0.01f, "%.2f", ImGuiKnobVariant_Tick, 40.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Pan: %.2f", master.pan);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Combined VU Meter and Volume Fader
        ImGui::SetCursorPosX((STRIP_WIDTH - VU_METER_WIDTH) * 0.5f);
        DrawVUMeterWithFader(master.peakLeft, master.peakRight, master.volume, 
                            VU_METER_WIDTH, VU_METER_HEIGHT, masterColor);
        
        ImGui::Spacing();
        
        // Volume dB display
        float db = master.volume > 0.0f ? 20.0f * log10f(master.volume) : -60.0f;
        ImGui::Text(" %.1f dB", db);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Mute button (master typically doesn't have solo)
        ImVec4 muteColor = master.muted ? 
            ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        ImVec4 muteHover = master.muted ? 
            ImVec4(1.0f, 0.6f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        
        ImGui::SetCursorPosX((STRIP_WIDTH - 50) * 0.5f);
        
        ImGui::PushStyleColor(ImGuiCol_Button, muteColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, muteHover);
        
        if (ImGui::Button("M", ImVec2(25, 25))) {
            master.muted = !master.muted;
        }
        ImGui::PopStyleColor(2);

        ImGui::EndChild();
        ImGui::PopID();
    }
    
    void RenderChannelStrip(size_t idx, bool isSelected)
    {
        ImGui::PushID(static_cast<int>(idx));

        ImGui::BeginChild("##Strip", ImVec2(STRIP_WIDTH, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Selection logic
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
            m_selectedTrack = idx;

        Track& track = Core::tracks[idx];

        // Channel color bar
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImU32 trackColor = ImGui::ColorConvertFloat4ToU32(track.color);
        draw->AddRectFilled(pos, ImVec2(pos.x + STRIP_WIDTH - 16, pos.y + 3), trackColor);
        ImGui::Dummy(ImVec2(0, 5));
        
        // Channel name
        ImGui::SetNextItemWidth(-1);
        char nameBuffer[256];
        strncpy(nameBuffer, track.name.c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        
        if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer))) {
            track.name = nameBuffer;
        }
        
        ImGui::Spacing();

        // Pan knob
        ImGui::SetCursorPosX((STRIP_WIDTH - 40) * 0.5f);
        ImGuiKnobs::Knob("Pan", &track.pan, 0.0f, 1.0f, 0.01f, "%.2f", ImGuiKnobVariant_Tick, 40.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Pan: %.2f", track.pan);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Combined VU Meter and Volume Fader
        ImGui::SetCursorPosX((STRIP_WIDTH - VU_METER_WIDTH) * 0.5f);
        DrawVUMeterWithFader(track.peakLeft, track.peakRight, track.volume, 
                            VU_METER_WIDTH, VU_METER_HEIGHT, trackColor);
        
        ImGui::Spacing();
        
        // Volume dB display
        float db = track.volume > 0.0f ? 20.0f * log10f(track.volume) : -60.0f;
        ImGui::Text(" %.1f dB", db);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Solo button
        ImVec4 soloColor = track.solo ? 
            ImVec4(1.0f, 0.7f, 0.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        ImVec4 soloHover = track.solo ? 
            ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        
        ImGui::PushStyleColor(ImGuiCol_Button, soloColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, soloHover);
        
        if (ImGui::Button("S", ImVec2(25, 25))) {
            track.solo = !track.solo;
            if (track.solo) {
                // Disable solo on all other tracks
                for (auto& t : Core::tracks)
                    t.solo = false;
                track.solo = true;
            }
        }
        ImGui::PopStyleColor(2);
        
        ImGui::SameLine();
        
        // Mute button
        ImVec4 muteColor = track.muted ? 
            ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        ImVec4 muteHover = track.muted ? 
            ImVec4(1.0f, 0.6f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        
        ImGui::PushStyleColor(ImGuiCol_Button, muteColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, muteHover);
        
        if (ImGui::Button("M", ImVec2(25, 25))) {
            track.muted = !track.muted;
        }
        ImGui::PopStyleColor(2);

        ImGui::EndChild();
        ImGui::PopID();
    }
    
    void DrawVUMeterWithFader(float vuLevelLeft, float vuLevelRight, float& volume, 
                             float width, float height, ImU32 channelColor)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        
        float meterWidth = width / 2.0f - 1.0f;
        
        // Left Channel Background
        draw->AddRectFilled(pos, ImVec2(pos.x + meterWidth, pos.y + height), 
            IM_COL32(30, 30, 30, 255));
        
        // Right Channel Background
        draw->AddRectFilled(ImVec2(pos.x + meterWidth + 2, pos.y), 
            ImVec2(pos.x + width, pos.y + height), 
            IM_COL32(30, 30, 30, 255));
        
        // VU Meter Segments
        float segHeight = height / VU_SEGMENTS;
        float segSpacing = 1.0f;
        
        // Draw Left Channel
        for (int i = 0; i < VU_SEGMENTS; i++) {
            float segLevel = static_cast<float>(i) / VU_SEGMENTS;
            if (segLevel <= vuLevelLeft) {
                ImU32 color;
                if (segLevel > 0.85f)
                    color = IM_COL32(255, 50, 50, 255);    // Red
                else if (segLevel > 0.7f)
                    color = IM_COL32(255, 200, 50, 255);   // Yellow
                else
                    color = IM_COL32(50, 255, 100, 255);   // Green
                
                float y = pos.y + height - (i + 1) * segHeight;
                draw->AddRectFilled(
                    ImVec2(pos.x + 1, y + segSpacing),
                    ImVec2(pos.x + meterWidth - 1, y + segHeight - segSpacing),
                    color
                );
            }
        }
        
        // Draw Right Channel
        for (int i = 0; i < VU_SEGMENTS; i++) {
            float segLevel = static_cast<float>(i) / VU_SEGMENTS;
            if (segLevel <= vuLevelRight) {
                ImU32 color;
                if (segLevel > 0.85f)
                    color = IM_COL32(255, 50, 50, 255);    // Red
                else if (segLevel > 0.7f)
                    color = IM_COL32(255, 200, 50, 255);   // Yellow
                else
                    color = IM_COL32(50, 255, 100, 255);   // Green
                
                float y = pos.y + height - (i + 1) * segHeight;
                draw->AddRectFilled(
                    ImVec2(pos.x + meterWidth + 3, y + segSpacing),
                    ImVec2(pos.x + width - 1, y + segHeight - segSpacing),
                    color
                );
            }
        }
        
        // Volume triangle indicator
        float volumeY = pos.y + height - (volume * height);
        
        // Left pointing triangle
        ImVec2 p1(pos.x + width + 2, volumeY);
        ImVec2 p2(pos.x + width + 10, volumeY - 5);
        ImVec2 p3(pos.x + width + 10, volumeY + 5);
        draw->AddTriangleFilled(p1, p2, p3, channelColor);
        
        // Make it interactive
        ImGui::InvisibleButton("##vumeter", ImVec2(width + 12, height));
        
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 0.0f)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            float newVolume = 1.0f - ((mousePos.y - pos.y) / height);
            volume = std::clamp(newVolume, 0.0f, 1.0f);
        }
        
        // Hover feedback
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
    }
    
    void RenderEffectsPanel()
    {
        // Header
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        if (m_selectedTrack == -1) {
            ImGui::Text("Master Effects");
        } else if (m_selectedTrack >= 0 && m_selectedTrack < (int)Core::tracks.size()) {
            ImGui::Text("%s - Effects", Core::tracks[m_selectedTrack].name.c_str());
        } else {
            ImGui::Text("No Track Selected");
        }
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Get the effects list for the selected track
        std::vector<Effect>* effects = nullptr;
        if (m_selectedTrack == -1) {
            effects = &Core::masterTrack.effects;
        } else if (m_selectedTrack >= 0 && m_selectedTrack < (int)Core::tracks.size()) {
            effects = &Core::tracks[m_selectedTrack].effects;
        }
        
        if (!effects) {
            ImGui::TextDisabled("No track selected");
            return;
        }
        
        // Add effect button
        if (ImGui::Button("+ Add Effect", ImVec2(-1, 0))) {
            ImGui::OpenPopup("AddEffectPopup");
        }
        
        // Add effect popup
        if (ImGui::BeginPopup("AddEffectPopup", ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::Text("Select UVI");
            ImGui::Separator();

            int plugin_id = 0;
            
            for (const auto& path : Core::settings.pluginPaths)
            {
                if (!std::filesystem::exists(path)) continue;
                
                for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
                {
                    if (entry.is_regular_file() && entry.path().extension() == ".dll")
                    {
                        ImGui::PushID(plugin_id++);
                        if (ImGui::Selectable(entry.path().filename().replace_extension().string().c_str()))
                        {
                            Effect new_effect;
                            PluginLoader::LoadEffect(new_effect, entry.path());
                            effects->push_back(new_effect);
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::PopID();
                    }
                }
            }
            
            ImGui::EndPopup();
        }
        
        ImGui::Spacing();
        
        // Effects list
        ImGui::BeginChild("##EffectsList", ImVec2(0, 0), false);
        
        for (int i = 0; i < (int)effects->size(); i++) {
            Effect& effect = (*effects)[i];
            
            ImGui::PushID(i);
            
            // Effect slot background
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 slotSize(ImGui::GetContentRegionAvail().x, 60.0f);
            
            ImU32 bgColor = IM_COL32(45, 45, 50, 255);
            ImU32 borderColor = IM_COL32(60, 60, 70, 255);
            
            draw->AddRectFilled(cursorPos, 
                ImVec2(cursorPos.x + slotSize.x, cursorPos.y + slotSize.y),
                bgColor, 4.0f);
            draw->AddRect(cursorPos, 
                ImVec2(cursorPos.x + slotSize.x, cursorPos.y + slotSize.y),
                borderColor, 4.0f);
            
            ImGui::Dummy(ImVec2(0, 5));
            ImGui::Indent(10.0f);
            
            // Effect name
            if (effect.plugin) {
                ImGui::Text("%s", effect.plugin->GetName());
            } else {
                ImGui::TextDisabled("Empty Slot");
            }
            
            ImGui::Spacing();
            
            // Buttons
            ImGui::BeginDisabled(!effect.plugin);
            if (ImGui::SmallButton("Open")) {
                PluginLoader::OpenEffect(effect);
            }
            ImGui::EndDisabled();
            
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                if (effect.plugin) {
                    PluginLoader::UnloadEffect(effect);
                }
                effects->erase(effects->begin() + i);
                ImGui::Unindent(10.0f);
                ImGui::PopID();
                break; // Exit loop after deletion
            }
            
            // Move up/down buttons
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::SameLine();
            ImGui::BeginDisabled(i == 0);
            if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
                std::swap((*effects)[i], (*effects)[i - 1]);
            }
            ImGui::EndDisabled();
            
            ImGui::SameLine();
            ImGui::BeginDisabled(i == (int)effects->size() - 1);
            if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
                std::swap((*effects)[i], (*effects)[i + 1]);
            }
            ImGui::EndDisabled();
            ImGui::PopStyleVar();
            
            ImGui::Unindent(10.0f);
            ImGui::Dummy(ImVec2(0, 5));
            
            ImGui::PopID();
        }
        
        // Empty state
        if (effects->empty()) {
            ImGui::Spacing();
            ImGui::TextDisabled("No effects loaded");
            ImGui::TextDisabled("Click '+ Add Effect' to add one");
        }
        
        ImGui::EndChild();
    }
};

#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// Icon definitions (use FontAwesome or define your own)
#ifndef ICON_FA_FOLDER
#define ICON_FA_FOLDER "[DIR]"
#define ICON_FA_FILE "[FILE]"
#define ICON_FA_FILE_AUDIO "[AUD]"
#define ICON_FA_MUSIC "[MID]"
#endif

class FileExplorer : public Naui::Panel
{
public:
    FileExplorer(void) : Naui::Panel("File Explorer")
    {
        m_currentPath = fs::current_path();
        strcpy(m_pathInput, m_currentPath.string().c_str());
        RefreshDirectory();
    }

protected:
    void OnRender(void) override
    {
        RenderToolbar();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Main content area with space for info panel
        ImGui::BeginChild("##FileList", ImVec2(0, -160), ImGuiChildFlags_ResizeY);
        RenderFileList();
        ImGui::EndChild();
        
        // Info panel at bottom
        RenderInfoPanel();
    }

private:
    struct FileItem {
        std::string name;
        std::string extension;
        fs::path fullPath;
        bool isDirectory;
        uintmax_t size;
    };

    fs::path m_currentPath;
    std::vector<FileItem> m_files;
    char m_pathInput[512];
    std::vector<fs::path> m_history;
    size_t m_historyIndex = 0;
    
    // Filter options
    char m_searchFilter[256] = "";
    
    // Selected file
    int m_selectedIndex = -1;

    void RenderToolbar()
    {
        // Navigation buttons
        ImGui::BeginDisabled(m_historyIndex == 0);
        if (ImGui::ArrowButton("##Back", ImGuiDir_Left)) {
            NavigateBack();
        }
        ImGui::EndDisabled();
        
        ImGui::SameLine();
        ImGui::BeginDisabled(m_historyIndex >= m_history.size() - 1);
        if (ImGui::ArrowButton("##Forward", ImGuiDir_Right)) {
            NavigateForward();
        }
        ImGui::EndDisabled();
        
        ImGui::SameLine();
        ImGui::BeginDisabled(!m_currentPath.has_parent_path());
        if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) {
            NavigateUp();
        }
        ImGui::EndDisabled();
        
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        
        // Path input
        ImGui::SetNextItemWidth(-80.0f);
        if (ImGui::InputTextWithHint("##Path", "Path...", m_pathInput, sizeof(m_pathInput), 
                            ImGuiInputTextFlags_EnterReturnsTrue)) {
            NavigateToPath(m_pathInput);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Refresh", ImVec2(70, 0))) {
            RefreshDirectory();
        }
        
        // Search
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputTextWithHint("##Search", "Search files...", m_searchFilter, sizeof(m_searchFilter))) {
            RefreshDirectory();
        }
    }

    void RenderFileList()
    {
        // Clean table layout
        if (ImGui::BeginTable("##Files", 3, 
            ImGuiTableFlags_Resizable | 
            ImGuiTableFlags_RowBg | 
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_BordersInnerV)) {
            
            // Setup columns
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.6f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();
            
            // Render files
            ImGuiListClipper clipper;
            clipper.Begin(m_files.size());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto& file = m_files[i];
                    
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    
                    // Icon and name
                    const char* icon = file.isDirectory ? ICON_FA_FOLDER : ICON_FA_FILE;
                    if (!file.isDirectory && IsAudioFile(file.extension)) {
                        icon = ICON_FA_FILE_AUDIO;
                    }
                    if (!file.isDirectory && (file.extension == ".mid" || file.extension == ".midi")) {
                        icon = ICON_FA_MUSIC;
                    }
                    
                    ImGui::PushID(i);
                    bool selected = i == m_selectedIndex;
                    
                    // Make the entire row selectable
                    if (ImGui::Selectable(("##row" + std::to_string(i)).c_str(), 
                                         selected, 
                                         ImGuiSelectableFlags_SpanAllColumns | 
                                         ImGuiSelectableFlags_AllowDoubleClick)) {
                        m_selectedIndex = i;
                        
                        // Handle double-click
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            if (file.isDirectory) {
                                NavigateToPath(file.fullPath.string());
                            } else {
                                OnFileSelected(file.fullPath);
                            }
                        }
                    }
                    
                    // Draw icon and name on top of selectable
                    ImGui::SameLine();
                    ImGui::Text("%s  %s", icon, file.name.c_str());
                    
                    // Type column
                    ImGui::TableNextColumn();
                    if (file.isDirectory) {
                        ImGui::TextDisabled("Folder");
                    } else {
                        ImGui::Text("%s", file.extension.empty() ? "-" : file.extension.c_str());
                    }
                    
                    // Size column
                    ImGui::TableNextColumn();
                    if (!file.isDirectory) {
                        ImGui::Text("%s", FormatFileSize(file.size).c_str());
                    } else {
                        ImGui::TextDisabled("-");
                    }
                    
                    ImGui::PopID();
                }
            }
            
            ImGui::EndTable();
        }
    }

    void RenderInfoPanel()
    {
        ImGui::Separator();
        ImGui::BeginChild("##InfoPanel", ImVec2(0, 0), false);
        
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_files.size()) {
            const auto& file = m_files[m_selectedIndex];
            
            ImGui::Spacing();
            ImGui::Indent(10.0f);
            
            // File name
            ImGui::Text("Name: %s", file.name.c_str());
            
            // Full path
            ImGui::TextWrapped("Path: %s", file.fullPath.string().c_str());
            
            if (!file.isDirectory) {
                // File size
                ImGui::Text("Size: %s", FormatFileSize(file.size).c_str());
                
                // File type
                if (!file.extension.empty()) {
                    ImGui::Text("Type: %s", file.extension.c_str());
                    
                    // Additional info for audio files
                    if (IsAudioFile(file.extension)) {
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Audio File");
                        // Add sample rate, bit depth, etc. if you have audio parsing
                    } else if (file.extension == ".mid" || file.extension == ".midi") {
                        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.4f, 1.0f), "MIDI File");
                    }
                }
            } else {
                ImGui::TextDisabled("Folder");
            }
            
            ImGui::Unindent(10.0f);
        } else {
            ImGui::Spacing();
            ImGui::Indent(10.0f);
            ImGui::TextDisabled("No file selected");
            ImGui::Unindent(10.0f);
        }
        
        ImGui::EndChild();
    }

    void RefreshDirectory()
    {
        m_files.clear();
        m_selectedIndex = -1;
        
        try {
            for (const auto& entry : fs::directory_iterator(m_currentPath)) {
                FileItem item;
                item.fullPath = entry.path();
                item.name = entry.path().filename().string();
                item.isDirectory = entry.is_directory();
                item.extension = item.isDirectory ? "" : entry.path().extension().string();
                
                try {
                    item.size = item.isDirectory ? 0 : fs::file_size(entry.path());
                } catch (...) {
                    item.size = 0;
                }
                
                // Apply search filter
                if (!PassesFilter(item)) continue;
                
                m_files.push_back(item);
            }
            
            // Sort: directories first, then alphabetically
            std::sort(m_files.begin(), m_files.end(), [](const FileItem& a, const FileItem& b) {
                if (a.isDirectory != b.isDirectory) return a.isDirectory;
                return a.name < b.name;
            });
            
        } catch (const fs::filesystem_error& e) {
            // Handle error silently or log
        }
        
        // Update path input
        strcpy(m_pathInput, m_currentPath.string().c_str());
    }

    bool PassesFilter(const FileItem& item)
    {
        // Search filter
        if (strlen(m_searchFilter) > 0) {
            std::string nameLower = item.name;
            std::string filterLower = m_searchFilter;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
            if (nameLower.find(filterLower) == std::string::npos) {
                return false;
            }
        }
        
        return true;
    }

    bool IsAudioFile(const std::string& ext)
    {
        static const std::vector<std::string> audioExts = {
            ".wav", ".mp3", ".flac", ".ogg", ".aif", ".aiff", 
            ".m4a", ".wma", ".aac", ".opus"
        };
        return std::find(audioExts.begin(), audioExts.end(), ext) != audioExts.end();
    }

    std::string FormatFileSize(uintmax_t size)
    {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unit = 0;
        double s = (double)size;
        
        while (s >= 1024 && unit < 3) {
            s /= 1024;
            unit++;
        }
        
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f %s", s, units[unit]);
        return buf;
    }

    void NavigateToPath(const std::string& path)
    {
        try {
            fs::path newPath(path);
            if (fs::exists(newPath) && fs::is_directory(newPath)) {
                // Add to history
                if (m_historyIndex < m_history.size()) {
                    m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
                }
                m_history.push_back(m_currentPath);
                m_historyIndex = m_history.size();
                
                m_currentPath = newPath;
                RefreshDirectory();
            }
        } catch (const fs::filesystem_error& e) {
            // Handle error
        }
    }

    void NavigateUp()
    {
        if (m_currentPath.has_parent_path()) {
            NavigateToPath(m_currentPath.parent_path().string());
        }
    }

    void NavigateBack()
    {
        if (m_historyIndex > 0) {
            m_historyIndex--;
            m_currentPath = m_history[m_historyIndex];
            RefreshDirectory();
        }
    }

    void NavigateForward()
    {
        if (m_historyIndex < m_history.size() - 1) {
            m_historyIndex++;
            m_currentPath = m_history[m_historyIndex];
            RefreshDirectory();
        }
    }

    void OnFileSelected(const fs::path& filePath)
    {
        // Override this or add callback for file selection handling
        // For a DAW, this would trigger sample loading, project opening, etc.
    }
};

class UphonicApp : public Naui::App {
private:
    void OnEnter(void) override {
        LoadIniSettingsFromDisk("Layouts/Default.ini");
        Core::mainWindow = GetPlatformWindow();
        Core::patterns.push_back(MidiPattern());
        Naui::AddPanel<PatternRack>();
        Naui::AddPanel<SampleRack>();
        Naui::AddPanel<MidiEditor>();
        Naui::AddPanel<SongTimeline>();
        Naui::AddPanel<MixerRack>();
        Naui::AddPanel<FileExplorer>();
        SoundDevice::Initialize();
    }
    
    void OnExit(void) override {
        //SaveIniSettingsToDisk("Layouts/Default.ini");
        SoundDevice::Shutdown();
    }

    void OnFileDrop(const char *path) override {
        Core::samples.push_back(SoundDevice::LoadSample(path));
    }

    void OnRender(void) override {
        BeginMainMenuBar();
        if (BeginMenu("File")) {
            if (MenuItem("Export to WAV"))
            {
                // get furthest end
                double end = 0.0;
                for (auto& track : Core::tracks) {
                    for (auto& block : track.blocks) {
                        if (block.start + block.length > end) {
                            end = block.start + block.length;
                        }
                    }
                }
                SoundDevice::ExportToWav("test.wav", 0, end);
            }
            if (MenuItem("Exit"))
                exit(0);

            EndMenu();
        }

        if (BeginMenu("View")) {
            for (auto& [id, panelPtr] : Naui::GetAllPanels()) {
                Naui::Panel& panel = *panelPtr;
                PushID(id);
                if (MenuItem(panel.GetTitle().c_str(), nullptr, panel.IsOpen()))
                    panel.SetOpen(!panel.IsOpen());
                PopID();
            }
                
            EndMenu();
        }
        EndMainMenuBar();

        for (Effect &effect : Core::masterTrack.effects)
        {
            if (effect.window)
                effect.plugin->IdleEditor();
        }
        for (Track &track : Core::tracks)
        {
            for (Effect &effect : track.effects)
            {
                if (effect.window)
                    effect.plugin->IdleEditor();
            }
        }
    }
};

int main() {
    UphonicApp app;
    app.Run();
    return 0;
}