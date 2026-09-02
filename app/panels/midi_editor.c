#define UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH 6.0f
#define UPH_MIDI_EDITOR_DEFAULT_NOTE_LENGTH 4.0
#define UPH_MIDI_EDITOR_DEFAULT_NOTE_VELOCITY 100

#define UPH_MIDI_EDITOR_ZOOM_X_MIN 8.0f
#define UPH_MIDI_EDITOR_ZOOM_X_MAX 256.0f
#define UPH_MIDI_EDITOR_ZOOM_Y_MIN 8.0f
#define UPH_MIDI_EDITOR_ZOOM_Y_MAX 60.0f
#define UPH_MIDI_EDITOR_PAN_SPEED 1.0f
#define UPH_MIDI_EDITOR_SCROLL_Y_SPEED 40.0f
#define UPH_MIDI_EDITOR_ZOOM_SPEED 0.1f
#define UPH_MIDI_EDITOR_PIANO_WIDTH 80.0f
#define UPH_MIDI_EDITOR_TOP_RULER_HEIGHT 32.0f

typedef uint8_t Uph_NoteInteractionMode;
enum
{
    UPH_NOTE_INTERACTION_NONE,
    UPH_NOTE_INTERACTION_MOVE,
    UPH_NOTE_INTERACTION_RESIZE_LEFT,
    UPH_NOTE_INTERACTION_RESIZE_RIGHT
};

typedef struct
{
    double initial_drag_beat_offset;
    double initial_start_beat;
    double initial_length_beats;
    uint8_t initial_key_number;
    int32_t initial_drag_key_offset;
    uint32_t note_index;
    bool active;
    Uph_NoteInteractionMode mode;
}
Uph_DraggingNoteState;

typedef struct
{
    uint32_t note_index;
    bool active;
}
Uph_HoveredNoteState;

typedef struct
{
    Naui_Vec2 scroll;
    Naui_Vec2 zoom;
    Uph_ActionMode current_action_mode;
    Uph_DraggingNoteState drag;
    Uph_HoveredNoteState hovered_note;
    Leaf_BoundingBox lanes_bounding_box;
    double last_note_length;
    double last_note_velocity;
    bool lanes_hovered;
    bool panel_hovered;
}
Uph_MidiEditorData;
static Uph_MidiEditorData uph_midi_editor_data;

NAUI_PANEL(uph_midi_editor)

static inline bool uph_is_black_key(uint8_t midi_note)
{
    int semitone = midi_note % 12;
    return semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10;
}

static inline int uph_white_key_index_before(uint8_t midi_note)
{
    static const int white_count_in_octave[12] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };
    int octave = midi_note / 12;
    int semitone = midi_note % 12;
    return octave * 7 + white_count_in_octave[semitone];
}

static inline int uph_white_key_total_count(void)
{
    return uph_white_key_index_before(127) + (uph_is_black_key(127) ? 0 : 1);
}

static inline void uph_midi_key_name(uint8_t key_number, char *out, size_t out_size)
{
    static const char *names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int semitone = key_number % 12;
    int octave = key_number / 12;
    snprintf(out, out_size, "%s%d", names[semitone], octave);
}

