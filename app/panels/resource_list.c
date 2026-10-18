NAUI_PANEL(uph_resource_list)

#define UPH_RESOURCE_LIST_ADD_SAMPLE_DIALOG_KEY "uph_add_sample"

static struct
{
    Uph_ResourceType type;
    Uph_ResourceIndex index;
    Naui_String old_name;
    bool active;
}
_Uph_ResourceListRename;

static struct
{
    Uph_ResourceType type;
    Uph_ResourceIndex index;
}
_Uph_ResourceListAnchor;

static bool _Uph_ResourceListItemHovered;

static const Uph_ResourceType uph_resource_list_types[] = { UPH_RESOURCE_PATTERN, UPH_RESOURCE_SAMPLE, UPH_RESOURCE_AUTOMATION };
#define UPH_RESOURCE_LIST_TYPE_COUNT ((uint32_t)(sizeof(uph_resource_list_types) / sizeof(uph_resource_list_types[0])))

static void uph_resource_list_on_attach(void)
{
    
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

    const Leaf_Color fg_color = naui_theme_color("uph_resource_list_item_fg_color");
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
            fg_color,
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

    const Leaf_Color fg_color = naui_theme_color("uph_resource_list_item_fg_color");
    uph_ui_waveform_zoomable(
        (Naui_Vec2) { box.x, box.y },
        (Naui_Vec2) { box.width, box.height },
        fg_color,
        zoom,
        start_offset,
        box,
        sample
    );
}

static void uph_automation_list_custom_draw(Leaf_BoundingBox box, void **user_data)
{
    const Uph_Automation *automation = (const Uph_Automation*)*user_data;
    if (naui_list_len(automation->points) == 0 || box.width <= 0.0f || box.height <= 0.0f)
        return;

    const double length = uph_calculate_automation_length(automation);
    if (length <= 0.0)
        return;

    const Leaf_Color fg_color = naui_theme_color("uph_resource_list_item_fg_color");
    const Leaf_Color fill_color = { fg_color.r, fg_color.g, fg_color.b, (uint8_t)(fg_color.a * 0.3f) };

    naui_push_clip_rect(box.x, box.y, box.width, box.height);
    uph_ui_automation_curve(
        automation,
        box.x,
        (float)((double)box.width / length),
        box.y,
        box.height,
        fg_color,
        fill_color,
        1.0f
    );
    naui_pop_clip_rect();
}

static Naui_String *uph_resource_list_name(Uph_ResourceType type, Uph_ResourceIndex index)
{
    if (type == UPH_RESOURCE_PATTERN && index < (uint32_t)naui_list_len(uph_state.project.midi_patterns))
        return &uph_state.project.midi_patterns[index].name;

    if (type == UPH_RESOURCE_SAMPLE && index < (uint32_t)naui_list_len(uph_state.project.samples))
        return &uph_state.project.samples[index].name;

    if (type == UPH_RESOURCE_AUTOMATION && index < (uint32_t)naui_list_len(uph_state.project.automations))
        return &uph_state.project.automations[index].name;

    return NULL;
}

static uint32_t uph_resource_list_count(Uph_ResourceType type)
{
    if (type == UPH_RESOURCE_PATTERN)
        return (uint32_t)naui_list_len(uph_state.project.midi_patterns);

    if (type == UPH_RESOURCE_SAMPLE)
        return (uint32_t)naui_list_len(uph_state.project.samples);

    if (type == UPH_RESOURCE_AUTOMATION)
        return (uint32_t)naui_list_len(uph_state.project.automations);

    return 0;
}

static bool *uph_resource_list_selected_flag(Uph_ResourceType type, Uph_ResourceIndex index)
{
    if (index >= uph_resource_list_count(type))
        return NULL;

    if (type == UPH_RESOURCE_PATTERN)
        return &uph_state.project.midi_patterns[index].selected;

    if (type == UPH_RESOURCE_SAMPLE)
        return &uph_state.project.samples[index].selected;

    if (type == UPH_RESOURCE_AUTOMATION)
        return &uph_state.project.automations[index].selected;

    return NULL;
}

