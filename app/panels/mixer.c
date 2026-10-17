NAUI_PANEL(uph_mixer)

static void uph_mixer_on_attach(void)
{

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

static void uph_mixer_effect_list_swap(Uph_EffectPlugin *effects, uint32_t a, uint32_t b)
{
    Uph_EffectPlugin temp = effects[a];
    effects[a] = effects[b];
    effects[b] = temp;
}

#define UPH_MIXER_MASTER_COLOR_INDEX 0

typedef struct Uph_MixerStrip
{
    Uph_ActionTrackRef ref;
    Uph_Track *track; // NULL for the master strip
    Naui_List(Uph_EffectPlugin) *effects;
    Uph_TrackState *state;
    float *volume, *pan;
    float *peak_left, *peak_right;
    float *smooth_peak_left, *smooth_peak_right;
}
Uph_MixerStrip;

static Uph_MixerStrip uph_mixer_strip_master(void)
{
    Uph_MasterTrack *master = &uph_state.project.master_track;
    return (Uph_MixerStrip){
        .ref = uph_action_track_ref_master(),
        .effects = &master->effects,
        .state = &master->state,
        .volume = &master->volume,
        .pan = &master->pan,
        .peak_left = &master->peak_left,
        .peak_right = &master->peak_right,
        .smooth_peak_left = &master->smooth_peak_left,
        .smooth_peak_right = &master->smooth_peak_right
    };
}

static Uph_MixerStrip uph_mixer_strip_track(Uph_Track *track)
{
    return (Uph_MixerStrip){
        .ref = uph_action_track_ref(track),
        .track = track,
        .effects = &track->effects,
        .state = &track->state,
        .volume = &track->volume,
        .pan = &track->pan,
        .peak_left = &track->peak_left,
        .peak_right = &track->peak_right,
        .smooth_peak_left = &track->smooth_peak_left,
        .smooth_peak_right = &track->smooth_peak_right
    };
}

static Uph_MixerStrip uph_mixer_strip_resolve(Uph_ActionTrackRef ref)
{
    if (uph_action_track_ref_is_master(ref))
        return uph_mixer_strip_master();

    Uph_Track *track = uph_action_track_resolve(ref);
    if (!track)
        return (Uph_MixerStrip){ 0 };

    return uph_mixer_strip_track(track);
}

static Uph_MixerStrip uph_mixer_selected_strip(void)
{
    if (uph_state.shared.selected_mixer_master)
        return uph_mixer_strip_master();

    if (uph_state.shared.current_mixer_track)
        return uph_mixer_strip_track(uph_state.shared.current_mixer_track);

    return (Uph_MixerStrip){ 0 };
}

static bool uph_mixer_strip_valid(const Uph_MixerStrip *strip)
{
    return strip->effects != NULL;
}

static bool uph_mixer_strip_selected(const Uph_MixerStrip *strip)
{
    if (!strip->track)
        return uph_state.shared.selected_mixer_master;

    return !uph_state.shared.selected_mixer_master && uph_resources_mixer_track_selected(strip->track);
}

static void uph_mixer_strip_select(const Uph_MixerStrip *strip, const bool allow_multi)
{
    if (!strip->track)
    {
        uph_resources_select_mixer_master();
        return;
    }

    const bool ctrl = naui_key_down(NAUI_KEY_LCONTROL) || naui_key_down(NAUI_KEY_RCONTROL);
    const bool shift = naui_key_down(NAUI_KEY_LSHIFT) || naui_key_down(NAUI_KEY_RSHIFT);

    if (allow_multi && ctrl)
        uph_resources_toggle_mixer_track(strip->track);
    else if (allow_multi && shift)
        uph_resources_select_mixer_track_range(strip->track);
    else
        uph_resources_select_mixer_track(strip->track);
}

static int32_t uph_mixer_strip_color_index(const Uph_MixerStrip *strip)
{
    return strip->track ? strip->track->color_index : UPH_MIXER_MASTER_COLOR_INDEX;
}

static uint64_t uph_mixer_strip_id(const Uph_MixerStrip *strip)
{
    return strip->track ? (uint64_t)strip->track : (uint64_t)&uph_state.project.master_track;
}

#define UPH_MIXER_EFFECT_INSET      4.0f
#define UPH_MIXER_EFFECT_ROW_PAD    4.0f
#define UPH_MIXER_EFFECT_ROW_GAP    4.0f
#define UPH_MIXER_EFFECT_ROW_HEIGHT 28.0f
#define UPH_MIXER_EFFECT_LED_HIT    18.0f
#define UPH_MIXER_EFFECT_LED_DOT    8.0f

static uint32_t s_mixer_effect_context_index;

typedef struct Uph_MixerEffectArrowData
{
    Naui_Color color;
    bool up;
    bool visible;
}
Uph_MixerEffectArrowData;

static void uph_mixer_draw_effect_arrow(Leaf_BoundingBox bounding_box, Uph_MixerEffectArrowData *data)
{
    if (!data->visible)
        return;

    const float width = NAUI_DPI(8.0f);
    const float height = NAUI_DPI(5.0f);
    const float center_x = bounding_box.x + bounding_box.width * 0.5f;
    const float center_y = bounding_box.y + bounding_box.height * 0.5f;

    Naui_Vec2 points[3];
    if (data->up)
    {
        points[0] = (Naui_Vec2){ center_x,                   center_y - height * 0.5f };
        points[1] = (Naui_Vec2){ center_x + width * 0.5f,    center_y + height * 0.5f };
        points[2] = (Naui_Vec2){ center_x - width * 0.5f,    center_y + height * 0.5f };
    }
    else
    {
        points[0] = (Naui_Vec2){ center_x - width * 0.5f,    center_y - height * 0.5f };
        points[1] = (Naui_Vec2){ center_x + width * 0.5f,    center_y - height * 0.5f };
        points[2] = (Naui_Vec2){ center_x,                   center_y + height * 0.5f };
    }
    naui_fill_polygon(points, 3, data->color);
}

static bool uph_mixer_effect_move_button(const Leaf_ID id, const bool up, const bool available, const bool row_hovered)
{
    const bool interactive = available && row_hovered;
    const bool hovered = interactive && uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Naui_Color bg_color = { 0 };
    if (hovered)
    {
        bg_color = naui_mouse_down(NAUI_MOUSE_LEFT) ?
            naui_theme_color("uph_ui_frame_pressed_bg_color") :
            naui_theme_color("uph_ui_frame_hovered_bg_color");
    }

    Uph_MixerEffectArrowData arrow = {
        .color = naui_theme_color(hovered ? "uph_ui_text_color" : "uph_ui_text_disabled_color"),
        .up = up,
        .visible = interactive
    };

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(16.0f)), LEAF_SIZE_FIXED(NAUI_DPI(20.0f))},
        .color = {bg_color},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
        .custom_draw = (Leaf_CustomDrawFn)uph_mixer_draw_effect_arrow,
        .custom_draw_data = LEAF_DATA_SLICE(arrow)
    });

    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static bool uph_mixer_effect_power_toggle(bool *enabled, const Leaf_ID id)
{
    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Naui_Color dot_color;
    if (*enabled)
        dot_color = naui_theme_color("uph_ui_slider_fill_color");
    else
        dot_color = naui_theme_color(hovered ? "uph_ui_text_color" : "uph_ui_text_disabled_color");

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIXER_EFFECT_LED_HIT)), LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIXER_EFFECT_LED_HIT))},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIXER_EFFECT_LED_DOT)), LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIXER_EFFECT_LED_DOT))},
            .color = {dot_color},
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });
    }

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        *enabled = !(*enabled);
        return true;
    }
    return false;
}