static void uph_midi_editor_side_piano_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    const float note_height = uph_midi_editor_data.zoom.y;
    const float black_key_width_ratio = 0.6f;
    const float black_key_height_ratio = 0.6f;
    const float black_key_width = box.width * black_key_width_ratio;
    const float black_key_height = note_height * black_key_height_ratio;
    const int white_total = uph_white_key_total_count();
    const float half_lane_height = uph_midi_editor_data.zoom.x * 7.0f / 24.0f;
    const float scroll_y = uph_midi_editor_data.scroll.y + half_lane_height;

    for (uint8_t i = 0; i < 128; i++)
    {
        if (uph_is_black_key(i))
            continue;

        int white_index = uph_white_key_index_before(i);
        int row_from_top = (white_total - 1) - white_index;

        const float y_position = box.y + row_from_top * note_height - scroll_y;
        if (y_position > box.y + box.height || y_position < box.y - note_height)
            continue;

        naui_fill_rect(
            (Naui_Vec2) { box.x, y_position },
            (Naui_Vec2) { box.width, note_height },
            LEAF_COLOR_WHITE,
            0.0f,
            LEAF_CORNER_NONE
            //NAUI_DPI(3.0f),
            //LEAF_CORNER_TR | LEAF_CORNER_BR
        );
        naui_draw_gradient_rect(
            (Naui_Vec2) { box.x, y_position },
            (Naui_Vec2) { box.width, note_height },
            (Naui_Gradient) { .color1 = leaf_rgb(120, 120, 120), .color2 = LEAF_COLOR_WHITE, .percent1 = 0.8f, .percent2 = 1.0f },
            1.0f,
            0.0f,
            LEAF_CORNER_NONE,
            //NAUI_DPI(3.0f),
            //LEAF_CORNER_TR | LEAF_CORNER_BR,
            NAUI_SIDE_TOP | NAUI_SIDE_BOTTOM | NAUI_SIDE_RIGHT
        );
    }

    for (uint8_t i = 0; i < 128; i++)
    {
        if (!uph_is_black_key(i))
            continue;

        int white_index = uph_white_key_index_before(i);
        int row_from_top = (white_total - 1) - white_index;
        float boundary_y = box.y + row_from_top * note_height - scroll_y;
        float key_y = boundary_y - black_key_height * 0.5f;

        naui_fill_gradient_rect(
            (Naui_Vec2) { box.x, key_y },
            (Naui_Vec2) { black_key_width, black_key_height },
            (Naui_Gradient){ .color1 = leaf_rgb(20, 20, 20), .color2 = leaf_rgb(60, 60, 60), .percent1 = 0.75f, .percent2 = 1.0f },
            //NAUI_DPI(2.0f),
            0.0f,
            LEAF_CORNER_NONE
            //LEAF_CORNER_TR | LEAF_CORNER_BR
        );
    }
}

static void uph_midi_editor_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, NAUI_TR("midi_editor.title"));
    uph_midi_editor_data.zoom = (Naui_Vec2){30.0f, 30.0f};
    uph_midi_editor_data.scroll = (Naui_Vec2){0.0f, 0.0f};
    uph_midi_editor_data.current_action_mode = UPH_ACTION_DRAW;
    uph_midi_editor_data.last_note_length = UPH_MIDI_EDITOR_DEFAULT_NOTE_LENGTH;
    uph_midi_editor_data.last_note_velocity = UPH_MIDI_EDITOR_DEFAULT_NOTE_VELOCITY;

    const float lane_height = uph_midi_editor_data.zoom.y * 7.0f / 12.0f;
    const uint32_t row_from_top = 57u;

    uph_midi_editor_data.scroll = (Naui_Vec2){
        0.0f,
        row_from_top * lane_height
    };
}

static void uph_midi_editor_on_detach(void)
{
    
}

static void uph_midi_editor_on_open(void)
{

}

static void uph_midi_editor_on_close(void)
{

}

static inline Naui_Vec4 uph_midi_editor_note_screen_box(Leaf_BoundingBox box, const Uph_MidiNote *note)
{
    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float zoom_y = uph_midi_editor_data.zoom.y;
    const float lane_height = zoom_y * 7.0f / 12.0f;
    const float scroll_x = uph_midi_editor_data.scroll.x;
    const float scroll_y = uph_midi_editor_data.scroll.y;

    const uint32_t row_from_top = 127u - note->key_number;

    const float y_position = box.y + row_from_top * lane_height - scroll_y;
    const float x_position = box.x + (float)(note->start_beat * zoom_x) - scroll_x;
    const float width = (float)(note->length_beats * zoom_x);

    return (Naui_Vec4) { x_position, y_position, width, lane_height };
}

static Uph_NoteInteractionMode uph_midi_editor_classify_hover(Naui_Vec4 hover_box, float mouse_x)
{
    if (mouse_x <= hover_box.x + UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH)
        return UPH_NOTE_INTERACTION_RESIZE_LEFT;
    if (mouse_x >= hover_box.x + hover_box.z - UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH)
        return UPH_NOTE_INTERACTION_RESIZE_RIGHT;
    return UPH_NOTE_INTERACTION_MOVE;
}