static bool uph_resource_list_is_selected(Uph_ResourceType type, Uph_ResourceIndex index)
{
    const bool *flag = uph_resource_list_selected_flag(type, index);
    return flag && *flag;
}

static void uph_resource_list_clear_selection(void)
{
    for (uint32_t t = 0; t < UPH_RESOURCE_LIST_TYPE_COUNT; t++)
    {
        const Uph_ResourceType type = uph_resource_list_types[t];
        for (uint32_t i = 0; i < uph_resource_list_count(type); i++)
            *uph_resource_list_selected_flag(type, i) = false;
    }
}

static void uph_resource_list_sync_selection(void)
{
    bool *flag = uph_resource_list_selected_flag(uph_state.shared.selected_resource.type, uph_state.shared.selected_resource.index);
    if (flag && *flag)
        return;

    uph_resource_list_clear_selection();
    if (flag)
        *flag = true;
}

static void uph_resource_list_set_primary(Uph_ResourceType type, Uph_ResourceIndex index)
{
    uph_state.shared.selected_resource.type = type;
    uph_state.shared.selected_resource.index = index;
    uph_action_refresh_block_defaults();
}

static void uph_resource_list_select_single(Uph_ResourceType type, Uph_ResourceIndex index)
{
    uph_resource_list_clear_selection();
    *uph_resource_list_selected_flag(type, index) = true;
    uph_resource_list_set_primary(type, index);
    _Uph_ResourceListAnchor.type = type;
    _Uph_ResourceListAnchor.index = index;
}

static bool uph_resource_list_find_other_selected(Uph_ResourceType skip_type, Uph_ResourceIndex skip_index, Uph_ResourceType *out_type, Uph_ResourceIndex *out_index)
{
    for (uint32_t t = 0; t < UPH_RESOURCE_LIST_TYPE_COUNT; t++)
    {
        const Uph_ResourceType type = uph_resource_list_types[t];
        for (uint32_t i = 0; i < uph_resource_list_count(type); i++)
        {
            if ((type == skip_type && i == skip_index) || !uph_resource_list_is_selected(type, i))
                continue;

            *out_type = type;
            *out_index = i;
            return true;
        }
    }
    return false;
}

static void uph_resource_list_toggle(Uph_ResourceType type, Uph_ResourceIndex index)
{
    bool *flag = uph_resource_list_selected_flag(type, index);
    _Uph_ResourceListAnchor.type = type;
    _Uph_ResourceListAnchor.index = index;

    if (!*flag)
    {
        *flag = true;
        uph_resource_list_set_primary(type, index);
        return;
    }

    const bool is_primary = uph_state.shared.selected_resource.type == type && uph_state.shared.selected_resource.index == index;
    if (!is_primary)
    {
        *flag = false;
        return;
    }

    Uph_ResourceType other_type;
    Uph_ResourceIndex other_index;
    if (uph_resource_list_find_other_selected(type, index, &other_type, &other_index))
    {
        *flag = false;
        uph_resource_list_set_primary(other_type, other_index);
    }
}

static void uph_resource_list_select_range(Uph_ResourceType type, Uph_ResourceIndex index)
{
    if (_Uph_ResourceListAnchor.type != type || _Uph_ResourceListAnchor.index >= uph_resource_list_count(type))
    {
        uph_resource_list_select_single(type, index);
        return;
    }

    const Uph_ResourceIndex anchor = _Uph_ResourceListAnchor.index;
    const Uph_ResourceIndex low = anchor < index ? anchor : index;
    const Uph_ResourceIndex high = anchor < index ? index : anchor;

    uph_resource_list_clear_selection();
    for (Uph_ResourceIndex i = low; i <= high; i++)
        *uph_resource_list_selected_flag(type, i) = true;

    uph_resource_list_set_primary(type, index);
}

