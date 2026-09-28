NAUI_PANEL(uph_resource_list)

static void uph_resource_list_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, NAUI_TR("resource_list.title"));
}

static void uph_resource_list_on_detach(void)
{
    
}

static void uph_resource_list_on_open(void)
{
    
}

static void uph_resource_list_on_close(void)
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
    const float x_scale = (furthest_beat > 0.0) ? (box.width / (float)furthest_beat) : 1.0f;

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

static void uph_sample_list_custom_draw(Leaf_BoundingBox box, void **user_data)
{
    Uph_Sample* sample = (Uph_Sample*)*user_data;
    Uph_SampleData *sample_data = &uph_state.project.sample_data[sample->data_index];

    if (!sample_data || sample_data->frame_count == 0)
        return;

    const float bpm = uph_state.project.bpm;
    if (bpm <= 0.0f || box.width <= 0.0f)
        return;

    const double time_scale = (sample->time_scale > 0.0) ? sample->time_scale : 1.0;
    const uint32_t sample_rate = uph_state.settings.audio.sample_rate;

    const double total_seconds = (double)sample_data->frame_count / (double)sample_rate;
    const double total_beats = uph_seconds_to_beats(total_seconds, bpm);

    if (total_beats <= 0.0)
        return;

    const double zoom = (double)box.width / (total_beats * time_scale);
    const double start_offset = 0.0;

    uph_ui_waveform_zoomable(
        (Naui_Vec2) { box.x, box.y },
        (Naui_Vec2) { box.width, box.height },
        LEAF_COLOR_WHITE,
        zoom,
        start_offset,
        box,
        sample
    );
}

static void uph_automation_list_custom_draw(Leaf_BoundingBox box, void **user_data)
{
    
}