static inline int32_t uph_midi_editor_mouse_row_from_top(Leaf_BoundingBox box, float lane_height)
{
    return (int32_t)floor(((double)naui_mouse_y() - box.y + uph_midi_editor_data.scroll.y) / lane_height);
}

static void uph_midi_editor_update_note_drag(Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
    Uph_DraggingNoteState *drag = &uph_midi_editor_data.drag;
    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float zoom_y = uph_midi_editor_data.zoom.y;
    const float lane_height = zoom_y * 7.0f / 12.0f;
    const float scroll_x = uph_midi_editor_data.scroll.x;

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(pattern->notes); i++)
    {
        Uph_MidiNote *note = &pattern->notes[i];

        bool is_dragging_this_note = drag->active && drag->note_index == i;

        if (is_dragging_this_note)
        {
            double mouse_beat = ((double)naui_mouse_x() - box.x + scroll_x) / zoom_x;

            if (drag->mode == UPH_NOTE_INTERACTION_MOVE)
            {
                note->start_beat = fmax(0.0, floor(mouse_beat + drag->initial_drag_beat_offset));

                const int32_t mouse_row_now = uph_midi_editor_mouse_row_from_top(box, lane_height);
                const int32_t new_row = mouse_row_now + drag->initial_drag_key_offset;
                const int32_t new_key = 127 - new_row;
                note->key_number = (uint8_t)NAUI_CLAMP(new_key, 0, 127);

                naui_set_cursor(NAUI_CURSOR_HAND);
            }
            else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_LEFT)
            {
                double new_start = fmax(0.0, floor(mouse_beat));
                double end_beat = drag->initial_start_beat + drag->initial_length_beats;

                new_start = fmin(new_start, end_beat - 1.0);

                note->start_beat = new_start;
                note->length_beats = end_beat - new_start;

                uph_midi_editor_data.last_note_length = note->length_beats;
                uph_midi_editor_data.last_note_velocity = note->velocity;

                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            }
            else if (drag->mode == UPH_NOTE_INTERACTION_RESIZE_RIGHT)
            {
                double new_length = ceil(mouse_beat) - note->start_beat;
                note->length_beats = fmax(1.0, new_length);

                uph_midi_editor_data.last_note_length = note->length_beats;
                uph_midi_editor_data.last_note_velocity = note->velocity;

                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            }

            if (naui_mouse_released(NAUI_MOUSE_LEFT))
            {
                drag->active = false;
                drag->mode = UPH_NOTE_INTERACTION_NONE;
            }

            return;
        }

        if (!uph_midi_editor_data.lanes_hovered)
            continue;

        if (!drag->active)
        {
            Naui_Vec4 hover_box = uph_midi_editor_note_screen_box(box, note);

            if (hover_box.y + hover_box.w < box.y || hover_box.y > box.y + box.height)
                continue;
            if (hover_box.x + hover_box.z < box.x || hover_box.x > box.x + box.width)
                continue;

            if (upb_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
            {
                Uph_NoteInteractionMode hover_mode = uph_midi_editor_classify_hover(hover_box, (float)naui_mouse_x());

                if (uph_midi_editor_data.current_action_mode == UPH_ACTION_SELECT ||
                    uph_midi_editor_data.current_action_mode == UPH_ACTION_DRAW)
                {
                    if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
                    {
                        drag->active = true;
                        drag->note_index = i;
                        drag->mode = hover_mode;
                        drag->initial_start_beat = note->start_beat;
                        drag->initial_length_beats = note->length_beats;
                        drag->initial_key_number = note->key_number;

                        double mouse_beat = ((double)naui_mouse_x() - box.x + scroll_x) / zoom_x;
                        drag->initial_drag_beat_offset = note->start_beat - mouse_beat;

                        const int32_t mouse_row_at_grab = uph_midi_editor_mouse_row_from_top(box, lane_height);
                        const int32_t note_row = 127 - (int32_t)note->key_number;
                        drag->initial_drag_key_offset = note_row - mouse_row_at_grab;
                        
                        uph_midi_editor_data.last_note_length = note->length_beats;
                        uph_midi_editor_data.last_note_velocity = note->velocity;
                    }

                    naui_set_cursor(
                        hover_mode == UPH_NOTE_INTERACTION_MOVE
                            ? NAUI_CURSOR_HAND
                            : NAUI_CURSOR_RESIZE_EW
                    );
                }

                uph_midi_editor_data.hovered_note.note_index = i;
                uph_midi_editor_data.hovered_note.active = true;
            }
        }
    }
}