static void uph_resource_list_click_select(Uph_ResourceType type, Uph_ResourceIndex index)
{
    if (uph_ui_ctrl_down())
        uph_resource_list_toggle(type, index);
    else if (uph_ui_shift_down())
        uph_resource_list_select_range(type, index);
    else
        uph_resource_list_select_single(type, index);
}

static void uph_resource_list_finish_rename(void)
{
    _Uph_ResourceListRename.active = false;

    Naui_String *name = uph_resource_list_name(_Uph_ResourceListRename.type, _Uph_ResourceListRename.index);
    if (!name || naui_strings_equal(*name, _Uph_ResourceListRename.old_name, true))
        return;

    Uph_ActionResourceRename data = {
        .resource_index = _Uph_ResourceListRename.index,
        .old_name = _Uph_ResourceListRename.old_name,
        .new_name = *name
    };

    const char *action = NULL;
    if (_Uph_ResourceListRename.type == UPH_RESOURCE_PATTERN)
        action = UPH_ACTION_PATTERN_RENAME;

    else if (_Uph_ResourceListRename.type == UPH_RESOURCE_SAMPLE)
        action = UPH_ACTION_SAMPLE_RENAME;

    else if (_Uph_ResourceListRename.type == UPH_RESOURCE_AUTOMATION)
        action = UPH_ACTION_AUTOMATION_RENAME;

    if (action)
        naui_action_execute_stack(action, data);
}

static void uph_resource_list_update_rename(void)
{
    const Uph_ResourceType type = uph_state.shared.selected_resource.type;
    const Uph_ResourceIndex index = uph_state.shared.selected_resource.index;
    const bool renaming = uph_state.shared.selected_resource.renaming;

    if (_Uph_ResourceListRename.active && (!renaming || _Uph_ResourceListRename.type != type || _Uph_ResourceListRename.index != index))
        uph_resource_list_finish_rename();

    if (renaming && !_Uph_ResourceListRename.active)
    {
        Naui_String *name = uph_resource_list_name(type, index);
        if (!name)
        {
            uph_state.shared.selected_resource.renaming = false;
            return;
        }

        _Uph_ResourceListRename.type = type;
        _Uph_ResourceListRename.index = index;
        _Uph_ResourceListRename.old_name = *name;
        _Uph_ResourceListRename.active = true;
    }
}

