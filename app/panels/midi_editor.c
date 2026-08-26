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
    const float note_height = NAUI_DPI(20.0f);
    const float black_key_width_ratio = 0.6f;
    const float black_key_height_ratio = 0.6f;
    const float black_key_width = box.width * black_key_width_ratio;
    const float black_key_height = note_height * black_key_height_ratio;

    for (uint8_t i = 0; i < 128; i++)
    {
        if (uph_is_black_key(i))
            continue;

        int white_index = uph_white_key_index_before(i);

        naui_fill_rect(
            naui_vec2(box.x, box.y + white_index * note_height),
            naui_vec2(box.width, note_height),
            LEAF_COLOR_WHITE,
            NAUI_DPI(3.0f),
            LEAF_CORNER_TR | LEAF_CORNER_BR
        );
        naui_draw_gradient_rect(
            naui_vec2(box.x, box.y + white_index * note_height),
            naui_vec2(box.width, note_height),
            (Naui_Gradient) { .color1 = leaf_rgb(120, 120, 120), .color2 = LEAF_COLOR_WHITE, .percent1 = 0.8f, .percent2 = 1.0f },
            1.0f,
            NAUI_DPI(3.0f),
            LEAF_CORNER_TR | LEAF_CORNER_BR,
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
            naui_vec2(box.x, key_y),
            naui_vec2(black_key_width, black_key_height),
            (Naui_Gradient){ .color1 = leaf_rgb(20, 20, 20), .color2 = leaf_rgb(60, 60, 60), .percent1 = 0.75f, .percent2 = 1.0f },
            NAUI_DPI(2.0f),
            LEAF_CORNER_TR | LEAF_CORNER_BR
        );
    }
}

static void uph_midi_editor_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "Midi Editor");
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

}

static void uph_midi_editor_on_update(void)
{
    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
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