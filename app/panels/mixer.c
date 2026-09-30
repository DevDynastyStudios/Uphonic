NAUI_PANEL(uph_mixer)

static void uph_mixer_on_attach(void)
{
	Naui_PanelID this = naui_current_panel();
	naui_panel_set_title(this, NAUI_TR("mixer.title"));
}

static void uph_mixer_on_detach(void)
{
	
}

static void uph_mixer_on_open(void)
{
	
}

static void uph_mixer_on_close(void)
{
	
}

static inline float uph_linear_to_db(float linear)
{
    if (linear <= 0.0001f) return -80.0f;
    return 20.0f * log10f(linear);
}

static inline float uph_db_to_linear(float db)
{
    return powf(10.0f, db / 20.0f);
}

#define UPH_MIXER_DB_MIN -60.0f
#define UPH_MIXER_DB_MAX 6.0f

static inline float uph_mixer_db_to_fraction(float db)
{
    float normalized = (db - UPH_MIXER_DB_MIN) / (UPH_MIXER_DB_MAX - UPH_MIXER_DB_MIN);
    return NAUI_CLAMP(normalized, 0.0f, 1.0f);
}

static void uph_mixer_draw_db_ruler(Leaf_BoundingBox bounding_box, void *unused)
{
    static const float db_ticks[] = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f };
    const int tick_count = sizeof(db_ticks) / sizeof(db_ticks[0]);

    const float font_size = NAUI_DPI(naui_theme_float("uph_mixer_ruler_font_size"));
    const Naui_Color label_color = naui_theme_color("uph_mixer_ruler_color");
    const float tick_length = NAUI_DPI(4.0f);
    const float label_gap = NAUI_DPI(3.0f);

    for (int i = 0; i < tick_count; i++)
    {
        float y = bounding_box.y + (1.0f - uph_mixer_db_to_fraction(db_ticks[i])) * bounding_box.height;

        char label[8];
        snprintf(label, sizeof(label), "%.0f", db_ticks[i]);

        Naui_Vec2 text_size = naui_measure_text(label, (uint32_t)strlen(label), font_size, 0);
        float label_y = NAUI_CLAMP(y - text_size.y * 0.5f, bounding_box.y, bounding_box.y + bounding_box.height - text_size.y);

        naui_draw_text(
            (Naui_Vec2){bounding_box.x + tick_length + label_gap, label_y},
            label, font_size, 0, label_color
        );
    }
}

static void uph_mixer_draw_peak_bars(Leaf_BoundingBox bounding_box, Uph_Track **data)
{
    Uph_Track *track = *data;
    const float gap = NAUI_DPI(1.0f);

    const float smooth_speed = naui_delta_time() * 30.0f;
    track->smooth_peak_left = NAUI_LERP(track->smooth_peak_left, uph_state.shared.song_timeline_playing ? track->peak_left : 0.0f, smooth_speed);
    track->smooth_peak_right = NAUI_LERP(track->smooth_peak_right, uph_state.shared.song_timeline_playing ? track->peak_right : 0.0f, smooth_speed);

    float left_fraction = uph_mixer_db_to_fraction(uph_linear_to_db(track->smooth_peak_left));
    float right_fraction = uph_mixer_db_to_fraction(uph_linear_to_db(track->smooth_peak_right));

    naui_fill_rect(
        (Naui_Vec2){bounding_box.x, bounding_box.y + (1.0f - left_fraction) * bounding_box.height},
        (Naui_Vec2){bounding_box.width * 0.5f - gap, bounding_box.height * left_fraction},
        leaf_rgb(100, 220, 100),
        NAUI_DPI(6.0f),
        NAUI_CORNER_TL | NAUI_CORNER_TR
    );

    naui_fill_rect(
        (Naui_Vec2){bounding_box.x + bounding_box.width * 0.5f + gap, bounding_box.y + (1.0f - right_fraction) * bounding_box.height},
        (Naui_Vec2){bounding_box.width * 0.5f - gap, bounding_box.height * right_fraction},
        leaf_rgb(100, 220, 100),
        NAUI_DPI(6.0f),
        NAUI_CORNER_TL | NAUI_CORNER_TR
    );
}

typedef struct Uph_MixerVolumeArrowData
{
    Uph_Track *track;
    uint64_t id;
} Uph_MixerVolumeArrowData;

