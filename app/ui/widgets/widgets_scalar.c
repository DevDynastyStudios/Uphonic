static bool _uph_ui_slider_scalar(const Leaf_ID id, void *value, bool is_float, float min_f, float max_f, int32_t min_i, int32_t max_i, const char *format, Uph_UISliderFlags flags, Leaf_Color handle_color, Leaf_Color fill_color)
{
    Uph_TextfieldData *tdata = &uph_global_widget_state.textfield_data;
    static Naui_String edit_buffer;
    bool text_editing = (tdata->id.value == id.value) && (tdata->mode == UPH_UI_EDIT_MODE_TEXT) && (tdata->target_number == value);
    bool result = false;
    const bool hovered = uph_ui_widget_hovered(id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    if (!text_editing && hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
    {
        if (is_float)
            edit_buffer = naui_string_format(format ? (char*)format : "%.3f", *(float*)value);
        else
            edit_buffer = naui_string_format(format ? (char*)format : "%d", *(int32_t*)value);

        _uph_ui_begin_text_edit(tdata, id, &edit_buffer, edit_buffer.length);
        tdata->target_number = value;

        text_editing = true;
    }

    if (text_editing)
    {
        bool sub_result = uph_ui_textfield(&edit_buffer, id, UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY, NULL);
        if (sub_result)
        {
            if (is_float)
                *(float*)value = NAUI_CLAMP((float)atof(edit_buffer.data), min_f, max_f);
            else
                *(int32_t*)value = NAUI_CLAMP((int32_t)atoi(edit_buffer.data), min_i, max_i);
            result = true;
        }

        if (tdata->id.value != id.value || tdata->mode != UPH_UI_EDIT_MODE_TEXT)
            tdata->target_number = NULL;

        return result;
    }

    Uph_DragData *ddata = &uph_global_widget_state.drag_data;
    Leaf_BoundingBox box = leaf_get_bounding_box(id);
    const bool vertical = (flags & UPH_UI_SLIDER_FLAG_VERTICAL) != 0;

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        ddata->id = id;

    bool dragging_this = (ddata->id.value == id.value);
    if (dragging_this && !naui_mouse_down(NAUI_MOUSE_LEFT))
    {
        ddata->id.value = 0;
        dragging_this = false;
    }

    if (dragging_this)
        naui_set_cursor(NAUI_CURSOR_HAND);

    if (dragging_this)
    {
        float t;
        if (vertical)
        {
            float local_y = (float)naui_mouse_y() - box.y;
            t = 1.0f - NAUI_CLAMP(local_y / NAUI_MAX(box.height, 1.0f), 0.0f, 1.0f);
        }
        else
        {
            float local_x = (float)naui_mouse_x() - box.x;
            t = NAUI_CLAMP(local_x / NAUI_MAX(box.width, 1.0f), 0.0f, 1.0f);
        }

        if (is_float)
        {
            float new_value = min_f + t * (max_f - min_f);
            if (new_value != *(float*)value)
            {
                *(float*)value = new_value;
                result = true;
            }
        }
        else
        {
            int32_t new_value = min_i + (int32_t)(t * (float)(max_i - min_i) + 0.5f);
            if (new_value != *(int32_t*)value)
            {
                *(int32_t*)value = new_value;
                result = true;
            }
        }
    }

    float t;
    if (is_float)
        t = (max_f > min_f) ? NAUI_CLAMP((*(float*)value - min_f) / (max_f - min_f), 0.0f, 1.0f) : 0.0f;
    else
        t = (max_i > min_i) ? NAUI_CLAMP((float)(*(int32_t*)value - min_i) / (float)(max_i - min_i), 0.0f, 1.0f) : 0.0f;

    const int32_t track_size = NAUI_DPI(UPH_UI_SLIDER_TRACK_SIZE);
    const int32_t handle_size = NAUI_DPI(dragging_this || hovered ? UPH_UI_SLIDER_HANDLE_HOVERED_SIZE : UPH_UI_SLIDER_HANDLE_SIZE);
    const Leaf_Size travel_size = vertical ? (Leaf_Size){ .width = LEAF_SIZE_FIXED(track_size), .height = LEAF_SIZE_PERCENT(t) }
        : (Leaf_Size){ .width = LEAF_SIZE_PERCENT(t), .height = LEAF_SIZE_FIXED(track_size) };
    const Leaf_Size cap_size = vertical ? (Leaf_Size){ .width = LEAF_SIZE_FIXED(track_size), .height = LEAF_SIZE_FIXED(handle_size / 2) }
        : (Leaf_Size){ .width = LEAF_SIZE_FIXED(handle_size / 2), .height = LEAF_SIZE_FIXED(track_size) };
    const Leaf_Size handle_box = { .width = LEAF_SIZE_FIXED(handle_size), .height = LEAF_SIZE_FIXED(handle_size) };
    const Leaf_Size layer_size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL };
    const Leaf_Padding layer_padding = vertical ? (Leaf_Padding){ .top = handle_size } : (Leaf_Padding){ .right = handle_size };
    const Leaf_Alignment layer_alignment = vertical ? (Leaf_Alignment){ LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_BOTTOM } : (Leaf_Alignment){ LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER };
    const Leaf_LayoutDirection layer_direction = vertical ? LEAF_DIRECTION_VERTICAL : LEAF_DIRECTION_HORIZONTAL;
    const Leaf_Floating layer_floating = { .parent_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP }, .self_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP } };

    leaf({
        .id = id,
        .size = {
            .width = vertical ? UPH_UI_SIZE_EXACT(_uph_ui_frame_height()) : LEAF_SIZE_GROW,
            .height = vertical ? LEAF_SIZE_GROW : UPH_UI_SIZE_EXACT(_uph_ui_frame_height())
        }
    })
    {
        leaf({
            .size = vertical ? (Leaf_Size){ .width = LEAF_SIZE_FIXED(track_size), .height = LEAF_SIZE_FULL }
                : (Leaf_Size){ .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(track_size) },
            .color = uph_state.colors.ui_well_bg,
            .floating = { .parent_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }, .self_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER } },
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL),
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
        });

        leaf({
            .size = layer_size,
            .padding = layer_padding,
            .child_alignment = layer_alignment,
            .direction = layer_direction,
            .floating = layer_floating,
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
        })
        {
            if (vertical)
                leaf({ .size = cap_size, .color = fill_color });

            leaf({
                .size = travel_size,
                .color = fill_color,
                .rounding = LEAF_ROUNDING_FIXED(track_size / 2, vertical ? (LEAF_CORNER_BL | LEAF_CORNER_BR) : (LEAF_CORNER_TL | LEAF_CORNER_BL))
            });

            if (!vertical)
                leaf({ .size = cap_size, .color = fill_color });
        }

        leaf({
            .size = layer_size,
            .padding = layer_padding,
            .child_alignment = layer_alignment,
            .direction = layer_direction,
            .floating = layer_floating,
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
        })
        {
            if (vertical)
                leaf({ .size = handle_box, .border = { .width = NAUI_DPI(2), .sides = LEAF_SIDE_ALL, .color = fill_color }, .color = handle_color, .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL) });

            leaf({ .size = vertical ? (Leaf_Size){ .width = LEAF_SIZE_FIXED(0), .height = LEAF_SIZE_PERCENT(t) } : (Leaf_Size){ .width = LEAF_SIZE_PERCENT(t), .height = LEAF_SIZE_FIXED(0) } });

            if (!vertical)
                leaf({ .size = handle_box, .border = { .width = NAUI_DPI(2), .sides = LEAF_SIDE_ALL, .color = fill_color }, .color = handle_color, .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL) });
        }
    }

    return result;
}