static bool uph_mixer_effect_add_button(const char *label, const Leaf_ID id)
{
    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Naui_Color color = naui_theme_color("uph_ui_frame_bg_color");
    if (hovered)
    {
        color = naui_mouse_down(NAUI_MOUSE_LEFT) ?
            naui_theme_color("uph_ui_frame_pressed_bg_color") :
            naui_theme_color("uph_ui_frame_hovered_bg_color");
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIXER_EFFECT_ROW_HEIGHT))},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = {color},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL)
    })
    {
        leaf_text(label, {
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(naui_theme_float("uph_ui_font_size"))),
            .color = {naui_theme_color("uph_ui_text_color")}
        });
    }

    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void uph_mixer_render_effect_row(
    Uph_EffectPlugin *effect, const uint32_t index, const uint32_t count,
    Uph_UIMenuID context_menu, int32_t *move_from, int32_t *move_to)
{
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Leaf_ID id = leaf_id_indexed("uph_effect_list_item", index);

    const bool row_hovered =
        naui_panel_is_hovered(naui_panel_current()) &&
        !uph_ui_any_widget_hovered() &&
        leaf_hovered_clipped(id);

    if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_ui_widget_hovered(id))
    {
        s_mixer_effect_context_index = index;
        uph_ui_open_context_menu(context_menu);
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIXER_EFFECT_ROW_HEIGHT))},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_MIXER_EFFECT_ROW_PAD), 0),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .child_gap = NAUI_DPI(UPH_MIXER_EFFECT_ROW_GAP),
        .color = {naui_theme_color("uph_ui_frame_bg_color")},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL)
    })
    {
        uph_mixer_effect_power_toggle(&effect->enabled, leaf_id_indexed("uph_effect_list_enable", index));

        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .clip_children = true
        })
        {
            leaf_text(effect->plugin.name.data, {
                .font_size = LEAF_SIZE_FIXED(font_size),
                .color = {naui_theme_color(effect->enabled ? "uph_ui_text_color" : "uph_ui_text_disabled_color")}
            });
        }

        leaf({
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
            .direction = LEAF_DIRECTION_HORIZONTAL
        })
        {
            if (uph_mixer_effect_move_button(leaf_id_indexed("uph_effect_list_move_up", index), true, index > 0, row_hovered))
            {
                *move_from = (int32_t)index;
                *move_to = (int32_t)index - 1;
            }
            if (uph_mixer_effect_move_button(leaf_id_indexed("uph_effect_list_move_down", index), false, index + 1 < count, row_hovered))
            {
                *move_from = (int32_t)index;
                *move_to = (int32_t)index + 1;
            }
        }
    }
}