bool uph_resource_list_box(Naui_String *name, Leaf_CustomDrawFn content_draw, Leaf_DataSlice content_draw_data, Leaf_ID id, float visual_scale, bool hovered, bool selected, bool renaming, const char *placeholder_name)
{
    const float final_scale = 0.8f + naui_ease_out_elastic(visual_scale) * 0.2f;
    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_FIXED(NAUI_DPI(152)),
            .height = LEAF_SIZE_DERIVED
        },
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .aspect_ratio = 2.2f
    })
    leaf({
        .size = {LEAF_SIZE_PERCENT(final_scale), LEAF_SIZE_PERCENT(final_scale)},
        .padding = LEAF_PADDING_ALL(NAUI_DPI(2)),
        .border = selected ? (Leaf_Border){
            .width = NAUI_DPI(1),
            .sides = LEAF_SIDE_ALL,
            .color = {naui_theme_color("uph_resource_list_item_selected_border_color")}
        } : (Leaf_Border){0},
        .color = {naui_theme_color("uph_resource_list_item_bg_color")},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), LEAF_CORNER_ALL),
        .clip_children = true
    }) {
        if (renaming)
        {
            if (uph_ui_textfield(name, id, UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE, placeholder_name))
            {
                if (name->length == 0)
					*name = naui_string_from_cstr(placeholder_name);

				Uph_ActionTrackRename data = {
					.new_name = naui_string_from_cstr(placeholder_name)
				};

				naui_action_execute(UPH_ACTION_TRACK_RENAME, &data);
                uph_state.shared.selected_resource.renaming = false;
            }
        }
        else
        {
            leaf_text(name->data, {
                .color = {naui_theme_color("uph_resource_list_item_text_color")},
                .font_size = {NAUI_DPI(naui_theme_float("uph_ui_font_size"))}
            });
        }
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
            .custom_draw = content_draw,
            .custom_draw_data = content_draw_data
        });
    }
    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void uph_resource_list_render_resource(uint32_t index, Uph_ResourceType type, Uph_UIMenuID context_menu, Leaf_ID id)
{
    bool is_primary = uph_state.shared.selected_resource.index == index && uph_state.shared.selected_resource.type == type;
    bool is_selected = uph_resource_list_is_selected(type, index);
    bool is_renaming = is_primary && uph_state.shared.selected_resource.renaming;
    bool hovered = !is_renaming && uph_ui_widget_hovered(id);

    if (hovered)
    {
        _Uph_ResourceListItemHovered = true;
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
        {
            uph_state.shared.selected_resource.renaming = false;
            if (is_selected)
                uph_resource_list_set_primary(type, index);
            else
                uph_resource_list_select_single(type, index);
            uph_ui_open_context_menu(context_menu);
        }
        else if (naui_mouse_double_clicked(NAUI_MOUSE_LEFT) && !uph_ui_ctrl_down() && !uph_ui_shift_down())
        {
            uph_resource_list_select_single(type, index);
            uph_state.shared.selected_resource.renaming = true;
        }
    }

    if (type == UPH_RESOURCE_PATTERN)
    {
        Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[index];
        pattern->visual_lifetime = NAUI_CLAMP01(pattern->visual_lifetime + naui_delta_time());

        if (uph_resource_list_box(
            &pattern->name,
            (Leaf_CustomDrawFn)uph_pattern_list_custom_draw,
            LEAF_DATA_SLICE(pattern),
            id,
            pattern->visual_lifetime,
            hovered,
            is_selected,
            is_renaming,
            NAUI_TR("pattern.default.name")
        ))
            uph_resource_list_click_select(type, index);
    }
    else if (type == UPH_RESOURCE_SAMPLE)
    {
        Uph_Sample *sample = &uph_state.project.samples[index];
        sample->visual_lifetime = NAUI_CLAMP01(sample->visual_lifetime + naui_delta_time());

        if (uph_resource_list_box(
            &sample->name,
            (Leaf_CustomDrawFn)uph_sample_list_custom_draw,
            LEAF_DATA_SLICE(sample),
            id,
            sample->visual_lifetime,
            hovered,
            is_selected,
            is_renaming,
            NAUI_TR("sample.default.name")
        ))
            uph_resource_list_click_select(type, index);
    }
    else if (type == UPH_RESOURCE_AUTOMATION)
    {
        Uph_Automation *automation = &uph_state.project.automations[index];
        automation->visual_lifetime = NAUI_CLAMP01(automation->visual_lifetime + naui_delta_time());

        if (uph_resource_list_box(
            &automation->name,
            (Leaf_CustomDrawFn)uph_automation_list_custom_draw,
            LEAF_DATA_SLICE(automation),
            id,
            automation->visual_lifetime,
            hovered,
            is_selected,
            is_renaming,
            NAUI_TR("automation.default.name")
        ))
            uph_resource_list_click_select(type, index);
    }
}

static void uph_resource_list_sample_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
    (void)user_data;
    if (is_cancelled || result->count == 0)
        return;

    const uint32_t sample_count_before = (uint32_t)naui_list_len(uph_state.project.samples);

    for (uint32_t i = 0; i < result->count; i++)
    {
        if (!uph_project_add_file(&uph_state.project, result->paths[i]))
            naui_log(NAUI_LOG_WARNING, "Couldn't add sample (%s)", result->paths[i].data);
    }

    const uint32_t sample_count_after = (uint32_t)naui_list_len(uph_state.project.samples);
    if (sample_count_after > sample_count_before)
    {
        uph_state.shared.selected_resource.type = UPH_RESOURCE_SAMPLE;
        uph_state.shared.selected_resource.index = sample_count_after - 1;
        uph_state.shared.selected_resource.renaming = false;
        uph_action_refresh_block_defaults();
    }
}

