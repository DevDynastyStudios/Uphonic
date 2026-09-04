NAUI_PANEL(uph_mixer)

typedef struct
{
    Uph_Track *track;
}
Uph_VolumePeaksCustomDrawData;

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

static Naui_Gradient uph_mixer_peak_gradient(void)
{
    return (Naui_Gradient) {
        .color1   = leaf_rgb(255, 255, 0),
        .color2   = leaf_rgb(0, 255, 0),
        .percent1 = 0.0f,
        .percent2 = 1.0f,
        .angle    = LEAF_DEG(90.0f),
    };
}

static void uph_mixer_draw_peak_bar(Leaf_BoundingBox box, float x, float width, float peak)
{
    naui_fill_rect(
        (Naui_Vec2) { x, box.y },
        (Naui_Vec2) { width, box.height },
        naui_theme_color("uph_track_header_border_color"),
        0.0f,
        NAUI_CORNER_NONE
    );
    naui_push_clip_rect(x, box.y + box.height * (1.0f - peak), width, box.height);
    naui_fill_gradient_rect(
        (Naui_Vec2) { x, box.y },
        (Naui_Vec2) { width, box.height },
        uph_mixer_peak_gradient(),
        0.0f,
        NAUI_CORNER_NONE
    );
    naui_pop_clip_rect();
}

#define uph_mixer_PEAK_SMOOTH_RATE 30.0f
static void uph_mixer_volume_peaks_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    Uph_VolumePeaksCustomDrawData *data = (Uph_VolumePeaksCustomDrawData*)user_data;
    Uph_Track *track = data->track;

    const float gap = NAUI_DPI(1.0f);

    track->smooth_peak_left  = NAUI_LERP(track->smooth_peak_left,  track->peak_left,  naui_delta_time() * uph_mixer_PEAK_SMOOTH_RATE);
    track->smooth_peak_right = NAUI_LERP(track->smooth_peak_right, track->peak_right, naui_delta_time() * uph_mixer_PEAK_SMOOTH_RATE);

    float half_width = box.width * 0.5f;
    float bar_width  = half_width - gap * 0.5f;

    uph_mixer_draw_peak_bar(box, box.x, bar_width, track->smooth_peak_left);
    uph_mixer_draw_peak_bar(box, box.x + half_width + gap * 0.5f, bar_width, track->smooth_peak_right);
}

static void uph_mixer_render_track(Uph_Track *track)
{
    leaf({
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(80.0f)), LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(10.0f), NAUI_DPI(10.0f)),
        .color = naui_theme_color("uph_track_header_color"),
        .child_gap = NAUI_DPI(12.0f),
        .border = {
            .width = 1.0f,
            .color = naui_theme_color("uph_track_header_border_color"),
            .sides = LEAF_SIDE_ALL
        },
    })
    {
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(4.0f))},
            .color = track->color,
            .rounding = {
                .value = NAUI_DPI(4.0f),
                .corners = LEAF_CORNER_ALL
            }
        });
        leaf_text(track->name.length ? track->name.data : "Untitled Track", {
            .font_size = NAUI_DPI(12.0f),
            .color = track->name.length ? naui_theme_color("uph_ui_text_color") : naui_theme_color("uph_ui_text_disabled_color")
        });

        leaf({
            .size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_DERIVED},
            .aspect_ratio = 1.0f,
            .color = naui_theme_color("uph_track_header_border_color"),
            .rounding = {
                .value = (float)INT_MAX,
                .corners = LEAF_CORNER_ALL
            }
        });

        Uph_VolumePeaksCustomDrawData peaks_data = {
            .track = track
        };
        leaf({
            .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_GROW},
            .custom_draw = uph_mixer_volume_peaks_custom_draw,
            .custom_draw_data = LEAF_DATA_SLICE(peaks_data)
        });
    }
}

static void uph_mixer_on_update(void)
{
    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
    })
    {
        for (uint32_t i = 0; i < naui_list_len(uph_state.project.tracks); i++)
        {
            Uph_Track *track = &uph_state.project.tracks[i];
            uph_mixer_render_track(track);
        }
    }
}