static void uph_mixer_render_effect_list(void)
{
    Uph_MixerStrip strip = uph_mixer_selected_strip();
    Uph_UIMenuID context_menu = uph_ui_context_menu();

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Naui_Color text_color = naui_theme_color("uph_ui_text_color");
    const Naui_Color text_disabled_color = naui_theme_color("uph_ui_text_disabled_color");
    const Naui_Color border_color = naui_theme_color("uph_ui_frame_border");
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const float rounding = NAUI_DPI(naui_theme_float("uph_ui_frame_rounding"));

    if (!uph_mixer_strip_valid(&strip))
    {
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .color = {naui_theme_color("uph_effect_list_bg_color")},
            .rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL),
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .clip_children = true
        })
        {
            leaf_text("No track selected", { .font_size = LEAF_SIZE_FIXED(font_size), .color = {text_disabled_color} });
        }
        return;
    }

    const uint32_t effect_count = (uint32_t)naui_list_len((*strip.effects));

    if (s_mixer_effect_context_index >= effect_count)
        s_mixer_effect_context_index = effect_count > 0 ? effect_count - 1 : 0;

    int32_t move_from = -1;
    int32_t move_to = -1;

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .color = {naui_theme_color("uph_effect_list_bg_color")},
        .rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL),
        .clip_children = true
    })
    {
        {
            const float dot_x = UPH_MIXER_EFFECT_INSET + UPH_MIXER_EFFECT_ROW_PAD + (UPH_MIXER_EFFECT_LED_HIT - UPH_MIXER_EFFECT_LED_DOT) * 0.5f;
            const float name_x = UPH_MIXER_EFFECT_INSET + UPH_MIXER_EFFECT_ROW_PAD + UPH_MIXER_EFFECT_LED_HIT + UPH_MIXER_EFFECT_ROW_GAP;

            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                .padding = {
                    .left = (int32_t)NAUI_DPI(dot_x),
                    .right = (int32_t)NAUI_DPI(padding.x),
                    .top = (int32_t)(NAUI_DPI(padding.y) * 1.5f),
                    .bottom = (int32_t)(NAUI_DPI(padding.y) * 1.5f)
                },
                .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .child_gap = NAUI_DPI(name_x - dot_x - UPH_MIXER_EFFECT_LED_DOT),
                .border = {
                    .color = {border_color},
                    .width = 1,
                    .sides = LEAF_SIDE_BOTTOM
                }
            })
            {
                leaf({
                    .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIXER_EFFECT_LED_DOT)), LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIXER_EFFECT_LED_DOT))},
                    .color = {uph_resources_track_color(uph_mixer_strip_color_index(&strip))},
                    .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
                });
                leaf_text(naui_string_format("%s Effects", strip.track ? strip.track->name.data : NAUI_TR("master.track.title")).data, { .font_size = LEAF_SIZE_FIXED(font_size), .color = {text_color} });
            }
        }

        if (effect_count == 0)
        {
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                leaf_text("No effects", { .font_size = LEAF_SIZE_FIXED(font_size), .color = {text_disabled_color} });
            }
        }
        else
        {
            static float scroll = 0.0f;
            Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(
                UPH_UI_SCROLL_DIRECTION_VERTICAL,
                &scroll,
                leaf_id("uph_effect_list_scrollbar")
            );
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                .padding = LEAF_PADDING_ALL(NAUI_DPI(UPH_MIXER_EFFECT_INSET)),
                .child_gap = NAUI_DPI(2)
            })
            {
                for (uint32_t i = 0; i < effect_count; i++)
                    uph_mixer_render_effect_row(&(*strip.effects)[i], i, effect_count, context_menu, &move_from, &move_to);
            }
            uph_ui_end_scroll_container(&scroll_container);
        }

        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
            .padding = LEAF_PADDING_ALL(NAUI_DPI(UPH_MIXER_EFFECT_INSET)),
            .border = {
                .color = {border_color},
                .width = 1,
                .sides = LEAF_SIDE_TOP
            }
        })
        {
            if (uph_mixer_effect_add_button("+ Add Effect", leaf_id("uph_effect_list_add_effect")))
            {
                if (strip.track)
                {
                    uph_state.shared.plugin_list_target = UPH_PLUGIN_LIST_TRACK_EFFECT;
                    uph_state.shared.current_plugin_list_track = strip.track;
                }
                else
                {
                    uph_state.shared.plugin_list_target = UPH_PLUGIN_LIST_MASTER_EFFECT;
                }
                naui_panel_open(uph_state.panels.plugin_list);
            }
        }
    }

    if (move_from >= 0 && move_to >= 0)
        uph_mixer_effect_list_swap(*strip.effects, (uint32_t)move_from, (uint32_t)move_to);

    if (effect_count > 0)
    {
        Uph_EffectPlugin *effect = &(*strip.effects)[s_mixer_effect_context_index];
        Uph_Plugin *plugin = &effect->plugin;

        const bool visible = uph_plugin_window_visible(plugin);
        if (uph_ui_menu_item(context_menu, visible ? NAUI_TR("effect_list.hide") : NAUI_TR("effect_list.show"), leaf_id("uph_effect_list_open")))
        {
            if (visible) uph_plugin_hide_window(plugin);
            else uph_plugin_show_window(plugin);
        }

        if (uph_ui_menu_item(context_menu, NAUI_TR("effect_list.remove"), leaf_id("uph_effect_list_remove")))
        {
            if (strip.track)
                uph_action_track_unload_plugin(strip.track, plugin);
            else
                uph_action_master_unload_plugin(plugin);
            naui_list_remove((*strip.effects), s_mixer_effect_context_index);
        }
    }
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

