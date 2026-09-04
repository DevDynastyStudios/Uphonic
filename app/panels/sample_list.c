NAUI_PANEL(uph_sample_list)

static void uph_sample_list_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, NAUI_TR("samples.title"));
}

static void uph_sample_list_on_detach(void)
{
    
}

static void uph_sample_list_on_open(void)
{
    
}

static void uph_sample_list_on_close(void)
{
    
}

static void uph_sample_list_waveform(Leaf_BoundingBox box, void **user_data)
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

static void uph_sample_list_on_update(void)
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
        Uph_UIMenuID context_menu = uph_ui_context_menu();

        uint32_t sample_count = (uint32_t)naui_list_len(uph_state.project.samples);
        for (uint32_t i = 0; i < sample_count; i++) {
            Uph_Sample *sample = &uph_state.project.samples[i];

            const Leaf_ID id = leaf_id_indexed("uph_sample_list_sample", i);
			bool hovered = uph_ui_widget_hovered(id);

			if (hovered)
				naui_request_cursor(NAUI_CURSOR_HAND);

            if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && hovered)
            {
                uph_state.shared.selected_resource.index = i;
                uph_state.shared.selected_resource.type = UPH_RESOURCE_SAMPLE;
                uph_ui_open_context_menu(context_menu);
            }

            if (uph_ui_list_box(
                sample->name.data,
                (Leaf_CustomDrawFn)uph_sample_list_waveform,
                LEAF_DATA_SLICE(sample),
                id,
                uph_state.shared.selected_resource.index == i
            ))
            {
                uph_state.shared.selected_resource.index = i;
                uph_state.shared.selected_resource.type = UPH_RESOURCE_SAMPLE;

                Uph_Sample *sample = &uph_state.project.samples[i];
                Uph_SampleData *sample_data = &uph_state.project.sample_data[sample->data_index];
                double beats = ceil(uph_frames_to_beats(sample_data->frame_count, uph_state.settings.audio.sample_rate, uph_state.project.bpm));
                uph_state.shared.song_timeline_current_block_length = beats;
            }
        }

        if (uph_ui_menu_item(context_menu, "Remove", leaf_id("uph_sample_remove")))
        {
            uph_resources_remove_sample(uph_state.shared.selected_resource.index);
            if (uph_state.shared.selected_resource.index > 0 && uph_state.shared.selected_resource.index == naui_list_len(uph_state.project.samples))
                uph_state.shared.selected_resource.index--;
            else if (naui_list_len(uph_state.project.samples) == 0)
                uph_state.shared.selected_resource.type = UPH_RESOURCE_NONE;
        }
        if (uph_ui_menu_item(context_menu, "Duplicate", leaf_id("uph_sample_duplicate")))
            uph_resources_copy_sample(uph_state.shared.selected_resource.index);

        const Leaf_ID plus_id = leaf_id("uph_sample_list_plus");
        uph_ui_list_plus_box(plus_id);
    }
}