static bool _uph_ui_drag_scalar(const Leaf_ID id, void *value, bool is_float, float speed, float min_f, float max_f, int32_t min_i, int32_t max_i, const char *format, Uph_UIDragFlags flags)
{
    Uph_TextfieldData *tdata = &uph_global_widget_state.textfield_data;
    Uph_DragData *ddata = &uph_global_widget_state.drag_data;

    bool result = false;

    static Naui_String edit_buffer;

    bool text_editing = (tdata->id.value == id.value) && (tdata->mode == UPH_UI_EDIT_MODE_TEXT) && (tdata->target_number == value);

    const bool hovered = uph_ui_widget_hovered(id);
    const bool dragging_this = (ddata->id.value == id.value);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    if (!text_editing && hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
    {
        if (is_float)
            edit_buffer = naui_string_format(format ? (char*)format : "%.3f", *(float*)value);
        else
            edit_buffer = naui_string_format(format ? (char*)format : "%d", *(int32_t*)value);

        _uph_ui_begin_text_edit(tdata, id, &edit_buffer, edit_buffer.length);
        tdata->target_number = value;
        ddata->id.value = 0;

        text_editing = true;
    }

    if (text_editing)
    {
        bool sub_result = uph_ui_textfield(&edit_buffer, id, UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY, NULL);

        if (sub_result)
        {
            if (is_float)
            {
                float parsed = (float)atof(edit_buffer.data);
                if (flags & UPH_UI_DRAG_FLAG_CLAMPED)
                    parsed = NAUI_CLAMP(parsed, min_f, max_f);
                *(float*)value = parsed;
            }
            else
            {
                int32_t parsed = (int32_t)atoi(edit_buffer.data);
                if (flags & UPH_UI_DRAG_FLAG_CLAMPED)
                    parsed = NAUI_CLAMP(parsed, min_i, max_i);
                *(int32_t*)value = parsed;
            }
            result = true;
        }

        if (tdata->id.value != id.value || tdata->mode != UPH_UI_EDIT_MODE_TEXT)
            tdata->target_number = NULL;

        return result;
    }

    static int32_t drag_last_mouse_x = 0;
    static float drag_int_accum = 0.0f;

    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT) && !dragging_this)
    {
        ddata->id = id;
        ddata->accum = 0.0f;
        drag_last_mouse_x = naui_mouse_x();
        drag_int_accum = 0.0f;
        if (is_float)
            ddata->start_value_f = *(float*)value;
        else
            ddata->start_value_i = *(int32_t*)value;
    }

    if (dragging_this)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
        {
            int32_t mx = naui_mouse_x();
            float delta = (float)(mx - drag_last_mouse_x);
            drag_last_mouse_x = mx;

            ddata->accum += delta * speed;

            if (is_float)
            {
                float new_value = *(float*)value + delta * speed;
                if (flags & UPH_UI_DRAG_FLAG_CLAMPED)
                    new_value = NAUI_CLAMP(new_value, min_f, max_f);
                if (new_value != *(float*)value)
                {
                    *(float*)value = new_value;
                    result = true;
                }
            }
            else
            {
                drag_int_accum += delta * speed;
                if (fabsf(drag_int_accum) >= 1.0f)
                {
                    int32_t step = (int32_t)drag_int_accum;
                    drag_int_accum -= (float)step;

                    int32_t new_value = *(int32_t*)value + step;
                    if (flags & UPH_UI_DRAG_FLAG_CLAMPED)
                        new_value = NAUI_CLAMP(new_value, min_i, max_i);
                    if (new_value != *(int32_t*)value)
                    {
                        *(int32_t*)value = new_value;
                        result = true;
                    }
                }
            }
        }
        else
        {
            ddata->id.value = 0;
        }
    }

    Naui_String display;
    if (is_float)
        display = naui_string_format(format ? (char*)format : "%.3f", *(float*)value);
    else
        display = naui_string_format(format ? (char*)format : "%d", *(int32_t*)value);

    const bool tinted = (flags & UPH_UI_DRAG_FLAG_TINTED) != 0;
    const Leaf_Color bg_color = tinted ? uph_state.colors.ui_tinted_frame_bg : uph_state.colors.ui_frame_bg;
    const Leaf_Color hovered_bg_color = tinted ? uph_state.colors.ui_tinted_frame_hovered_bg : uph_state.colors.ui_frame_hovered_bg;
    const Leaf_Color pressed_bg_color = tinted ? uph_state.colors.ui_tinted_frame_pressed_bg : uph_state.colors.ui_frame_pressed_bg;

    const Leaf_Color color = dragging_this ? pressed_bg_color : (hovered ? hovered_bg_color : bg_color);
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;

    const bool frameless = (flags & UPH_UI_DRAG_FLAG_FRAMELESS) != 0;

    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_GROW,
            .height = frameless ? LEAF_SIZE_FIT : UPH_UI_SIZE_EXACT(_uph_ui_frame_height())
        },
        .padding = frameless ? (Leaf_Padding){0} : LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .border = frameless ? (Leaf_Border){0} : _uph_ui_border(tinted ? uph_state.colors.ui_tinted_frame_border : uph_state.colors.ui_frame_border),
        .color = frameless ? LEAF_COLOR_TRANSPARENT : color,
        .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
        .clip_children = true
    })
    {
        leaf_text(display.data, {
            .color = uph_state.colors.ui_text,
            .font_size = LEAF_SIZE_FIXED(font_size)
        });
    }

    return result;
}