static const float uph_mixer_db_ticks[] = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f };
#define UPH_MIXER_DB_TICK_COUNT ((int)(sizeof(uph_mixer_db_ticks) / sizeof(uph_mixer_db_ticks[0])))

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

    for (int i = 0; i < UPH_MIXER_DB_TICK_COUNT; i++)
    {
        float y = bounding_box.y + (1.0f - uph_mixer_db_to_fraction(uph_mixer_db_ticks[i])) * bounding_box.height;

        char label[8];
        snprintf(label, sizeof(label), "%.0f", uph_mixer_db_ticks[i]);

        Naui_Vec2 text_size = naui_measure_text(label, (uint32_t)strlen(label), font_size, 0);
        float label_y = NAUI_CLAMP(y - text_size.y * 0.5f, bounding_box.y, bounding_box.y + bounding_box.height - text_size.y);

        naui_draw_text(
            (Naui_Vec2){bounding_box.x + tick_length - label_gap - text_size.x + bounding_box.width, label_y},
            label, font_size, 0, label_color
        );
    }
}

static void uph_mixer_draw_peak_bars(Leaf_BoundingBox bounding_box, Uph_ActionTrackRef *data)
{
    Uph_MixerStrip strip = uph_mixer_strip_resolve(*data);
    if (!uph_mixer_strip_valid(&strip))
        return;

    const float gap = NAUI_DPI(1.0f);

    const float smooth_speed = naui_delta_time() * 30.0f;
    *strip.smooth_peak_left = NAUI_LERP(*strip.smooth_peak_left, *strip.peak_left, smooth_speed);
    *strip.smooth_peak_right = NAUI_LERP(*strip.smooth_peak_right, *strip.peak_right, smooth_speed);

    float left_fraction = uph_mixer_db_to_fraction(uph_linear_to_db(*strip.smooth_peak_left));
    float right_fraction = uph_mixer_db_to_fraction(uph_linear_to_db(*strip.smooth_peak_right));

    const Naui_Color bar_min_color = naui_theme_color("uph_mixer_bar_min_color");
    const Naui_Color bar_max_color = naui_theme_color("uph_mixer_bar_max_color");

    const float half_width = bounding_box.width * 0.5f;
    naui_push_clip_rect(
        bounding_box.x, bounding_box.y + (1.0f - left_fraction) * bounding_box.height,
        half_width - gap, bounding_box.height * left_fraction
    );
    naui_fill_gradient_rect(
        (Naui_Vec2){bounding_box.x, bounding_box.y},
        (Naui_Vec2){half_width, bounding_box.height},
        (Naui_Gradient){.angle = 90 * NAUI_DEG2RAD, .color1 = bar_max_color, .color2 = bar_min_color, .percent1 = 0.0f, .percent2 = 1.0f},
        0.0f,
        NAUI_CORNER_NONE
    );
    naui_pop_clip_rect();

    naui_push_clip_rect(
        bounding_box.x + half_width + gap, bounding_box.y + (1.0f - right_fraction) * bounding_box.height,
        half_width - gap, bounding_box.height * right_fraction
    );
    naui_fill_gradient_rect(
        (Naui_Vec2){bounding_box.x + half_width, bounding_box.y},
        (Naui_Vec2){half_width, bounding_box.height},
        (Naui_Gradient){.angle = 90 * NAUI_DEG2RAD, .color1 = bar_max_color, .color2 = bar_min_color, .percent1 = 0.0f, .percent2 = 1.0f},
        0.0f,
        NAUI_CORNER_NONE
    );
    naui_pop_clip_rect();
}

static struct
{
    Uph_ActionTrackRef track;
    Uph_ActionTrackValueKind kind;
    float old_value;
    bool active;
} _Uph_MixerEdit;

typedef struct Uph_MixerVolumeArrowData
{
    Uph_ActionTrackRef track;
    uint64_t id;
} Uph_MixerVolumeArrowData;

static void uph_mixer_commit_edit(void)
{
    if (!_Uph_MixerEdit.active)
        return;

    _Uph_MixerEdit.active = false;
    Uph_MixerStrip strip = uph_mixer_strip_resolve(_Uph_MixerEdit.track);
    if (!uph_mixer_strip_valid(&strip))
        return;

    Uph_ActionTrackValue data = {
        .track = _Uph_MixerEdit.track,
        .kind = _Uph_MixerEdit.kind,
        .old_value = _Uph_MixerEdit.old_value,
        .new_value = _Uph_MixerEdit.kind == UPH_ACTION_TRACK_VALUE_PAN ? *strip.pan : *strip.volume
    };

    if (data.old_value != data.new_value)
        naui_action_execute_stack(_Uph_MixerEdit.kind == UPH_ACTION_TRACK_VALUE_PAN ? UPH_ACTION_TRACK_PAN : UPH_ACTION_TRACK_VOLUME, data);
}

static void uph_mixer_edit_value(Uph_ActionTrackRef ref, Uph_ActionTrackValueKind kind, float before)
{
    if (_Uph_MixerEdit.active && (_Uph_MixerEdit.kind != kind || _Uph_MixerEdit.track.parent != ref.parent || _Uph_MixerEdit.track.index != ref.index))
        uph_mixer_commit_edit();

    if (!_Uph_MixerEdit.active)
    {
        _Uph_MixerEdit.track = ref;
        _Uph_MixerEdit.kind = kind;
        _Uph_MixerEdit.old_value = before;
        _Uph_MixerEdit.active = true;
    }
}

