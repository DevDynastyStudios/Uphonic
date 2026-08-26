NAUI_PANEL(uph_sample_list)

static void uph_sample_list_on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "Samples");
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

static void uph_sample_list_waveform(Leaf_BoundingBox box, void **user_data) {
    Uph_Sample* sample = (Uph_Sample*)*user_data;

    uph_ui_waveform_zoomable(
        (Naui_Vec2) { box.x, box.y },
        (Naui_Vec2) { box.width, box.height },
        LEAF_COLOR_WHITE,
        1.0,
        0.0,
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
        uint32_t sampleCount = (uint32_t)naui_list_len(uph_state.project.samples);
        for (uint32_t i = 0; i < sampleCount; i++) {
            Uph_Sample *sample = &uph_state.project.samples[i];

            const Leaf_ID sample_id = leaf_id_indexed("uph_sample_list_sample", i);
            leaf({
                .id = sample_id,
                .custom_draw = (Leaf_CustomDrawFn)uph_sample_list_waveform,
                .custom_draw_data = LEAF_DATA_SLICE(sample),
                .size = {
                    .width = LEAF_SIZE_FIXED(NAUI_DPI(150)),
                    .height = LEAF_SIZE_DERIVED
                },
                .padding = LEAF_PADDING_ALL(NAUI_DPI(2)),
                .aspect_ratio = 2.2f,
                .border = {
                    .width = NAUI_DPI(1.0f),
                    .sides = LEAF_SIDE_ALL,
                    .color = leaf_rgb(145, 111, 205)
                },
                .color = leaf_rgb(108, 83, 154),
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), LEAF_CORNER_ALL),
                .clip_children = true
            }) {
                leaf_text(sample->name.data, {
                    .color = LEAF_COLOR_WHITE,
                    .font_size = NAUI_DPI(13.0f)
                });
            }
        }

        leaf({
            .size = {
                .width = LEAF_SIZE_FIXED(NAUI_DPI(150)),
                .height = LEAF_SIZE_DERIVED
            },
            .padding = LEAF_PADDING_ALL(NAUI_DPI(2)),
            .aspect_ratio = 2.2f,
            .border = {
                .width = NAUI_DPI(1.0f),
                .sides = LEAF_SIDE_ALL,
                .color = naui_theme_color("uph_ui_frame_border")
            },
            .color = naui_theme_color("uph_ui_frame_bg_color"),
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
            }) { }
        }
    }
}