static void uph_resource_list_open_sample_dialog(void)
{
    if (!naui_dialog_open_file(UPH_RESOURCE_LIST_ADD_SAMPLE_DIALOG_KEY, uph_audio_engine_extensions, uph_resource_list_sample_picked, NULL))
        return;

    naui_dialog_title_set(UPH_RESOURCE_LIST_ADD_SAMPLE_DIALOG_KEY, NAUI_TR("resource_list.sample.add"));
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
            if (type == UPH_RESOURCE_SAMPLE)
            {
                uph_resource_list_open_sample_dialog();
                return;
            }

            uph_state.shared.selected_resource.type = type;
            uph_state.shared.selected_resource.renaming = false;
            uph_state.shared.song_timeline_current_block_start_offset = 0;
            uph_state.shared.song_timeline_current_block_length = (double)uph_state.project.time_signature.denominator;

            if (type == UPH_RESOURCE_PATTERN)
            {
                uph_state.shared.selected_resource.index = naui_list_len(uph_state.project.midi_patterns);
				naui_action_execute_stack(UPH_ACTION_PATTERN_CREATE, (Uph_ActionResourceCreate){0});
            }
            else if (type == UPH_RESOURCE_AUTOMATION)
            {
                uph_state.shared.selected_resource.index = naui_list_len(uph_state.project.automations);
				naui_action_execute_stack(UPH_ACTION_AUTOMATION_CREATE, (Uph_ActionResourceCreate){0});
            }
        }
    }
}

static void uph_resource_list_remove(Uph_ResourceType type, Uph_ResourceIndex index)
{
    if (type == UPH_RESOURCE_PATTERN)
    {
        Uph_ActionPatternDelete data = { .resource_index = index };
        naui_action_execute_stack(UPH_ACTION_PATTERN_DELETE, data);
    }
    else if (type == UPH_RESOURCE_SAMPLE)
    {
        Uph_ActionSampleDelete data = { .resource_index = index };
        naui_action_execute_stack(UPH_ACTION_SAMPLE_DELETE, data);
    }
    else if (type == UPH_RESOURCE_AUTOMATION)
    {
        Uph_ActionAutomationDelete data = { .resource_index = index };
        naui_action_execute_stack(UPH_ACTION_AUTOMATION_DELETE, data);
    }
}

static void uph_resource_list_duplicate(Uph_ResourceType type, Uph_ResourceIndex index)
{
    if (type == UPH_RESOURCE_PATTERN)
    {
        Uph_ActionPatternDuplicate data = { .source_index = index };
        naui_action_execute_stack(UPH_ACTION_PATTERN_DUPLICATE, data);
    }
    else if (type == UPH_RESOURCE_SAMPLE)
    {
        Uph_ActionSampleDuplicate data = { .source_index = index };
        naui_action_execute_stack(UPH_ACTION_SAMPLE_DUPLICATE, data);
    }
    else if (type == UPH_RESOURCE_AUTOMATION)
    {
        Uph_ActionAutomationDuplicate data = { .source_index = index };
        naui_action_execute_stack(UPH_ACTION_AUTOMATION_DUPLICATE, data);
    }
}

static void uph_resource_list_remove_selected(void)
{
    naui_action_group_start(UPH_ACTION_RESOURCES_DELETE_GROUP);
    for (uint32_t t = 0; t < UPH_RESOURCE_LIST_TYPE_COUNT; t++)
    {
        const Uph_ResourceType type = uph_resource_list_types[t];
        for (uint32_t i = uph_resource_list_count(type); i > 0; i--)
        {
            if (uph_resource_list_is_selected(type, i - 1))
                uph_resource_list_remove(type, i - 1);
        }
    }
    naui_action_group_end();
}