static void uph_mixer_draw_volume_arrow(Leaf_BoundingBox bounding_box, Uph_MixerVolumeArrowData *data)
{
    Uph_MixerStrip strip = uph_mixer_strip_resolve(data->track);
    if (!uph_mixer_strip_valid(&strip))
        return;

    const float arrow_width = NAUI_DPI(10.0f);
    const float arrow_height = NAUI_DPI(12.0f);
    const float hit_padding = NAUI_DPI(4.0f);

    float db = uph_linear_to_db(*strip.volume);
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
        naui_panel_is_hovered(uph_state.panels.mixer) &&
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
            uph_mixer_edit_value(strip.ref, UPH_ACTION_TRACK_VALUE_VOLUME, *strip.volume);
            *strip.volume = uph_db_to_linear(new_db);

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

#define UPH_MIXER_DRAG_THRESHOLD 6.0f

typedef struct Uph_MixerDrag
{
    Uph_ActionTrackRef source;
    Naui_Vec2 press_position;
    bool pending;
    bool active;
    bool select_on_release;
}
Uph_MixerDrag;

static Uph_MixerDrag s_mixer_drag;

static void uph_mixer_drag_begin(Uph_ActionTrackRef source, bool select_on_release)
{
    s_mixer_drag = (Uph_MixerDrag){
        .source = source,
        .press_position = { (float)naui_mouse_x(), (float)naui_mouse_y() },
        .pending = true,
        .select_on_release = select_on_release
    };
}

// Pressing the drag handle of a strip that is part of a multi-selection must leave the selection alone, otherwise
// there would be nothing left to drag. (If it turns out to be a plain click, the drag code collapses it on release.)
static bool uph_mixer_press_keeps_selection(const Uph_MixerStrip *strip, const Leaf_ID handle_id)
{
    if (!strip->track || !uph_mixer_strip_selected(strip))
        return false;

    if (naui_list_len(uph_state.shared.selected_mixer_tracks) < 2)
        return false;

    const bool ctrl = naui_key_down(NAUI_KEY_LCONTROL) || naui_key_down(NAUI_KEY_RCONTROL);
    const bool shift = naui_key_down(NAUI_KEY_LSHIFT) || naui_key_down(NAUI_KEY_RSHIFT);
    if (ctrl || shift)
        return false;

    return leaf_hovered(handle_id);
}

static void uph_mixer_render_strip(Uph_MixerStrip strip, Uph_UIMenuID context_menu)
{
    const uint64_t track_id = uph_mixer_strip_id(&strip);
    const Naui_Color accent_color = uph_resources_track_color(uph_mixer_strip_color_index(&strip));
    Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    const bool is_master = (strip.track == NULL);

    Leaf_ID id = leaf_id_indexed("uph_mixer_track", track_id);
    const Leaf_ID handle_id = leaf_id_indexed("uph_mixer_drag_handle", track_id);
    bool selection_kept_for_drag = false;
    if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && naui_panel_is_hovered(naui_panel_current()) && leaf_hovered_clipped(id))
    {
        selection_kept_for_drag = uph_mixer_press_keeps_selection(&strip, handle_id);
        if (!selection_kept_for_drag)
            uph_mixer_strip_select(&strip, true);
    }
    else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && naui_panel_is_hovered(naui_panel_current()) && leaf_hovered_clipped(id))
    {
        if (!uph_mixer_strip_selected(&strip))
            uph_mixer_strip_select(&strip, false);
        uph_ui_open_context_menu(context_menu);
    }

    const Leaf_Color bg_color = uph_mixer_strip_selected(&strip) ?
        naui_theme_color("uph_mixer_track_selected_bg_color") :
        naui_theme_color("uph_mixer_track_bg_color");

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(100 + padding.x * 2)), LEAF_SIZE_FULL},
        .color = (strip.track && strip.track->type == UPH_RESOURCE_GROUP) ?
        LEAF_GRADIENT_PERCENT(
            accent_color,
            -0.5f,
            bg_color,
            0.5f,
            LEAF_DEG(90.0f)
        ) : (Leaf_ColorFill){bg_color}
    })
    {
        if (strip.track && strip.track->type == UPH_RESOURCE_GROUP)
        {
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(8))},
                .color = accent_color
            }); 
        }
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 2)),
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP},
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
            if (!is_master)
            {
                if (naui_panel_is_hovered(naui_panel_current()) && leaf_hovered_clipped(handle_id))
                {
                    naui_set_cursor(NAUI_CURSOR_HAND);
                    if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
                        uph_mixer_drag_begin(strip.ref, selection_kept_for_drag);
                }

                leaf({
                    .id = handle_id,
                    .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                    .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP},
                    .child_gap = NAUI_DPI(8)
                })
                {
                    if (strip.track->type != UPH_RESOURCE_GROUP)
                    {
                        leaf({
                            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(6))},
                            .color = accent_color,
                            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
                        });
                    }
                    leaf_text(strip.track->name.data, { .font_size = font_size, .color = text_color });
                }
            }
            else
            {
                leaf_text(NAUI_TR("master.track.title"), { .font_size = font_size, .color = text_color });
            }

            const float old_pan = *strip.pan;
            leaf({.size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_DERIVED}, .aspect_ratio = 1.0f})
            if (uph_ui_knob_float(strip.pan, leaf_id_indexed("uph_mixer_pan_knob", track_id), -1.0f, 1.0f, 0.0f, accent_color, UPH_UI_KNOB_START_TYPE_CENTER))
                uph_mixer_edit_value(strip.ref, UPH_ACTION_TRACK_VALUE_PAN, old_pan);

            leaf({.size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_FIT}})
            {
                if (uph_ui_drag_float(strip.pan, leaf_id_indexed("uph_mixer_pan", track_id), 0.01f, -1.0f, 1.0f, "%.2f", UPH_UI_DRAG_FLAG_CLAMPED))
                    uph_mixer_edit_value(strip.ref, UPH_ACTION_TRACK_VALUE_PAN, old_pan);

                if (!naui_mouse_down(NAUI_MOUSE_LEFT))
                    uph_mixer_commit_edit();
            }

            const int32_t button_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
            leaf({
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .child_gap = NAUI_DPI(2)
            })
            {
                if (uph_ui_text_toggle_button("M", leaf_id_indexed("uph_mixer_mute_toggle", track_id), *strip.state & UPH_TRACK_MUTED))
                {
                    if (!is_master)
                        uph_resources_toggle_track_state(strip.track, UPH_TRACK_MUTED, UPH_ACTION_TRACK_MUTE);
                    else
                        uph_resources_toggle_master_state(UPH_TRACK_MUTED, UPH_ACTION_TRACK_MUTE);
                }

                if (!is_master && uph_ui_text_toggle_button("S", leaf_id_indexed("uph_mixer_solo_toggle", track_id), uph_state.project.soloed_track == strip.track))
                {
                    Uph_ActionTrackSolo data = { .track = strip.ref };
                    naui_action_execute_stack(UPH_ACTION_TRACK_SOLO, data);
                }
            }

            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .child_gap = NAUI_DPI(8)
            })
            {
                leaf({
                    .size = {LEAF_SIZE_PERCENT(0.2f), LEAF_SIZE_FULL},
                    .custom_draw = (Leaf_CustomDrawFn)uph_mixer_draw_db_ruler
                });
                Uph_ActionTrackRef track_ref = strip.ref;
                leaf({
                    .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
                    .color = {naui_theme_color("uph_ui_frame_bg_color")},
                    .custom_draw = (Leaf_CustomDrawFn)uph_mixer_draw_peak_bars,
                    .custom_draw_data = LEAF_DATA_SLICE(track_ref)
                });
                Uph_MixerVolumeArrowData arrow_data = {
                    .track = track_ref,
                    .id = track_id
                };
                leaf({
                    .size = {LEAF_SIZE_PERCENT(0.2f), LEAF_SIZE_FULL},
                    .custom_draw = (Leaf_CustomDrawFn)uph_mixer_draw_volume_arrow,
                    .custom_draw_data = LEAF_DATA_SLICE(arrow_data)
                });
            }

            float volume_db = uph_linear_to_db(*strip.volume);

            leaf({.size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_FIT}})
            {
                if (uph_ui_drag_float(&volume_db, leaf_id_indexed("uph_mixer_volume", track_id), 0.01f, UPH_MIXER_DB_MIN, UPH_MIXER_DB_MAX, "%.2f", UPH_UI_DRAG_FLAG_CLAMPED))
                {
                    uph_mixer_edit_value(strip.ref, UPH_ACTION_TRACK_VALUE_VOLUME, *strip.volume);
                    *strip.volume = uph_db_to_linear(volume_db);
                }

                if (!naui_mouse_down(NAUI_MOUSE_LEFT))
                    uph_mixer_commit_edit();
            }

            if (!is_master && strip.track->type != UPH_RESOURCE_GROUP)
            {
                leaf({
                    .direction = LEAF_DIRECTION_HORIZONTAL,
                    .child_gap = NAUI_DPI(2)
                })
                {
                    if (uph_ui_image_toggle_button(
                        naui_asset_image("uph_icon_mic"),
                        leaf_id_indexed("uph_mixer_arm_toggle", track_id),
                        (Naui_Vec2) { button_size, button_size },
                        text_color,
                        *strip.state & UPH_TRACK_ARMED
                    )) uph_resources_toggle_track_state(strip.track, UPH_TRACK_ARMED, UPH_ACTION_TRACK_ARM);
                    if (uph_ui_image_toggle_button(
                        naui_asset_image("uph_icon_headphones"),
                        leaf_id_indexed("uph_mixer_monitor_toggle", track_id),
                        (Naui_Vec2) { button_size, button_size },
                        text_color,
                        strip.track->monitored
                    ))
                    {
                        strip.track->monitored = !strip.track->monitored;

                        if (strip.track->type == UPH_RESOURCE_PATTERN
                            && !strip.track->monitored
                            && strip.track->instrument.loaded)
                            uph_plugin_queue_stop_all(&strip.track->instrument, 0);
                    }
                }
            }
        }
    }

    if (!strip.track || strip.track->type != UPH_RESOURCE_GROUP)
        return;

    leaf({
        .size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(8))},
            .color = uph_resources_track_color(strip.track->color_index)
        });
        leaf({
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_GROW},
            .direction = LEAF_DIRECTION_HORIZONTAL
        })
        {
            for (uint32_t i = 0; i < naui_list_len(strip.track->subtracks); i++)
            {
                Uph_Track *track = &strip.track->subtracks[i];
                uph_mixer_render_strip(uph_mixer_strip_track(track), context_menu);
            }
        }
    }
}