static void uph_midi_editor_update_draw_input(Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
    if (!uph_midi_editor_data.lanes_hovered)
        return;

    if (uph_midi_editor_data.hovered_note.active)
        return;

    if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return;

    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float zoom_y = uph_midi_editor_data.zoom.y;
    const float lane_height = zoom_y * 7.0f / 12.0f;
    const float scroll_x = uph_midi_editor_data.scroll.x;

    const double beat = ((double)naui_mouse_x() - box.x + scroll_x) / zoom_x;
    const int32_t row_from_top = uph_midi_editor_mouse_row_from_top(box, lane_height);
    const uint8_t key_number = (uint8_t)NAUI_CLAMP(127 - row_from_top, 0, 127);

    Uph_MidiNote note = {
        .start_beat = fmax(0.0, floor(beat)),
        .length_beats = uph_midi_editor_data.last_note_length,
        .key_number = key_number,
        .velocity = uph_midi_editor_data.last_note_velocity
    };

    uph_midi_editor_data.drag.active = true;
    uph_midi_editor_data.drag.note_index = (uint32_t)naui_list_len(pattern->notes);
    uph_midi_editor_data.drag.mode = UPH_NOTE_INTERACTION_MOVE;
    uph_midi_editor_data.drag.initial_drag_beat_offset = 0.0;
    uph_midi_editor_data.drag.initial_drag_key_offset = 0;
    uph_midi_editor_data.drag.initial_start_beat = note.start_beat;
    uph_midi_editor_data.drag.initial_length_beats = note.length_beats;
    uph_midi_editor_data.drag.initial_key_number = note.key_number;

    naui_list_push(pattern->notes, note);
}

static void uph_midi_editor_update_cut_input(Leaf_BoundingBox box, Uph_MidiPattern *pattern)
{
    if (!uph_midi_editor_data.lanes_hovered)
        return;

    if (!uph_midi_editor_data.hovered_note.active)
        return;

    if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return;

    const uint32_t note_index = uph_midi_editor_data.hovered_note.note_index;
    Uph_MidiNote *note = &pattern->notes[note_index];

    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float scroll_x = uph_midi_editor_data.scroll.x;
    const double mouse_beat = ((double)naui_mouse_x() - box.x + scroll_x) / zoom_x;
    const double cut_beat = floor(mouse_beat);

    const double left_length = cut_beat - note->start_beat;
    const double right_length = note->length_beats - left_length;

    if (left_length >= 1.0 && right_length >= 1.0)
    {
        Uph_MidiNote right_half = *note;
        right_half.start_beat = cut_beat;
        right_half.length_beats = right_length;

        note->length_beats = left_length;

        naui_list_push(pattern->notes, right_half);
    }
}

static void uph_midi_editor_update_delete_input(Uph_MidiPattern *pattern)
{
    if (!uph_midi_editor_data.lanes_hovered)
        return;

    if (!uph_midi_editor_data.hovered_note.active)
        return;

    if (!naui_mouse_pressed(NAUI_MOUSE_RIGHT))
        return;

    naui_list_uremove(pattern->notes, uph_midi_editor_data.hovered_note.note_index);
}

static void uph_midi_editor_render_beat_grid(Leaf_BoundingBox box)
{
    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float scroll_x = uph_midi_editor_data.scroll.x;

    const Leaf_Color beat_color = naui_theme_color("uph_track_grid_beat_color");
    const Leaf_Color bar_color = naui_theme_color("uph_track_grid_bar_color");

    const int32_t first_line = (int32_t)(scroll_x / zoom_x);
    const uint32_t line_count = (uint32_t)(box.width / zoom_x) + 2;

    for (uint32_t i = 0; i < line_count; i++)
    {
        const int32_t line_index = first_line + (int32_t)i;
        if (line_index < 0)
            continue;

        const float x = box.x + (float)line_index * zoom_x - scroll_x;

        if (x < box.x || x > box.x + box.width)
            continue;

        const bool is_downbeat = (line_index % 4) == 0;
        const Leaf_Color line_color = is_downbeat ? bar_color : beat_color;

        naui_draw_line(
            (Naui_Vec2) { x, box.y },
            (Naui_Vec2) { x, box.y + box.height },
            line_color,
            1.0f
        );
    }
}

