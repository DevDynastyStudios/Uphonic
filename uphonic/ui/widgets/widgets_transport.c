typedef struct
{
    Leaf_Color color;
    bool up;
}
Uph_UITriangleDrawData;

static void uph__ui_triangle_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_UITriangleDrawData *data = (const Uph_UITriangleDrawData*)user_data;

    const float width = NAUI_MIN(box.width, box.height * 1.6f) * 0.6f;
    const float height = width * 0.55f;
    const float center_x = box.x + box.width * 0.5f;
    const float center_y = box.y + box.height * 0.5f;
    const float tip_y = data->up ? center_y - height * 0.5f : center_y + height * 0.5f;
    const float base_y = data->up ? center_y + height * 0.5f : center_y - height * 0.5f;

    const Naui_Vec2 points[3] =
    {
        { center_x - width * 0.5f, base_y },
        { center_x + width * 0.5f, base_y },
        { center_x,                tip_y }
    };
    naui_fill_polygon(points, 3, data->color);
}

static Leaf_Color uph__ui_dim_text_color(void)
{
    Leaf_Color color = uph_state.colors.ui_text;
    color.a = (uint8_t)(color.a * 0.6f);
    return color;
}

static float uph__ui_tall_frame_height(void)
{
    return uph__ui_frame_height() + NAUI_DPI(8);
}

static bool uph__ui_triangle_button(const Leaf_ID id, bool up, float width, float height, Naui_CornerFlags corners)
{
    const bool hovered = uph_ui_widget_hovered(id);
    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    Leaf_Color background = LEAF_COLOR_TRANSPARENT;
    if (hovered)
        background = naui_mouse_down(NAUI_MOUSE_LEFT) ? uph_state.colors.ui_frame_pressed_bg : uph_state.colors.ui_frame_hovered_bg;

    Uph_UITriangleDrawData draw_data = {
        .color = hovered ? uph_state.colors.ui_text : uph__ui_dim_text_color(),
        .up = up
    };

    leaf({
        .id = id,
        .custom_draw = uph__ui_triangle_draw,
        .custom_draw_data = LEAF_DATA_SLICE(draw_data),
        .size = { .width = LEAF_SIZE_FIXED(width), .height = LEAF_SIZE_FIXED(height) },
        .color = background,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), corners)
    });

    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void uph__ui_frame_icon(Naui_Image *image, float size)
{
    leaf({
        .image = image,
        .size = { .width = LEAF_SIZE_FIXED(size), .height = LEAF_SIZE_FIXED(size) },
        .color = uph__ui_dim_text_color()
    });
}

static void uph__ui_frame_divider(float height)
{
    Leaf_Color color = uph_state.colors.ui_text;
    color.a = (uint8_t)(color.a * 0.12f);
    leaf({
        .size = { .width = LEAF_SIZE_FIXED(NAUI_MAX(1.0f, NAUI_DPI(1))), .height = LEAF_SIZE_FIXED(height) },
        .color = color
    });
}