static int uph_mixer_remove_order(const void *a, const void *b)
{
    const Uph_ActionTrackRef *ra = a;
    const Uph_ActionTrackRef *rb = b;

    const bool child_a = ra->parent != UPH_ACTION_TRACK_TOP;
    const bool child_b = rb->parent != UPH_ACTION_TRACK_TOP;
    if (child_a != child_b)
        return child_a ? -1 : 1;
    if (ra->parent != rb->parent)
        return ra->parent > rb->parent ? -1 : 1;
    if (ra->index != rb->index)
        return ra->index > rb->index ? -1 : 1;
    return 0;
}

static void uph_mixer_remove_selected_tracks(void)
{
    const uint32_t count = (uint32_t)naui_list_len(uph_state.shared.selected_mixer_tracks);
    if (count == 0)
        return;

    Uph_ActionTrackRef *refs = malloc(sizeof(Uph_ActionTrackRef) * count);
    if (!refs)
        return;

    uint32_t ref_count = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        const Uph_ActionTrackRef ref = uph_action_track_ref(uph_state.shared.selected_mixer_tracks[i]);
        if (ref.parent != UPH_ACTION_TRACK_INVALID)
            refs[ref_count++] = ref;
    }

    qsort(refs, ref_count, sizeof(Uph_ActionTrackRef), uph_mixer_remove_order);

    // Removing several tracks is a single undo step. (A lone track keeps its own "Track Delete" entry.)
    const bool grouped = ref_count > 1;
    if (grouped)
        naui_action_group_start(UPH_ACTION_TRACKS_DELETE_GROUP);

    for (uint32_t i = 0; i < ref_count; i++)
    {
        Uph_ActionTrackDelete data = { .track = refs[i] };
        naui_action_execute_stack(UPH_ACTION_TRACK_DELETE, data);
    }

    if (grouped)
        naui_action_group_end();

    free(refs);
}