bool uph_ui_drag_float(float *value, const Leaf_ID id, float speed, float min, float max, const char *format, Uph_UIDragFlags flags)
{
    return _uph_ui_drag_scalar(id, value, true, speed, min, max, 0, 0, format, flags);
}

bool uph_ui_drag_int(int32_t *value, const Leaf_ID id, float speed, int32_t min, int32_t max, const char *format, Uph_UIDragFlags flags)
{
    return _uph_ui_drag_scalar(id, value, false, speed, 0.0f, 0.0f, min, max, format, flags);
}

bool uph_ui_slider_float_ex(float *value, const Leaf_ID id, float min, float max, const char *format, Uph_UISliderFlags flags, Leaf_Color handle_color, Leaf_Color fill_color)
{
    return _uph_ui_slider_scalar(id, value, true, min, max, 0, 0, format, flags, handle_color, fill_color);
}

bool uph_ui_slider_float(float *value, const Leaf_ID id, float min, float max, const char *format, Uph_UISliderFlags flags)
{
    return uph_ui_slider_float_ex(value, id, min, max, format, flags, uph_state.colors.panel_body_bg, uph_state.colors.ui_slider_fill);
}

bool uph_ui_slider_int(int32_t *value, const Leaf_ID id, int32_t min, int32_t max, const char *format, Uph_UISliderFlags flags)
{
    return _uph_ui_slider_scalar(id, value, false, 0.0f, 0.0f, min, max, format, flags, uph_state.colors.panel_body_bg, uph_state.colors.ui_slider_fill);
}