static void uph_midi_editor_render_cut_line(Leaf_BoundingBox box)
{
    if (uph_midi_editor_data.current_action_mode != UPH_ACTION_CUT)
        return;

    if (!uph_midi_editor_data.lanes_hovered)
        return;

    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float scroll_x = uph_midi_editor_data.scroll.x;

    const double mouse_beat = ((double)naui_mouse_x() - box.x + scroll_x) / zoom_x;
    const double cut_beat = floor(mouse_beat);

    const float x = box.x + (float)(cut_beat * zoom_x) - scroll_x;

    naui_draw_line(
        (Naui_Vec2) { x, box.y },
        (Naui_Vec2) { x, box.y + box.height },
        naui_theme_color("uph_playhead_color"),
        NAUI_DPI(1.5f)
    );
}

static void uph_midi_editor_lanes_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float zoom_y = uph_midi_editor_data.zoom.y;
    const float scroll_y = uph_midi_editor_data.scroll.y;

    const float lane_height = zoom_y * 7.0f / 12.0f;

    for (uint8_t i = 0; i < 128; i++)
    {
        const uint32_t row_from_top = 127u - i;
        const float y_position = box.y + row_from_top * lane_height - scroll_y;
        if (y_position > box.y + box.height || y_position < box.y - lane_height)
            continue;

        naui_fill_rect(
            (Naui_Vec2) { box.x, y_position },
            (Naui_Vec2) { box.width, lane_height },
            uph_is_black_key(i) ? leaf_rgba(0, 0, 0, 50) : LEAF_COLOR_TRANSPARENT,
            0.0f,
            LEAF_CORNER_NONE
        );
    }

    uph_midi_editor_data.hovered_note.active = false;

    naui_push_clip_rect(box.x, box.y, box.width, box.height);
    uph_midi_editor_render_cut_line(box);
    uph_midi_editor_render_beat_grid(box);
    naui_pop_clip_rect();

    if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
        return;

    naui_push_clip_rect(box.x, box.y, box.width, box.height);

    Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[uph_state.shared.selected_resource.index];

    uph_midi_editor_update_note_drag(box, pattern);

    if (uph_midi_editor_data.current_action_mode == UPH_ACTION_SELECT)
    {
        // Selection not implemented yet
    }
    else if (uph_midi_editor_data.current_action_mode == UPH_ACTION_DRAW)
    {
        uph_midi_editor_update_draw_input(box, pattern);
    }
    else if (uph_midi_editor_data.current_action_mode == UPH_ACTION_CUT)
    {
        uph_midi_editor_update_cut_input(box, pattern);
    }

    uph_midi_editor_update_delete_input(pattern);

    const float note_label_font_size = fminf(NAUI_DPI(11.0f), lane_height * 0.8f);
    const float note_label_padding = NAUI_DPI(3.0f);
    const Leaf_Color note_label_color = leaf_rgba(0, 0, 0, 180);

    for (uint32_t i = 0; i < naui_list_len(pattern->notes); i++)
    {
        Uph_MidiNote *note = &pattern->notes[i];

        const uint32_t row_from_top = 127u - note->key_number;
        const float y_position = box.y + row_from_top * lane_height - scroll_y;
        if (y_position > box.y + box.height || y_position < box.y - lane_height)
            continue;

        const float note_x = box.x + note->start_beat * zoom_x - uph_midi_editor_data.scroll.x;
        const float note_width = note->length_beats * zoom_x;

        naui_fill_rect(
            (Naui_Vec2) { note_x, y_position },
            (Naui_Vec2) { note_width, lane_height },
            LEAF_COLOR_WHITE,
            0.0f,
            LEAF_CORNER_NONE
        );

        if (note_label_font_size >= NAUI_DPI(6.0f))
        {
            char label[8];
            uph_midi_key_name(note->key_number, label, sizeof(label));

            naui_push_clip_rect(note_x, y_position, note_width, lane_height);
            naui_draw_text(
                (Naui_Vec2) { note_x + note_label_padding, y_position + (lane_height - note_label_font_size) * 0.5f },
                label,
                note_label_font_size,
                0,
                note_label_color
            );
            naui_pop_clip_rect();
        }
    }

    naui_pop_clip_rect();
}

