typedef struct
{
    Naui_Vec2 zoom;
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

static void uph_midi_editor_side_piano_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    const float note_height = uph_midi_editor_data.zoom.y;
    const float black_key_width_ratio = 0.6f;
    const float black_key_height_ratio = 0.6f;
    const float black_key_width = box.width * black_key_width_ratio;
    const float black_key_height = note_height * black_key_height_ratio;

    for (uint8_t i = 0; i < 128; i++)
    {
        if (uph_is_black_key(i))
            continue;

        int white_index = uph_white_key_index_before(i);

        const float y_position = box.y + white_index * note_height;
        if (y_position > box.y + box.height || y_position < box.y)
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
        float boundary_y = box.y + white_index * note_height;
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
    naui_panel_set_title(this, "Midi Editor");
    uph_midi_editor_data.zoom = (Naui_Vec2){30.0f, 30.0f};
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

static void uph_midi_editor_lanes_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    const float zoom_x = uph_midi_editor_data.zoom.x;
    const float zoom_y = uph_midi_editor_data.zoom.y;

    const float lane_height = zoom_y * 7.0f / 12.0f;
    for (uint8_t i = 0; i < 128; i++)
    {
        const float y_position = box.y + i * lane_height - zoom_y;
        if (y_position > box.y + box.height || y_position < box.y)
            continue;

        naui_fill_rect(
            (Naui_Vec2) { box.x, y_position },
            (Naui_Vec2) { box.width, lane_height },
            uph_is_black_key(i) ? leaf_rgba(0, 0, 0, 50) : LEAF_COLOR_TRANSPARENT,
            0.0f,
            LEAF_CORNER_NONE
        );
    }

    if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
        return;

    Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[uph_state.shared.selected_resource.index];
    for (uint32_t i = 0; i < naui_list_len(pattern->notes); i++)
    {
        Uph_MidiNote *note = &pattern->notes[i];

        const float y_position = box.y + note->key_number * lane_height - zoom_y;
        if (y_position > box.y + box.height || y_position < box.y)
            continue;
        naui_fill_rect(
            (Naui_Vec2) { box.x + note->start_beat * zoom_x, y_position },
            (Naui_Vec2) { note->length_beats * zoom_x, lane_height },
            LEAF_COLOR_WHITE,
            0.0f,
            LEAF_CORNER_NONE
        );
    }

    /*if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return;

    Uph_MidiNote note = {
        .key_number = (naui_mouse_y() - box.y) / lane_height + 2,
        .start_beat = floor((naui_mouse_x() - box.x) / zoom_x),
        .length_beats = 3.0f
    };
    naui_list_push(pattern->notes, note);*/
}

static void uph_midi_editor_on_update(void)
{
    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_gap = 1.0f
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(80.0f)), LEAF_SIZE_FULL},
            .custom_draw = uph_midi_editor_side_piano_custom_draw
        });
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .custom_draw = uph_midi_editor_lanes_custom_draw
        });
    }
}