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

static void uph_mixer_effect_list_swap(Uph_Track *track, uint32_t a, uint32_t b)
{
    Uph_EffectPlugin temp = track->effects[a];
    track->effects[a] = track->effects[b];
    track->effects[b] = temp;
}

static void uph_mixer_render_effect_list(Uph_UIMenuID context_menu)
{
    Uph_Track *track = uph_state.shared.selected_mixer_track;

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Naui_Color text_color = naui_theme_color("uph_ui_text_color");
    const Naui_Color text_disabled_color = naui_theme_color("uph_ui_text_disabled_color");
    const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));

    leaf({
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .color = naui_theme_color("uph_effect_list_bg_color")
    })
    {
        leaf_text("Track Effects", { .font_size = font_size, .color = text_color });
        if (uph_ui_text_button("Add Effect", leaf_id("uph_effect_list_add_effect")))
        {
            uph_state.shared.plugin_list_for_track_instrument = false;
            uph_state.shared.current_plugin_list_track = track;
            naui_open_panel(uph_state.panels.plugin_list);
        }

        const uint32_t effect_count = (uint32_t)naui_list_len(track->effects);
        if (effect_count == 0)
            continue;

        static uint32_t current_effect_index;

        int32_t move_from = -1;
        int32_t move_to = -1;
        for (uint32_t i = 0; i < effect_count; i++)
        {
            Uph_EffectPlugin *effect = &track->effects[i];
            Uph_Plugin *plugin = &effect->plugin;
            
            Leaf_ID id = leaf_id_indexed("uph_effect_list_item", i);
            
            if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_ui_widget_hovered(id))
            {
                current_effect_index = i;
                uph_ui_open_context_menu(context_menu);
            }

            leaf({
                .id = id,
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32))},
                .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
                .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                .direction = LEAF_DIRECTION_HORIZONTAL
            })
            {
                leaf({
                    .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
                    .direction = LEAF_DIRECTION_HORIZONTAL,
                    .child_gap = NAUI_DPI(6)
                })
                {
                    uph_ui_checkbox(&effect->enabled, leaf_id_indexed("uph_effect_list_enable", i));
                    leaf_text(plugin->name.data, { .font_size = font_size, .color = effect->enabled ? text_color : text_disabled_color });
                }
                leaf({
                    .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
                    .direction = LEAF_DIRECTION_HORIZONTAL,
                    .child_gap = NAUI_DPI(2)
                })
                {
                    if (i > 0 && uph_ui_text_button("^", leaf_id_indexed("uph_effect_list_move_up", i)))
                    {
                        move_from = (int32_t)i;
                        move_to = (int32_t)i - 1;
                    }
                    if (i + 1 < effect_count && uph_ui_text_button("v", leaf_id_indexed("uph_effect_list_move_down", i)))
                    {
                        move_from = (int32_t)i;
                        move_to = (int32_t)i + 1;
                    }
                }
            }
        }

        if (move_from >= 0 && move_to >= 0)
            uph_mixer_effect_list_swap(track, (uint32_t)move_from, (uint32_t)move_to);

        {
            Uph_EffectPlugin *effect = &track->effects[current_effect_index];
            Uph_Plugin *plugin = &effect->plugin;
            
            const bool visible = uph_plugin_window_visible(plugin);
            if (uph_ui_menu_item(context_menu, visible ? NAUI_TR("effect_list.hide") : NAUI_TR("effect_list.show"), leaf_id("uph_effect_list_open")))
            {
                if (visible) uph_hide_plugin_window(plugin);
                else uph_show_plugin_window(plugin);
            }

            if (uph_ui_menu_item(context_menu, NAUI_TR("effect_list.remove"), leaf_id("uph_effect_list_remove")))
            {
                uph_action_track_unload_plugin(track, plugin);
                naui_list_remove(track->effects, current_effect_index);
            }
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

static void uph_mixer_draw_peak_bars(Leaf_BoundingBox bounding_box, Uph_Track **data)
{
    Uph_Track *track = *data;
    const float gap = NAUI_DPI(1.0f);

    const float smooth_speed = naui_delta_time() * 30.0f;
    track->smooth_peak_left = NAUI_LERP(track->smooth_peak_left, track->peak_left, smooth_speed);
    track->smooth_peak_right = NAUI_LERP(track->smooth_peak_right, track->peak_right, smooth_speed);

    float left_fraction = uph_mixer_db_to_fraction(uph_linear_to_db(track->smooth_peak_left));
    float right_fraction = uph_mixer_db_to_fraction(uph_linear_to_db(track->smooth_peak_right));

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
    Uph_Track *track;
    uint64_t id;
} Uph_MixerVolumeArrowData;

static void uph_mixer_commit_edit(void)
{
    if (!_Uph_MixerEdit.active)
        return;

    _Uph_MixerEdit.active = false;
    Uph_Track *track = uph_action_track_resolve(_Uph_MixerEdit.track);
    if (!track)
        return;

    Uph_ActionTrackValue data = {
        .track = _Uph_MixerEdit.track,
        .kind = _Uph_MixerEdit.kind,
        .old_value = _Uph_MixerEdit.old_value,
        .new_value = _Uph_MixerEdit.kind == UPH_ACTION_TRACK_VALUE_PAN ? track->pan : track->volume
    };

    if (data.old_value != data.new_value)
        naui_action_execute_stack(_Uph_MixerEdit.kind == UPH_ACTION_TRACK_VALUE_PAN ? UPH_ACTION_TRACK_PAN : UPH_ACTION_TRACK_VOLUME, data);
}

static void uph_mixer_edit_value(Uph_Track *track, Uph_ActionTrackValueKind kind, float before)
{
    const Uph_ActionTrackRef ref = uph_action_track_ref(track);

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
			uph_mixer_edit_value(track, UPH_ACTION_TRACK_VALUE_VOLUME, track->volume);
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
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 2)),
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

        uph_ui_knob_float(&track->pan, leaf_id_indexed("uph_mixer_pan_knob", track_id), -1.0f, 1.0f, 0.0f, NAUI_DPI(48.0f), uph_resources_track_color(track->color_index));
        leaf_text("Pan", { .font_size = font_size, .color = text_color });

        leaf({.size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_FIT}})
		{
			const float old_pan = track->pan;
            if (uph_ui_drag_float(&track->pan, leaf_id_indexed("uph_mixer_pan", track_id), 0.01f, -1.0f, 1.0f, "%.2f", UPH_UI_DRAG_CLAMPED))
				uph_mixer_edit_value(track, UPH_ACTION_TRACK_VALUE_PAN, old_pan);

			if (!naui_mouse_down(NAUI_MOUSE_LEFT))
        		uph_mixer_commit_edit();
		}

        const int32_t button_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .child_gap = NAUI_DPI(2)
        })
        {
            if (uph_ui_text_toggle_button("M", leaf_id_indexed("uph_mixer_mute_toggle", track_id), track->state & UPH_TRACK_MUTED));
            if (uph_ui_text_toggle_button("S", leaf_id_indexed("uph_mixer_solo_toggle", track_id), uph_state.project.soloed_track == track));
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
                .size = {LEAF_SIZE_PERCENT(0.2f), LEAF_SIZE_FULL},
                .custom_draw = (Leaf_CustomDrawFn)uph_mixer_draw_volume_arrow,
                .custom_draw_data = LEAF_DATA_SLICE(arrow_data)
            });
        }

        leaf_text("Volume", { .font_size = font_size, .color = text_color });

        float volume_db = uph_linear_to_db(track->volume);

        leaf({.size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_FIT}})
		{
            if (uph_ui_drag_float(&volume_db, leaf_id_indexed("uph_mixer_volume", track_id), 0.01f, UPH_MIXER_DB_MIN, UPH_MIXER_DB_MAX, "%.2f", UPH_UI_DRAG_CLAMPED))
			{
				uph_mixer_edit_value(track, UPH_ACTION_TRACK_VALUE_VOLUME, track->volume);
                track->volume = uph_db_to_linear(volume_db);
			}

			if (!naui_mouse_down(NAUI_MOUSE_LEFT))
        		uph_mixer_commit_edit();
		}

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
                track->state & UPH_TRACK_ARMED
            ));
            if (uph_ui_image_toggle_button(
                naui_asset_image("uph_icon_headphones"),
                leaf_id_indexed("uph_mixer_monitor_toggle", track_id),
                (Naui_Vec2) { button_size, button_size },
                text_color,
                track->monitored
            )) track->monitored = !track->monitored;
        }
    }
}

static void uph_mixer_on_update(void)
{
	naui_panel_set_title(naui_current_panel(), NAUI_TR("mixer.title"));
    Uph_UIMenuID effect_list_context_menu = uph_ui_context_menu();

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
        for (uint32_t i = 0; i < naui_list_len(uph_state.project.tracks); i++)
        {
            Uph_Track *track = &uph_state.project.tracks[i];
            uph_mixer_render_track(track);
        }
        uph_ui_end_scroll_container(&scroll_container);

        if (!uph_state.shared.selected_mixer_track)
            continue;
        Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
        leaf({
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(200)), LEAF_SIZE_FULL}
        })
        {
            uph_mixer_render_effect_list(effect_list_context_menu);
        }
    }
}