static void uph_resource_list_duplicate_selected(void)
{
    naui_action_group_start(UPH_ACTION_RESOURCES_DUPLICATE_GROUP);
    for (uint32_t t = 0; t < UPH_RESOURCE_LIST_TYPE_COUNT; t++)
    {
        const Uph_ResourceType type = uph_resource_list_types[t];
        const uint32_t count = uph_resource_list_count(type);
        for (uint32_t i = 0; i < count; i++)
        {
            bool *flag = uph_resource_list_selected_flag(type, i);
            if (!*flag)
                continue;

            *flag = false;
            uph_resource_list_duplicate(type, i);

            bool *copy_flag = uph_resource_list_selected_flag(type, uph_resource_list_count(type) - 1);
            if (copy_flag)
                *copy_flag = true;
        }
    }
    naui_action_group_end();
}

static void uph_resource_list_on_update(void)
{
	naui_panel_set_title(naui_panel_current(), NAUI_TR("resource_list.title"));

	const int32_t icon_size = NAUI_DPI(16);
    uph_resource_list_update_rename();
    uph_resource_list_sync_selection();

    const bool menu_was_open = uph_ui_any_menu_open();
    _Uph_ResourceListItemHovered = false;

    if (naui_panel_is_hovered(naui_panel_current()) && naui_key_pressed(NAUI_KEY_DELETE) &&
        !uph_state.shared.selected_resource.renaming && uph_state.shared.selected_resource.type != UPH_RESOURCE_NONE)
        uph_resource_list_remove_selected();

    Uph_UIMenuID context_menu = uph_ui_context_menu();
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Color section_title_text_color = naui_theme_color("uph_resource_list_section_text_color");
    const Naui_Color section_title_bg_color = naui_theme_color("uph_resource_list_section_bg_color");
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
	static float scroll = 0.0f;

      Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(
        UPH_UI_SCROLL_DIRECTION_VERTICAL,
        &scroll,
        leaf_id("uph_resource_list_scrollbar")
    );
    leaf({
        .size = {
            .width = LEAF_SIZE_FULL,
            .height = LEAF_SIZE_FIT
        },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .child_gap = NAUI_DPI(8)
    }) {

        leaf({.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT}, .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)), .color = section_title_bg_color, .direction = LEAF_DIRECTION_HORIZONTAL, .child_gap = NAUI_DPI(4), .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER} }, )
		{
			leaf({
				.size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
				.color = section_title_text_color,
				.image = (void*)naui_asset_image("uph_icon_piano")
			});
            leaf_text(NAUI_TR("resource_list.patterns.title"), {.font_size = font_size, .color = section_title_text_color});
		}
    
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

        leaf({.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT}, .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)), .color = section_title_bg_color, .direction = LEAF_DIRECTION_HORIZONTAL, .child_gap = NAUI_DPI(4), .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER} }, )
		{
			leaf({
				.size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
				.color = section_title_text_color,
				.image = (void*)naui_asset_image("uph_icon_wave")
			});
            leaf_text(NAUI_TR("resource_list.samples.title"), {.font_size = font_size, .color = section_title_text_color});
		}
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

        leaf({.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT}, .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)), .color = section_title_bg_color, .direction = LEAF_DIRECTION_HORIZONTAL, .child_gap = NAUI_DPI(4), .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER} }, )
		{
			leaf({
				.size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
				.color = section_title_text_color,
				.image = (void*)naui_asset_image("uph_icon_automation")
			});
            leaf_text(NAUI_TR("resource_list.automations.title"), {.font_size = font_size, .color = section_title_text_color});
		}
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
    uph_ui_end_scroll_container(&scroll_container);

    if (uph_ui_menu_item(context_menu, NAUI_TR("resource_list.rename"), leaf_id("uph_pattern_rename")))
        uph_state.shared.selected_resource.renaming = true;

    if (uph_ui_menu_item(context_menu, NAUI_TR("resource_list.remove"), leaf_id("uph_pattern_remove")))
        uph_resource_list_remove_selected();

    if (uph_ui_menu_item(context_menu, NAUI_TR("resource_list.duplicate"), leaf_id("uph_pattern_duplicate")))
        uph_resource_list_duplicate_selected();

    if (!menu_was_open && !_Uph_ResourceListItemHovered && naui_any_mouse_pressed())
    {
        uph_resource_list_clear_selection();
        uph_resource_list_sync_selection();
    }
}