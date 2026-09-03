NAUI_PANEL(uph_pattern_list)

static void uph_pattern_list_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, NAUI_TR("patterns.title"));
}

static void uph_pattern_list_on_detach(void)
{
    
}

static void uph_pattern_list_on_open(void)
{
    
}

static void uph_pattern_list_on_close(void)
{
    
}

static void uph_pattern_list_custom_draw(Leaf_BoundingBox box, void **user_data)
{
    Uph_MidiPattern* pattern = (Uph_MidiPattern*)*user_data;
    const uint32_t note_count = (uint32_t)naui_list_len(pattern->notes);

    if (note_count == 0)
        return;

    uint8_t lowest_key = UINT8_MAX;
    uint8_t highest_key = 0;
    double furthest_beat = 0.0;

    for (uint32_t i = 0; i < note_count; i++)
    {
        const Uph_MidiNote *note = &pattern->notes[i];
        if (note->key_number < lowest_key)
            lowest_key = note->key_number;
        if (note->key_number > highest_key)
            highest_key = note->key_number;

        const double note_end_beat = note->start_beat + note->length_beats;
        if (note_end_beat > furthest_beat)
            furthest_beat = note_end_beat;
    }

    const uint32_t key_range = (uint32_t)(highest_key - lowest_key) + 1;

    const float slot_height = box.height / (float)key_range;
    const float note_height = fmaxf(slot_height, 1.0f);

    const float x_scale = (furthest_beat > 0.0)
        ? (box.width / (float)furthest_beat)
        : 1.0f;

    for (uint32_t i = 0; i < note_count; i++)
    {
        const Uph_MidiNote *note = &pattern->notes[i];

        const double note_start_beat = note->start_beat;

        if (note_start_beat + note->length_beats < 0.0)
            continue;

        const float x = box.x + (float)(note_start_beat * x_scale);
        const float width = (float)(note->length_beats * x_scale);

        const uint32_t key_offset_from_top = (uint32_t)(highest_key - note->key_number);
        const float y = box.y + (float)key_offset_from_top * slot_height;

        naui_fill_rect(
            (Naui_Vec2) { x, y },
            (Naui_Vec2) { fmaxf(width, 1.0f), note_height },
            LEAF_COLOR_WHITE,
            0,
            NAUI_CORNER_NONE
        );
    }
}

static double uph_pattern_list_calculate_pattern_length(const Uph_MidiPattern *pattern)
{
    double length = 4.0; // minimum size
    for (uint32_t i = 0; i < naui_list_len(pattern->notes); i++)
    {
        const double note_end = pattern->notes[i].start_beat + pattern->notes[i].length_beats;
        if (note_end > length)
            length = note_end;
    }
    return length;
}

static void uph_pattern_list_on_update(void)
{
    leaf({
        .size = {
            .width = LEAF_SIZE_FULL,
            .height = LEAF_SIZE_FULL
        },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(6)),
        .child_gap = NAUI_DPI(6),
        .child_cross_gap = NAUI_DPI(6),
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .wrap_children = true
    }) {
        uint32_t pattern_count = (uint32_t)naui_list_len(uph_state.project.midi_patterns);
        Uph_UIMenuID context_menu = uph_ui_context_menu();

        for (uint32_t i = 0; i < pattern_count; i++) {
            Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[i];

            const Leaf_ID id = leaf_id_indexed("uph_pattern_list_pattern", i);
            if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_ui_widget_hovered(id))
            {
                uph_state.shared.selected_resource.index = i;
                uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
                uph_ui_open_context_menu(context_menu);
            }

            if (uph_ui_list_box(
                pattern->name.data,
                (Leaf_CustomDrawFn)uph_pattern_list_custom_draw,
                LEAF_DATA_SLICE(pattern),
                id,
                uph_state.shared.selected_resource.index == i
            ))
            {
                uph_state.shared.selected_resource.index = i;
                uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;

                Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[i];
                uph_state.shared.song_timeline_current_block_start_offset = 0;
                uph_state.shared.song_timeline_current_block_length = uph_pattern_list_calculate_pattern_length(pattern);
            }
        }

        if (uph_ui_menu_item(context_menu, "Remove", leaf_id("uph_pattern_remove")))
        {
            uph_resources_remove_pattern(uph_state.shared.selected_resource.index);
            if (uph_state.shared.selected_resource.index > 0 && uph_state.shared.selected_resource.index == naui_list_len(uph_state.project.midi_patterns))
                uph_state.shared.selected_resource.index--;
            else if (naui_list_len(uph_state.project.midi_patterns) == 0)
                uph_state.shared.selected_resource.type = UPH_RESOURCE_NONE;
        }
        if (uph_ui_menu_item(context_menu, "Duplicate", leaf_id("uph_pattern_duplicate")))
            uph_resources_copy_pattern(uph_state.shared.selected_resource.index);

        const Leaf_ID plus_id = leaf_id("uph_pattern_list_plus");
        if (uph_ui_list_plus_box(plus_id))
        {
            uph_state.shared.selected_resource.index = naui_list_len(uph_state.project.midi_patterns);
            uph_state.shared.selected_resource.type = UPH_RESOURCE_PATTERN;
            uph_state.shared.song_timeline_current_block_start_offset = 0;
            uph_state.shared.song_timeline_current_block_length = 4.0;
            uph_resources_add_pattern();
        }
    }
}