static void uph_midi_editor_render_top_ruler(Leaf_BoundingBox bbox)
{
    const Leaf_Color beat_color = naui_theme_color("uph_track_grid_beat_color");
    const Leaf_Color bar_color = naui_theme_color("uph_track_grid_bar_color");
    const Leaf_Color number_color = naui_theme_color("uph_track_grid_text_color");

    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float scroll_x = uph_midi_editor_data.scroll.x;

    const int32_t first_line = (int32_t)(scroll_x / zoom_x);
    const uint32_t line_count = (uint32_t)(bbox.width / zoom_x) + 2;

    naui_push_clip_rect(bbox.x - 0.5f, bbox.y, bbox.width, bbox.height);
    for (uint32_t i = 0; i < line_count; i++)
    {
        const int32_t line_index = first_line + (int32_t)i;
        if (line_index < 0)
            continue;

        const float x = bbox.x + (float)line_index * zoom_x - scroll_x;

        if (x < bbox.x - zoom_x || x > bbox.x + bbox.width)
            continue;

        const bool is_downbeat = (line_index % 4) == 0;
        const Leaf_Color line_color = is_downbeat ? bar_color : beat_color;

        naui_draw_line(
            (Naui_Vec2) { x, bbox.y + bbox.height * (is_downbeat ? 0.4f : 0.6f)},
            (Naui_Vec2) { x, bbox.y + bbox.height },
            line_color,
            1.0f
        );

        if (is_downbeat)
        {
            char label[16];
            snprintf(label, sizeof(label), "%d", line_index / 4);
            naui_draw_text((Naui_Vec2) { x + 4.0f, bbox.y + bbox.height * 0.4f }, label, NAUI_DPI(13.0f), 0, number_color);
        }
    }
    naui_pop_clip_rect();
}

static void uph_midi_editor_render_toolbox(void)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32.0f))},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
        .color = naui_theme_color("uph_ui_toolbox_bg_color"),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .child_gap = NAUI_DPI(16.0f),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        const float button_size = NAUI_DPI(14.0f);
        const Naui_Color bg_color = naui_theme_color("uph_ui_frame_secondary_bg_color");
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
        })
        {
            const Naui_Color icon_color = naui_theme_color("uph_tool_icon_color");
            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_select"),
                leaf_id("uph_midi_editor_select"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_TL | NAUI_CORNER_BL,
                uph_midi_editor_data.current_action_mode == UPH_ACTION_SELECT
            )) uph_midi_editor_data.current_action_mode = UPH_ACTION_SELECT;

            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_draw"),
                leaf_id("uph_midi_editor_draw"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_NONE,
                uph_midi_editor_data.current_action_mode == UPH_ACTION_DRAW
            )) uph_midi_editor_data.current_action_mode = UPH_ACTION_DRAW;

            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_cut"),
                leaf_id("uph_midi_editor_cut"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_TR | NAUI_CORNER_BR,
                uph_midi_editor_data.current_action_mode == UPH_ACTION_CUT
            )) uph_midi_editor_data.current_action_mode = UPH_ACTION_CUT;
        }
    }
}

static float uph_midi_editor_max_scroll_y(float viewport_height)
{
    const float zoom_y = uph_midi_editor_data.zoom.y;
    const float lane_height = zoom_y * 7.0f / 12.0f;
    const float content_height = 128.0f * lane_height;
    return fmaxf(0.0f, content_height - viewport_height);
}