bool uph_resource_list_box(Naui_String *name, Leaf_CustomDrawFn content_draw, Leaf_DataSlice content_draw_data, Leaf_ID id, bool hovered, bool selected, bool renaming, const char *placeholder_name)
{
    leaf({
        .id = id,
        .custom_draw = content_draw,
        .custom_draw_data = content_draw_data,
        .size = {
            .width = LEAF_SIZE_FIXED(NAUI_DPI(150)),
            .height = LEAF_SIZE_DERIVED
        },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(2)),
        .aspect_ratio = 2.2f,
        .border = {
            .width = NAUI_DPI(selected ? 3.0f : 1.0f),
            .sides = LEAF_SIDE_ALL,
            .color = {leaf_rgb(145, 111, 205)}
        },
        .color = {leaf_rgb(108, 83, 154)},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), LEAF_CORNER_ALL),
        .clip_children = true
    }) {
        if (renaming)
        {
            if (uph_ui_textfield(name, id, UPH_UI_TEXTFIELD_ALWAYS_ACTIVE, placeholder_name))
            {
                if (name->length == 0)
                    *name = naui_string_from_cstr(placeholder_name);
                uph_state.shared.selected_resource.renaming = false;
            }
        }
        else
        {
            leaf_text(name->data, {
                .color = {LEAF_COLOR_WHITE},
                .font_size = {NAUI_DPI(13)}
            });
        }
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void uph_resource_list_render_resource(uint32_t index, Uph_ResourceType type, Uph_UIMenuID context_menu, Leaf_ID id)
{
    bool is_selected = uph_state.shared.selected_resource.index == index &&
        uph_state.shared.selected_resource.type == type;
    bool is_renaming = is_selected && uph_state.shared.selected_resource.renaming;

    bool hovered = !is_renaming && uph_ui_widget_hovered(id);

    if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
        {
            uph_state.shared.selected_resource.renaming = false;
            uph_state.shared.selected_resource.index = index;
            uph_state.shared.selected_resource.type = type;
            uph_ui_open_context_menu(context_menu);
        }
        else if (naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
        {
            uph_state.shared.selected_resource.index = index;
            uph_state.shared.selected_resource.type = type;
            uph_state.shared.selected_resource.renaming = true;
        }
    }

    if (type == UPH_RESOURCE_PATTERN)
    {
        Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[index];
        if (uph_resource_list_box(
            &pattern->name,
            (Leaf_CustomDrawFn)uph_pattern_list_custom_draw,
            LEAF_DATA_SLICE(pattern),
            id,
            hovered,
            is_selected,
            is_renaming,
            "Untitled Pattern"
        ))
        {
            uph_state.shared.selected_resource.index = index;
            uph_state.shared.selected_resource.type = type;

            uph_state.shared.song_timeline_current_block_start_offset = 0;
            uph_state.shared.song_timeline_current_block_length = uph_calculate_pattern_length(pattern);
        }
    }
    else if (type == UPH_RESOURCE_SAMPLE)
    {
        Uph_Sample *sample = &uph_state.project.samples[index];
        if (uph_resource_list_box(
            &sample->name,
            (Leaf_CustomDrawFn)uph_sample_list_custom_draw,
            LEAF_DATA_SLICE(sample),
            id,
            hovered,
            is_selected,
            is_renaming,
            "Untitled Sample"
        ))
        {
            uph_state.shared.selected_resource.index = index;
            uph_state.shared.selected_resource.type = type;

            Uph_SampleData *sample_data = &uph_state.project.sample_data[sample->data_index];
            double beats = ceil(uph_frames_to_beats(sample_data->frame_count, uph_state.settings.audio.sample_rate, uph_state.project.bpm));
            uph_state.shared.song_timeline_current_block_length = beats;
        }
    }
    else if (type == UPH_RESOURCE_AUTOMATION)
    {
        Uph_Automation *automation = &uph_state.project.automations[index];
        if (uph_resource_list_box(
            &automation->name,
            (Leaf_CustomDrawFn)uph_automation_list_custom_draw,
            LEAF_DATA_SLICE(automation),
            id,
            hovered,
            is_selected,
            is_renaming,
            "Untitled Automation"
        ))
        {
            uph_state.shared.selected_resource.index = index;
            uph_state.shared.selected_resource.type = type;

            uph_state.shared.song_timeline_current_block_start_offset = 0;
            uph_state.shared.song_timeline_current_block_length = uph_calculate_automation_length(automation);
        }
    }
}

void uph_resource_list_plus_box(Uph_ResourceType type, Leaf_ID id)
{
    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_FIXED(NAUI_DPI(150)),
            .height = LEAF_SIZE_DERIVED
        },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(2)),
        .aspect_ratio = 2.2f,
        .border = {
            .width = NAUI_DPI(1),
            .sides = LEAF_SIDE_ALL,
            .color = {naui_theme_color("uph_ui_frame_border")}
        },
        .color = {naui_theme_color("uph_ui_frame_bg_color")},
        .child_alignment = {
            LEAF_ALIGN_X_CENTER,
            LEAF_ALIGN_Y_CENTER
        },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), LEAF_CORNER_ALL)
    }) {
        leaf({
            .image = naui_asset_image("uph_icon_plus"),
            .size = {
                .width = LEAF_SIZE_PERCENT(0.1f),
                .height = LEAF_SIZE_DERIVED
            },
            .color = naui_theme_color("uph_ui_frame_border"),
            .aspect_ratio = 1.0f
        });
    }

	bool hovered = uph_ui_widget_hovered(id);
	if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {

            uph_state.shared.selected_resource.type = type;
            uph_state.shared.selected_resource.renaming = false;
            uph_state.shared.song_timeline_current_block_start_offset = 0;
            uph_state.shared.song_timeline_current_block_length = 4.0;

            if (type == UPH_RESOURCE_PATTERN)
            {
                uph_state.shared.selected_resource.index = naui_list_len(uph_state.project.midi_patterns);
                uph_resources_add_pattern();
            }
            else if (type == UPH_RESOURCE_SAMPLE)
            {
                //uph_state.shared.selected_resource.index = naui_list_len(uph_state.project.samples);
                //uph_resources_add_pattern();
            }
            else if (type == UPH_RESOURCE_AUTOMATION)
            {
                uph_state.shared.selected_resource.index = naui_list_len(uph_state.project.automations);
                uph_resources_add_automation();
            }
        }
    }
}