bool uph_ui_bpm(float *value, const char *id, float min, float max)
{
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const float frame_height = uph__ui_tall_frame_height();
    const int32_t arrow_width = NAUI_DPI(16);
    const float arrow_height = (frame_height - NAUI_DPI(6)) * 0.5f;
    const float icon_size = font_size * 1.15f;
    const Leaf_ID tap_id = leaf_id_indexed(id, 3);
    bool changed = false;

    const bool tap_hovered = uph_ui_widget_hovered(tap_id);
    if (tap_hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        float tapped_bpm;
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && uph__metronome_tap(&tapped_bpm))
        {
            *value = NAUI_CLAMP(roundf(tapped_bpm * 100.0f) / 100.0f, min, max);
            changed = true;
        }
    }

    Leaf_Color tap_background = LEAF_COLOR_TRANSPARENT;
    if (tap_hovered)
        tap_background = naui_mouse_down(NAUI_MOUSE_LEFT) ? uph_state.colors.ui_frame_pressed_bg : uph_state.colors.ui_frame_hovered_bg;

    leaf({
        .id = leaf_id(id),
        .size = { .width = LEAF_SIZE_FIT, .height = UPH_UI_SIZE_EXACT(frame_height) },
        .padding = { .left = NAUI_DPI(3), .right = NAUI_DPI(3) },
        .border = uph__ui_frame_border(),
        .color = uph_state.colors.ui_frame_bg,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
        .child_gap = NAUI_DPI(6),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
            .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
            .direction = LEAF_DIRECTION_HORIZONTAL
        })
        {
            leaf({
                .id = tap_id,
                .size = { .width = LEAF_SIZE_FIXED(icon_size + NAUI_DPI(padding.x) * 2.0f), .height = LEAF_SIZE_FIXED(frame_height - NAUI_DPI(6)) },
                .color = tap_background,
                .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL)
            })
            {
                leaf({
                    .image = uph_state.icons.metronome,
                    .size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
                    .color = tap_hovered ? uph_state.colors.ui_text : uph__ui_dim_text_color()
                });
            }

            leaf({
                .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(48)), .height = LEAF_SIZE_FIT },
                .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
                .direction = LEAF_DIRECTION_VERTICAL
            })
            {
                if (uph_ui_drag_float(value, leaf_id_indexed(id, 0), 1.0f, min, max, "%.2f", UPH_UI_DRAG_FLAG_CLAMPED | UPH_UI_DRAG_FLAG_FRAMELESS))
                    changed = true;

                leaf_text("BPM", {
                    .color = uph__ui_dim_text_color(),
                    .font_size = LEAF_SIZE_FIXED(font_size * 0.72f)
                });
            }
        }

        uph__ui_frame_divider(frame_height - NAUI_DPI(12));

        leaf({
            .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
            .direction = LEAF_DIRECTION_VERTICAL
        })
        {
            if (uph__ui_triangle_button(leaf_id_indexed(id, 1), true, arrow_width, arrow_height, NAUI_CORNER_TL | NAUI_CORNER_TR))
            {
                *value = NAUI_CLAMP(floorf(*value) + 1.0f, min, max);
                changed = true;
            }
            if (uph__ui_triangle_button(leaf_id_indexed(id, 2), false, arrow_width, arrow_height, NAUI_CORNER_BL | NAUI_CORNER_BR))
            {
                *value = NAUI_CLAMP(ceilf(*value) - 1.0f, min, max);
                changed = true;
            }
        }
    }

    return changed;
}

const char *uph_ui_format_time(double beat, bool seconds_format, char *out, size_t out_size)
{
    if (seconds_format)
    {
        const double seconds = NAUI_MAX(uph_tempo_beat_to_seconds(NAUI_MAX(beat, 0.0)), 0.0);
        const int64_t total_ms = (int64_t)floor(seconds * 1000.0);
        snprintf(out, out_size, "%02d:%02d.%03d",
            (int)(total_ms / 60000),
            (int)((total_ms / 1000) % 60),
            (int)(total_ms % 1000));
        return "MIN:SEC";
    }

    const double beat_length = uph_time_signature_beat_length();
    const int64_t beats_per_bar = NAUI_MAX((int64_t)llround(uph_time_signature_bar_length() / beat_length), (int64_t)1);
    const double beat_units = NAUI_MAX(beat, 0.0) / beat_length + 1e-9;
    const int64_t whole_beats = (int64_t)floor(beat_units);
    const int ticks = (int)floor((beat_units - (double)whole_beats) * 96.0);
    snprintf(out, out_size, "%d:%d:%02d",
        (int)(whole_beats / beats_per_bar),
        (int)(whole_beats % beats_per_bar) + 1,
        ticks);
    return "BAR:BEAT:TICK";
}