static void uph_mixer_draw_volume_arrow(Leaf_BoundingBox bounding_box, Uph_MixerVolumeArrowData *data)
{
    Uph_Track *track = data->track;

    const float arrow_width = NAUI_DPI(10.0f);
    const float arrow_height = NAUI_DPI(12.0f);
    const float hit_padding = NAUI_DPI(4.0f);

    float db = uph_linear_to_db(track->volume);
    float fraction = uph_mixer_db_to_fraction(db);
    float y = bounding_box.y + (1.0f - fraction) * bounding_box.height;

    Leaf_BoundingBox hit_box = {
        .x = bounding_box.x,
        .y = y - arrow_height * 0.5f - hit_padding,
        .width = bounding_box.width,
        .height = arrow_height + hit_padding * 2.0f
    };

    Naui_Vec2 mouse = {naui_mouse_x(), naui_mouse_y()};
    bool hovered =
        naui_panel_hovered(uph_state.panels.mixer) &&
        mouse.x >= hit_box.x && mouse.x <= hit_box.x + hit_box.width &&
        mouse.y >= hit_box.y && mouse.y <= hit_box.y + hit_box.height;

    static uint64_t active_id = 0;
    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        active_id = data->id;

    bool dragging = (active_id == data->id);
    if (dragging)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
        {
            float t = 1.0f - NAUI_CLAMP((mouse.y - bounding_box.y) / bounding_box.height, 0.0f, 1.0f);
            float new_db = UPH_MIXER_DB_MIN + t * (UPH_MIXER_DB_MAX - UPH_MIXER_DB_MIN);
            track->volume = uph_db_to_linear(new_db);

            y = bounding_box.y + (1.0f - t) * bounding_box.height;
        }
        else
        {
            active_id = 0;
        }
    }

    Naui_Color color = naui_theme_color("uph_mixer_arrow_color");

    Naui_Vec2 arrow[3] = {
        {bounding_box.x,               y},                          // tip
        {bounding_box.x + arrow_width, y - arrow_height * 0.5f},    // top
        {bounding_box.x + arrow_width, y + arrow_height * 0.5f}     // bottom
    };
    naui_fill_polygon(arrow, 3, color);
}

static void uph_mixer_render_track(Uph_Track *track)
{
    uint64_t track_id = (uint64_t)track;
    Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    Leaf_ID id = leaf_id_indexed("uph_mixer_track", track_id);
    if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && naui_panel_hovered(naui_current_panel()) && leaf_hovered(id))
    {
        uph_state.shared.selected_mixer_track = track;
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(100)), LEAF_SIZE_FULL},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP},
        .color = {
            uph_state.shared.selected_mixer_track == track ?
            naui_theme_color("uph_mixer_track_selected_bg_color") :
            naui_theme_color("uph_mixer_track_bg_color")
        },
        .border = {
            .color = {naui_theme_color("uph_mixer_track_border_color")},
            .width = 1,
            .sides = LEAF_SIDE_LEFT
        },
        .child_gap = NAUI_DPI(8)
    })
    {
        const Naui_Color text_color = naui_theme_color("uph_ui_text_color");
        const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(6))},
            .color = uph_resources_track_color(track->color_index),
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });
        leaf_text(track->name.data, { .font_size = font_size, .color = text_color });

        leaf({
            .size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_DERIVED},
            .color = {naui_theme_color("uph_ui_frame_bg_color")},
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL),
            .aspect_ratio = 1.0f
        });

        leaf({.size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_FIT}})
            uph_ui_drag_float(&track->pan, leaf_id_indexed("uph_mixer_pan", track_id), 0.01f, -1.0f, 1.0f, "%.2f", UPH_UI_DRAG_CLAMPED);

        float volume_db = uph_linear_to_db(track->volume);

        leaf({.size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_FIT}})
            if (uph_ui_drag_float(&volume_db, leaf_id_indexed("uph_mixer_volume", track_id), 0.01f, UPH_MIXER_DB_MIN, UPH_MIXER_DB_MAX, "%.2f dB", UPH_UI_DRAG_CLAMPED))
                track->volume = uph_db_to_linear(volume_db);
        
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .child_gap = NAUI_DPI(8)
        })
        {
            leaf({
                .size = {LEAF_SIZE_PERCENT(0.15f), LEAF_SIZE_FULL},
                .custom_draw = (Leaf_CustomDrawFn)uph_mixer_draw_db_ruler
            });
            leaf({
                .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
                .color = {naui_theme_color("uph_ui_frame_bg_color")},
                .custom_draw = (Leaf_CustomDrawFn)uph_mixer_draw_peak_bars,
                .custom_draw_data = LEAF_DATA_SLICE(track)
            });
            Uph_MixerVolumeArrowData arrow_data = {
                .track = track,
                .id = track_id
            };
            leaf({
                .size = {LEAF_SIZE_PERCENT(0.15f), LEAF_SIZE_FULL},
                .custom_draw = (Leaf_CustomDrawFn)uph_mixer_draw_volume_arrow,
                .custom_draw_data = LEAF_DATA_SLICE(arrow_data)
            });
        }
    }
}

static void uph_mixer_on_update(void)
{
    static float scroll = 0.0f;
    Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(
        UPH_UI_SCROLL_DIRECTION_HORIZONTAL,
        &scroll,
        leaf_id("uph_mixer_scrollbar")
    );
    for (uint32_t i = 0; i < naui_list_len(uph_state.project.tracks); i++)
    {
        Uph_Track *track = &uph_state.project.tracks[i];
        uph_mixer_render_track(track);
    }
    uph_ui_end_scroll_container(&scroll_container);
}