typedef struct Uph_MixerDropTarget
{
    int32_t parent;
    uint32_t index;
    float line_x, line_y, line_height;
    bool valid;
}
Uph_MixerDropTarget;

static bool uph_mixer_strip_box(const Uph_Track *track, Leaf_BoundingBox *out)
{
    *out = leaf_get_bounding_box(leaf_id_indexed("uph_mixer_track", (uint64_t)track));
    return out->width > 0.0f;
}

static bool uph_mixer_drop_on_strip(Uph_MixerDropTarget *target, float mouse_x, Leaf_BoundingBox box, int32_t parent, uint32_t index)
{
    if (mouse_x >= box.x + box.width)
        return false;

    const bool after = mouse_x >= box.x + box.width * 0.5f;
    *target = (Uph_MixerDropTarget){
        .parent = parent,
        .index = after ? index + 1 : index,
        .line_x = after ? box.x + box.width : box.x,
        .line_y = box.y,
        .line_height = box.height,
        .valid = true
    };
    return true;
}

static Uph_MixerDropTarget uph_mixer_find_drop_target(float mouse_x, bool dragging_group)
{
    Uph_MixerDropTarget target = { 0 };
    Naui_List(Uph_Track) tracks = uph_state.project.tracks;
    const uint32_t count = (uint32_t)naui_list_len(tracks);

    Leaf_BoundingBox last_box = { 0 };
    bool have_last = false;

    for (uint32_t i = 0; i < count; i++)
    {
        Leaf_BoundingBox box;
        if (!uph_mixer_strip_box(&tracks[i], &box))
            continue;

        have_last = true;
        last_box = box;

        if (tracks[i].type != UPH_RESOURCE_GROUP)
        {
            if (uph_mixer_drop_on_strip(&target, mouse_x, box, UPH_ACTION_TRACK_TOP, i))
                return target;
            continue;
        }

        if (dragging_group)
        {
            float block_right = box.x + box.width;
            const uint32_t block_children = (uint32_t)naui_list_len(tracks[i].subtracks);
            for (uint32_t s = 0; s < block_children; s++)
            {
                Leaf_BoundingBox child_box;
                if (!uph_mixer_strip_box(&tracks[i].subtracks[s], &child_box))
                    continue;

                last_box = child_box;
                if (child_box.x + child_box.width > block_right)
                    block_right = child_box.x + child_box.width;
            }

            if (mouse_x < block_right)
            {
                const bool after = mouse_x >= box.x + (block_right - box.x) * 0.5f;
                return (Uph_MixerDropTarget){
                    .parent = UPH_ACTION_TRACK_TOP,
                    .index = after ? i + 1 : i,
                    .line_x = after ? block_right : box.x,
                    .line_y = box.y,
                    .line_height = box.height,
                    .valid = true
                };
            }
            continue;
        }

        if (uph_mixer_drop_on_strip(&target, mouse_x, box, UPH_ACTION_TRACK_TOP, i))
        {
            if (target.index > i)
            {
                target.parent = (int32_t)i;
                target.index = 0;
            }
            return target;
        }

        const uint32_t child_count = (uint32_t)naui_list_len(tracks[i].subtracks);
        for (uint32_t s = 0; s < child_count; s++)
        {
            Leaf_BoundingBox child_box;
            if (!uph_mixer_strip_box(&tracks[i].subtracks[s], &child_box))
                continue;

            last_box = child_box;
            if (uph_mixer_drop_on_strip(&target, mouse_x, child_box, (int32_t)i, s))
                return target;
        }
    }

    if (have_last)
    {
        target = (Uph_MixerDropTarget){
            .parent = UPH_ACTION_TRACK_TOP,
            .index = count,
            .line_x = last_box.x + last_box.width,
            .line_y = last_box.y,
            .line_height = last_box.height,
            .valid = true
        };
    }
    return target;
}

// What the current drag carries: the whole selection when the grabbed strip is part of it, otherwise just that strip.
// The caller frees the list.
static Naui_List(Uph_TrackPtr) uph_mixer_drag_tracks(Uph_Track *source)
{
    Naui_List(Uph_TrackPtr) tracks = NULL;

    if (uph_resources_mixer_track_selected(source))
    {
        for (uint32_t i = 0; i < naui_list_len(uph_state.shared.selected_mixer_tracks); i++)
            naui_list_push(tracks, uph_state.shared.selected_mixer_tracks[i]);
    }
    else
    {
        naui_list_push(tracks, source);
    }

    return tracks;
}