bool uph_ui_time_signature(Uph_TimeSignature *signature, const char *id)
{
    static const uint32_t denominator_values[] = { 2, 4, 8, 16 };
    static const char *denominator_labels[] = { "2", "4", "8", "16" };
    static const Uph_TimeSignature presets[] = {
        {2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 8}, {7, 8}, {9, 8}, {12, 8}, {15, 16}
    };
    static const char *preset_labels[] = { "2/4", "3/4", "4/4", "5/4", "6/8", "7/8", "9/8", "12/8", "15/16" };
    const uint32_t preset_count = sizeof(presets) / sizeof(presets[0]);

    Uph_GlobalWidgetState *data = &uph_global_widget_state;
    const Leaf_ID frame_id = leaf_id(id);
    const Leaf_ID denominator_id = leaf_id_indexed(id, 1);
    const Leaf_ID arrow_id = leaf_id_indexed(id, 2);
    Uph_UIMenuNode *menu = uph__ui_menu_root(data, frame_id);

    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const float frame_height = uph__ui_tall_frame_height();
    const Leaf_Color text_color = uph_state.colors.ui_text;
    bool changed = false;

    const bool was_open = uph__ui_menu_is_open(data, menu);
    const bool denominator_hovered = uph_ui_widget_hovered(denominator_id) || (was_open && leaf_hovered(denominator_id));
    const bool arrow_hovered = uph_ui_widget_hovered(arrow_id) || (was_open && leaf_hovered(arrow_id));
    if (denominator_hovered || arrow_hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            if (was_open)
                uph__ui_close_menu(data);
            else
                uph__ui_open_menu(data, menu, false);
        }
    }
    const bool open = uph__ui_menu_is_open(data, menu);

    char denominator_text[8];
    snprintf(denominator_text, sizeof(denominator_text), "%u", (unsigned)signature->denominator);

    Leaf_Color arrow_background = LEAF_COLOR_TRANSPARENT;
    if (open)
        arrow_background = uph_state.colors.ui_frame_pressed_bg;
    else if (arrow_hovered)
        arrow_background = uph_state.colors.ui_frame_hovered_bg;

    Uph_UITriangleDrawData arrow_data = {
        .color = (open || arrow_hovered) ? text_color : uph__ui_dim_text_color(),
        .up = false
    };

    leaf({
        .id = frame_id,
        .size = { .width = LEAF_SIZE_FIT, .height = UPH_UI_SIZE_EXACT(frame_height) },
        .padding = { .left = NAUI_DPI(padding.x), .right = NAUI_DPI(3) },
        .border = uph__ui_frame_border(),
        .color = uph_state.colors.ui_frame_bg,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
        .child_gap = NAUI_DPI(6),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
            .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
            .child_gap = NAUI_DPI(1),
            .direction = LEAF_DIRECTION_HORIZONTAL
        })
        {
            uph__ui_frame_icon(uph_state.icons.treble_clef, font_size * 1.9f);

            leaf({
                .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(16)), .height = LEAF_SIZE_FIT },
                .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
                .direction = LEAF_DIRECTION_VERTICAL
            })
            {
                if (uph_ui_drag_int((int32_t*)&signature->numerator, leaf_id_indexed(id, 0), 0.1f, 1, 32, "%d", UPH_UI_DRAG_FLAG_CLAMPED | UPH_UI_DRAG_FLAG_FRAMELESS))
                    changed = true;

                leaf({
                    .id = denominator_id,
                    .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
                    .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
                })
                {
                    leaf_text(denominator_text, {
                        .color = text_color,
                        .font_size = LEAF_SIZE_FIXED(font_size)
                    });
                }
            }
        }

        uph__ui_frame_divider(frame_height - NAUI_DPI(12));

        leaf({
            .id = arrow_id,
            .custom_draw = uph__ui_triangle_draw,
            .custom_draw_data = LEAF_DATA_SLICE(arrow_data),
            .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(16)), .height = LEAF_SIZE_FIXED(frame_height - NAUI_DPI(6)) },
            .color = arrow_background,
            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL)
        });
    }

    for (uint32_t i = 0; i < preset_count; i++)
    {
        if (uph_ui_menu_item((Uph_UIMenuID)menu, preset_labels[i], leaf_id_indexed(id, 16 + i)))
        {
            signature->numerator = presets[i].numerator;
            signature->denominator = presets[i].denominator;
            changed = true;
        }
    }

    Uph_UIMenuID note_menu = uph_ui_submenu((Uph_UIMenuID)menu, "Note Value", leaf_id_indexed(id, 8));
    for (uint32_t i = 0; i < 4; i++)
    {
        if (uph_ui_menu_item(note_menu, denominator_labels[i], leaf_id_indexed(id, 32 + i)))
        {
            signature->denominator = denominator_values[i];
            changed = true;
        }
    }

    return changed;
}