static void uph_midi_editor_update_input(Leaf_BoundingBox box)
{
    const float wheel_y = (float)naui_mouse_scroll_delta();
    const bool ctrl_held = naui_key_down(NAUI_KEY_LCONTROL);
    const float max_scroll_y = uph_midi_editor_max_scroll_y(box.height);

    if (uph_midi_editor_data.panel_hovered)
    {
        if (ctrl_held && wheel_y != 0.0f)
        {
            const float old_zoom_x = uph_midi_editor_data.zoom.x;

            const double mouse_beat_before =
                ((double)naui_mouse_x() - box.x + uph_midi_editor_data.scroll.x) / old_zoom_x;

            float new_zoom_x = old_zoom_x * (1.0f + wheel_y * UPH_MIDI_EDITOR_ZOOM_SPEED);
            new_zoom_x = NAUI_CLAMP(new_zoom_x, UPH_MIDI_EDITOR_ZOOM_X_MIN, UPH_MIDI_EDITOR_ZOOM_X_MAX);

            uph_midi_editor_data.zoom.x = new_zoom_x;

            uph_midi_editor_data.scroll.x =
                (float)(mouse_beat_before * new_zoom_x) - ((float)naui_mouse_x() - box.x);
            uph_midi_editor_data.scroll.x = fmaxf(0.0f, uph_midi_editor_data.scroll.x);
        }
        else if (wheel_y != 0.0f)
        {
            uph_midi_editor_data.scroll.y -= wheel_y * UPH_MIDI_EDITOR_SCROLL_Y_SPEED;
            uph_midi_editor_data.scroll.y = NAUI_CLAMP(uph_midi_editor_data.scroll.y, 0.0f, max_scroll_y);
        }
    }

    static Naui_Vec2 pan_last_mouse;
    static bool panning = false;

    if (uph_midi_editor_data.panel_hovered && naui_mouse_pressed(NAUI_MOUSE_MIDDLE))
    {
        panning = true;
        pan_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
    }

    if (panning)
    {
        Naui_Vec2 current = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
        Naui_Vec2 delta = (Naui_Vec2) { current.x - pan_last_mouse.x, current.y - pan_last_mouse.y };

        uph_midi_editor_data.scroll.x -= delta.x * UPH_MIDI_EDITOR_PAN_SPEED;
        uph_midi_editor_data.scroll.x = fmaxf(0.0f, uph_midi_editor_data.scroll.x);

        const float current_max_scroll_y = uph_midi_editor_max_scroll_y(box.height);
        uph_midi_editor_data.scroll.y -= delta.y * UPH_MIDI_EDITOR_PAN_SPEED;
        uph_midi_editor_data.scroll.y = NAUI_CLAMP(uph_midi_editor_data.scroll.y, 0.0f, current_max_scroll_y);

        pan_last_mouse = current;

        if (naui_mouse_released(NAUI_MOUSE_MIDDLE))
            panning = false;
    }
}

static void uph_midi_editor_on_update(void)
{
    const Leaf_ID lanes_id = leaf_id("uph_midi_editor_lanes");

    if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
    {
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        })
        {
            leaf_text("No midi pattern is selected.", {
                .color = naui_theme_color("uph_ui_text_color"),
                .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"))
            });
        }
        return;
    }

    uph_midi_editor_data.panel_hovered = naui_panel_hovered(naui_current_panel());
    uph_midi_editor_data.lanes_hovered = leaf_hovered(lanes_id) && uph_midi_editor_data.panel_hovered;
    uph_midi_editor_data.lanes_bounding_box = leaf_get_bounding_box(lanes_id);

    uph_midi_editor_update_input(uph_midi_editor_data.lanes_bounding_box);

    uph_midi_editor_render_toolbox();

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIDI_EDITOR_TOP_RULER_HEIGHT))},
        .child_gap = 1.0f
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIDI_EDITOR_PIANO_WIDTH)), LEAF_SIZE_FULL}
        });
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .custom_draw = (Leaf_CustomDrawFn)uph_midi_editor_render_top_ruler
        });
    }

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .child_gap = 1.0f,
        .clip_children = true
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIDI_EDITOR_PIANO_WIDTH)), LEAF_SIZE_FULL},
            .custom_draw = uph_midi_editor_side_piano_custom_draw
        });
        leaf({
            .id = lanes_id,
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .custom_draw = uph_midi_editor_lanes_custom_draw
        });
    }
}