static void uph_resource_list_on_update(void)
{
    Uph_UIMenuID context_menu = uph_ui_context_menu();
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Color section_title_text_color = naui_theme_color("uph_resource_list_section_text_color");
    const Naui_Color section_title_bg_color = naui_theme_color("uph_resource_list_section_bg_color");
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    leaf({
        .size = {
            .width = LEAF_SIZE_FULL,
            .height = LEAF_SIZE_FULL
        },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .child_gap = NAUI_DPI(8)
    }) {
        leaf({.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT}, .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)), .color = section_title_bg_color})
            leaf_text("Patterns", {.font_size = font_size, .color = section_title_text_color});
    
        leaf({
            .size = {
                .width = LEAF_SIZE_FULL,
                .height = LEAF_SIZE_FIT
            },
            .child_gap = NAUI_DPI(6),
            .child_cross_gap = NAUI_DPI(6),
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .wrap_children = true
        })
        {
            for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.midi_patterns); i++)
                uph_resource_list_render_resource(i, UPH_RESOURCE_PATTERN, context_menu, leaf_id_indexed("uph_pattern_list_item", i));
            uph_resource_list_plus_box(UPH_RESOURCE_PATTERN, leaf_id("uph_pattern_list_plus"));
        }

        leaf({.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT}, .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)), .color = section_title_bg_color})
            leaf_text("Samples", {.font_size = font_size, .color = section_title_text_color});
        leaf({
            .size = {
                .width = LEAF_SIZE_FULL,
                .height = LEAF_SIZE_FIT
            },
            .child_gap = NAUI_DPI(6),
            .child_cross_gap = NAUI_DPI(6),
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .wrap_children = true
        })
        {
            for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.samples); i++)
                uph_resource_list_render_resource(i, UPH_RESOURCE_SAMPLE, context_menu, leaf_id_indexed("uph_sample_list_item", i));
            uph_resource_list_plus_box(UPH_RESOURCE_SAMPLE, leaf_id("uph_sample_list_plus"));
        }

        leaf({.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT}, .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)), .color = section_title_bg_color})
            leaf_text("Automations", {.font_size = font_size, .color = section_title_text_color});
        leaf({
            .size = {
                .width = LEAF_SIZE_FULL,
                .height = LEAF_SIZE_FIT
            },
            .child_gap = NAUI_DPI(6),
            .child_cross_gap = NAUI_DPI(6),
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .wrap_children = true
        })
        {
            for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.automations); i++)
                uph_resource_list_render_resource(i, UPH_RESOURCE_AUTOMATION, context_menu, leaf_id_indexed("uph_automation_list_item", i));
            uph_resource_list_plus_box(UPH_RESOURCE_AUTOMATION, leaf_id("uph_automation_list_plus"));
        }
    }

    if (uph_ui_menu_item(context_menu, "Rename", leaf_id("uph_pattern_rename")))
        uph_state.shared.selected_resource.renaming = true;

    if (uph_ui_menu_item(context_menu, "Remove", leaf_id("uph_pattern_remove")))
    {
        uph_resources_remove_pattern(uph_state.shared.selected_resource.index);
        if (uph_state.shared.selected_resource.index > 0 && uph_state.shared.selected_resource.index == naui_list_len(uph_state.project.midi_patterns))
            uph_state.shared.selected_resource.index--;
        else if (naui_list_len(uph_state.project.midi_patterns) == 0)
            uph_state.shared.selected_resource.type = UPH_RESOURCE_NONE;
        uph_state.shared.selected_resource.renaming = false;
    }
    if (uph_ui_menu_item(context_menu, "Duplicate", leaf_id("uph_pattern_duplicate")))
        uph_resources_copy_pattern(uph_state.shared.selected_resource.index);
}