static void uph_mixer_update_drag(void)
{
    if (!s_mixer_drag.pending && !s_mixer_drag.active)
        return;

    Uph_Track *source = uph_action_track_resolve(s_mixer_drag.source);
    if (!source || naui_key_pressed(NAUI_KEY_ESCAPE))
    {
        s_mixer_drag = (Uph_MixerDrag){ 0 };
        return;
    }

    const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };
    const bool mouse_down = naui_mouse_down(NAUI_MOUSE_LEFT);

    if (s_mixer_drag.pending)
    {
        if (!mouse_down)
        {
            if (s_mixer_drag.select_on_release)
                uph_resources_select_mixer_track(source);

            s_mixer_drag = (Uph_MixerDrag){ 0 };
            return;
        }

        const float dx = mouse.x - s_mixer_drag.press_position.x;
        const float dy = mouse.y - s_mixer_drag.press_position.y;
        const float threshold = NAUI_DPI(UPH_MIXER_DRAG_THRESHOLD);
        if (dx * dx + dy * dy < threshold * threshold)
            return;

        s_mixer_drag.pending = false;
        s_mixer_drag.active = true;
    }

    Naui_List(Uph_TrackPtr) dragged = uph_mixer_drag_tracks(source);

    // Groups only fit between top-level strips, so one group in the drag restricts where everything can be dropped.
    bool dragging_group = false;
    for (uint32_t i = 0; i < naui_list_len(dragged); i++)
        dragging_group = dragging_group || dragged[i]->type == UPH_RESOURCE_GROUP;

    const Uph_MixerDropTarget target = naui_panel_is_hovered(uph_state.panels.mixer) ?
        uph_mixer_find_drop_target(mouse.x, dragging_group) : (Uph_MixerDropTarget){ 0 };

    if (!mouse_down)
    {
        if (target.valid)
        {
            Uph_Track *dest_group = target.parent == UPH_ACTION_TRACK_TOP ? NULL : &uph_state.project.tracks[target.parent];
            uph_resources_move_tracks(dragged, dest_group, target.index);
        }
        naui_list_free(dragged);
        s_mixer_drag = (Uph_MixerDrag){ 0 };
        return;
    }

    const uint32_t carried = uph_resources_count_movable_tracks(dragged);
    naui_list_free(dragged);

    char label[256];
    if (carried > 1)
        snprintf(label, sizeof(label), "%s +%u", source->name.data, carried - 1);
    else
        snprintf(label, sizeof(label), "%s", source->name.data);

    naui_set_cursor(NAUI_CURSOR_HAND);

    if (target.valid)
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(3.0f)), LEAF_SIZE_FIXED(target.line_height)},
            .color = {naui_theme_color("uph_ui_slider_fill_color")},
            .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
            .floating = { .offset = { target.line_x - NAUI_DPI(1.5f), target.line_y } }
        });
    }

    leaf({
        .padding = LEAF_PADDING_AXES(NAUI_DPI(8), NAUI_DPI(4)),
        .color = {naui_theme_color("uph_ui_dropdown_bg_color")},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
        .floating = { .offset = { mouse.x + NAUI_DPI(12), mouse.y + NAUI_DPI(12) } }
    })
    {
        leaf_text(label, {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = {naui_theme_color("uph_ui_text_color")}
        });
    }
}

static void uph_mixer_on_update(void)
{
	naui_panel_set_title(naui_panel_current(), NAUI_TR("mixer.title"));
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        static float scroll = 0.0f;
        Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(
            UPH_UI_SCROLL_DIRECTION_HORIZONTAL,
            &scroll,
            leaf_id("uph_mixer_scrollbar")
        );
        Uph_UIMenuID context_menu = uph_ui_context_menu();
        uph_mixer_render_strip(uph_mixer_strip_master(), context_menu);
        for (uint32_t i = 0; i < naui_list_len(uph_state.project.tracks); i++)
        {
            Uph_Track *track = &uph_state.project.tracks[i];
            uph_mixer_render_strip(uph_mixer_strip_track(track), context_menu);
        }
        uph_mixer_update_drag();
        uph_ui_end_scroll_container(&scroll_container);

        if (!uph_state.shared.selected_mixer_master && uph_state.shared.current_mixer_track)
        {
            Uph_Track *current = uph_state.shared.current_mixer_track;

            bool restructured = false;

            if (uph_resources_count_groupable_tracks(uph_state.shared.selected_mixer_tracks) >= 2 &&
                uph_ui_menu_item(context_menu, "Group Tracks", leaf_id("uph_mixer_group_tracks")))
            {
                uph_resources_group_tracks(uph_state.shared.selected_mixer_tracks, "Untitled Group", current->color_index);
                restructured = true;
            }

            if (!restructured && current->type == UPH_RESOURCE_GROUP && !current->parent &&
                uph_ui_menu_item(context_menu, "Ungroup", leaf_id("uph_mixer_ungroup_tracks")))
            {
                uph_resources_ungroup_track(current);
                restructured = true;
            }

            if (!restructured && uph_ui_menu_item(context_menu, "Remove", leaf_id("uph_mixer_remove_track")))
                uph_mixer_remove_selected_tracks();
        }

        Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
        leaf({
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(200)), LEAF_SIZE_FULL}
        })
        {
            uph_mixer_render_effect_list();
        }
    }
}