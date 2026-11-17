static bool uph__ui_is_word_char(char c)
{
    return (unsigned char)c >= 0x80 || isalnum((unsigned char)c) || c == '_';
}

static void uph__ui_textfield_finish_composition(Uph_TextfieldData *data, bool commit)
{
    Naui_ImeComposition ime;
    if (!naui_ime_get_composition(&ime))
        return;

    if (commit && data->mode == UPH_UI_EDIT_MODE_TEXT && data->target_string)
    {
        size_t i = 0;
        while (i < (size_t)ime.length)
        {
            uint32_t codepoint = 0;
            const size_t step = naui_utf8_next(ime.text + i, (size_t)ime.length - i, &codepoint);
            if (step == 0)
            {
                ++i;
                continue;
            }
            i += step;

            const size_t inserted = naui_string_insert_codepoint(data->target_string, data->cursor, codepoint);
            if (inserted == 0)
                break;

            data->cursor += inserted;
        }
    }

    naui_ime_cancel();
}

static void uph__ui_begin_text_edit(Uph_TextfieldData *data, const Leaf_ID id, Naui_String *value, size_t cursor)
{
    uph__ui_textfield_finish_composition(data, true);

    data->id = id;
    data->mode = UPH_UI_EDIT_MODE_TEXT;
    data->target_string = value;
    data->target_number = NULL;
    data->cursor = NAUI_MIN(cursor, value->length);
    data->select_start = data->cursor;
    data->select_active = false;
    data->scroll_x = 0.0f;
}

static void uph__ui_end_edit(Uph_TextfieldData *data)
{
    uph__ui_textfield_finish_composition(data, false);

    data->id.value = 0;
    data->mode = UPH_UI_EDIT_MODE_NONE;
    data->target_string = NULL;
    data->target_number = NULL;
    data->select_active = false;
}

static size_t uph__ui_prev_word(const Naui_String *value, size_t cursor)
{
    if (cursor == 0)
        return 0;

    size_t i = cursor;
    while (i > 0 && !uph__ui_is_word_char(value->data[i - 1]))
        --i;
    while (i > 0 && uph__ui_is_word_char(value->data[i - 1]))
        --i;
    return i;
}

static size_t uph__ui_next_word(const Naui_String *value, size_t cursor)
{
    if (cursor >= value->length)
        return value->length;

    size_t i = cursor;
    while (i < value->length && !uph__ui_is_word_char(value->data[i]))
        ++i;
    while (i < value->length && uph__ui_is_word_char(value->data[i]))
        ++i;
    return i;
}

static void uph__ui_textfield_delete_selection(Uph_TextfieldData *data)
{
    if (!data->select_active)
        return;

    size_t lo = NAUI_MIN(data->cursor, data->select_start);
    size_t hi = NAUI_MAX(data->cursor, data->select_start);

    naui_string_remove(data->target_string, lo, hi - lo);
    data->cursor = lo;
    data->select_active = false;
}

static bool uph__ui_char_allowed(uint32_t codepoint, Uph_UITextFieldFlags flags, const Naui_String *value, size_t cursor)
{
    if (!(flags & UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY))
        return true;

    if (codepoint >= '0' && codepoint <= '9')
        return true;

    if (codepoint == '-' && cursor == 0)
        return true;

    if (codepoint == '.')
    {
        for (size_t i = 0; i < value->length; ++i)
            if (value->data[i] == '.')
                return false;
        return true;
    }

    return false;
}

static size_t uph__ui_cursor_from_x(const Naui_String *value, float local_x, float font_size)
{
    if (local_x <= 0.0f)
        return 0;

    size_t best = value->length;
    float best_dist = fabsf(naui_measure_text(value->data, value->length, font_size).x - local_x);

    for (size_t i = 0; i <= value->length; ++i)
    {
        if (!naui_string_is_char_boundary(value, i))
            continue;

        float x = naui_measure_text(value->data, i, font_size).x;
        float dist = fabsf(x - local_x);
        if (dist < best_dist)
        {
            best_dist = dist;
            best = i;
        }
    }

    return best;
}

static Leaf_BoundingBox uph__ui_textfield_line_box(Leaf_BoundingBox box, float font_size)
{
    const float line_height = NAUI_MIN(naui_measure_text("Ag", 2, font_size).y, box.height);
    box.y += (int32_t)roundf(((float)box.height - line_height) * 0.5f);
    box.height = (int32_t)roundf(line_height);
    return box;
}

static void uph__ui_textfield_text_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldDrawData *draw_data = (Uph_TextfieldDrawData*)user_data;
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Leaf_BoundingBox clip = box;
    box = uph__ui_textfield_line_box(box, font_size);

    Naui_ImeComposition ime = { 0 };
    const bool has_preedit = draw_data->composing && naui_ime_get_composition(&ime);

    naui_push_clip_rect(clip.x, clip.y, clip.width, clip.height);
    if (has_preedit)
    {
        const size_t at = NAUI_MIN(draw_data->cursor, draw_data->value->length);
        const Naui_String composed = naui_string_format("%.*s%.*s%s", (int)at, draw_data->value->data, (int)ime.length, ime.text, draw_data->value->data + at);
        const float origin_x = box.x - draw_data->scroll_x;
        const Naui_Color text_color = uph_state.colors.ui_text;

        naui_draw_text((Naui_Vec2) { origin_x, box.y }, composed.data, font_size, text_color);

        const float start_x = origin_x + naui_measure_text(draw_data->value->data, (uint32_t)at, font_size).x;
        const float thin = NAUI_MAX(1.0f, NAUI_DPI(1));
        const float thick = NAUI_MAX(2.0f, NAUI_DPI(2));
        const float underline_y = box.y + box.height - thick;
        const float gap = NAUI_MAX(1.0f, NAUI_DPI(2));

        for (int32_t i = 0; i < ime.clause_count; ++i)
        {
            const Naui_ImeClause clause = ime.clauses[i];
            const float clause_x0 = naui_measure_text(ime.text, (uint32_t)clause.start, font_size).x;
            const float clause_x1 = naui_measure_text(ime.text, (uint32_t)clause.end, font_size).x;
            const float clause_width = NAUI_MAX(clause_x1 - clause_x0 - (i + 1 < ime.clause_count ? gap : 0.0f), 1.0f);
            const float height = clause.target ? thick : thin;
            naui_fill_rect((Naui_Vec2) { start_x + clause_x0, underline_y + (thick - height) }, (Naui_Vec2) { clause_width, height }, text_color, 0.0f, NAUI_CORNER_NONE);
        }
    }
    else if (draw_data->value->length)
    {
        naui_draw_text(
            (Naui_Vec2) { box.x - draw_data->scroll_x, box.y },
            draw_data->value->data,
            font_size,
            uph_state.colors.ui_text
        );
    }
    else if (draw_data->placeholder)
    {
        naui_draw_text(
            (Naui_Vec2) { box.x, box.y },
            draw_data->placeholder,
            font_size,
            uph_state.colors.ui_text_disabled
        );
    }
    naui_pop_clip_rect();
}

static void uph__ui_textfield_selection_draw(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldData* data = (Uph_TextfieldData*)user_data;

    if (!data->select_active || data->select_start == data->cursor)
        return;

    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    box = uph__ui_textfield_line_box(box, font_size);
    const float draw_x = box.x - data->scroll_x;

    size_t lo = NAUI_MIN(data->cursor, data->select_start);
    size_t hi = NAUI_MAX(data->cursor, data->select_start);

    const Naui_Vec2 lo_pos = naui_measure_text(data->target_string->data, lo, font_size);
    const Naui_Vec2 hi_pos = naui_measure_text(data->target_string->data, hi, font_size);

    naui_push_clip_rect(box.x, box.y, box.width, box.height);
    naui_fill_rect(
        (Naui_Vec2) { draw_x + lo_pos.x, box.y },
        (Naui_Vec2) { hi_pos.x - lo_pos.x, box.height },
        uph_state.colors.ui_selection,
        0.0f,
        NAUI_CORNER_NONE
    );
    naui_pop_clip_rect();
}

static void uph__ui_textfield_cursor(Leaf_BoundingBox box, void *user_data)
{
    const Uph_TextfieldData* data = (Uph_TextfieldData*)user_data;

    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    box = uph__ui_textfield_line_box(box, font_size);
    const float draw_x = box.x - data->scroll_x;

    Naui_Vec2 text_size = naui_measure_text(data->target_string->data, data->cursor, font_size);

    Naui_ImeComposition ime;
    if (naui_ime_get_composition(&ime))
        text_size.x += naui_measure_text(ime.text, (uint32_t)ime.cursor, font_size).x;

    const int32_t cursor_width = NAUI_DPI(2);

    naui_push_clip_rect(box.x, box.y, box.width + cursor_width, box.height);
    naui_fill_rect(
        (Naui_Vec2) {
            draw_x + text_size.x,
            box.y
        },
        (Naui_Vec2) {
            cursor_width,
            box.height
        },
        uph_state.colors.ui_text,
        0.0f,
        NAUI_CORNER_NONE
    );
    naui_pop_clip_rect();
}

bool uph_ui_any_textfield_active(void)
{
    return uph_global_widget_state.textfield_data.mode == UPH_UI_EDIT_MODE_TEXT;
}

bool uph_ui_textfield_active(const Leaf_ID id)
{
    const Uph_TextfieldData *data = &uph_global_widget_state.textfield_data;
    return data->id.value == id.value && data->mode == UPH_UI_EDIT_MODE_TEXT;
}

static void uph__ui_textfield_copy_selection(const Uph_TextfieldData *data)
{
    if (!data->select_active || data->select_start == data->cursor)
        return;

    const size_t lo = NAUI_MIN(data->cursor, data->select_start);
    const size_t hi = NAUI_MAX(data->cursor, data->select_start);
    Naui_String *s = data->target_string;

    const char saved = s->data[hi];
    s->data[hi] = '\0';
    naui_app_set_clipboard(s->data + lo);
    s->data[hi] = saved;
}

static void uph__ui_textfield_paste(Uph_TextfieldData *data, Naui_String *value, Uph_UITextFieldFlags flags)
{
    const char *clip = naui_app_get_clipboard();
    if (!clip || !*clip)
        return;

    uph__ui_textfield_delete_selection(data);

    const size_t clip_length = strlen(clip);
    size_t i = 0;
    while (i < clip_length)
    {
        uint32_t codepoint = 0;
        const size_t step = naui_utf8_next(clip + i, clip_length - i, &codepoint);
        if (step == 0)
        {
            ++i;
            continue;
        }
        i += step;

        if (codepoint < 0x20 || codepoint == 0x7F)
            continue;

        if (!uph__ui_char_allowed(codepoint, flags, value, data->cursor))
            continue;

        const size_t inserted = naui_string_insert_codepoint(value, data->cursor, codepoint);
        if (inserted == 0)
            break;

        data->cursor += inserted;
    }
}

static void uph__ui_textfield_layers(Uph_TextfieldData *data, Uph_TextfieldDrawData *draw_data, bool active)
{
    if (active)
    {
        leaf({
            .custom_draw = uph__ui_textfield_selection_draw,
            .custom_draw_data = LEAF_DATA_SLICE(*data),
            .size = {
                .width = LEAF_SIZE_FULL,
                .height = LEAF_SIZE_FULL
            },
            .floating = {
                .parent_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
                .self_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
            },
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
        });
    }

    leaf({
        .custom_draw = uph__ui_textfield_text_draw,
        .custom_draw_data = LEAF_DATA_SLICE(*draw_data),
        .size = {
            .width = LEAF_SIZE_FULL,
            .height = LEAF_SIZE_FULL
        },
        .floating = {
            .parent_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
            .self_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
        },
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
    });

    leaf({
        .custom_draw = active ? uph__ui_textfield_cursor : NULL,
        .custom_draw_data = LEAF_DATA_SLICE(*data),
        .size = {
            .width = LEAF_SIZE_FULL,
            .height = LEAF_SIZE_FULL
        },
        .floating = {
            .parent_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
            .self_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
        },
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
    });
}

bool uph_ui_textfield(Naui_String* value, const Leaf_ID id, Uph_UITextFieldFlags flags, const char *placeholder)
{
    Uph_TextfieldData *data = &uph_global_widget_state.textfield_data;

    bool result = false;

    const bool hovered = uph_ui_widget_hovered(id);
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const float text_offset = (flags & UPH_UI_TEXTFIELD_FLAG_SEARCH) ? font_size + NAUI_DPI(6) : 0.0f;

    bool active = (data->id.value == id.value) && (data->mode == UPH_UI_EDIT_MODE_TEXT);

    if (!active && (flags & UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE) && data->id.value == 0)
    {
        uph__ui_begin_text_edit(data, id, value, value->length);
        active = true;
    }

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_IBEAM);

    if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        if (hovered)
        {
            Leaf_BoundingBox box = leaf_get_bounding_box(id);
            float local_x = (float)naui_mouse_x() - box.x - text_offset + (active ? data->scroll_x : 0.0f);

            uph__ui_begin_text_edit(data, id, value, uph__ui_cursor_from_x(value, local_x, font_size));
            active = true;
        }
        else if (active && !(flags & UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE))
        {
            uph__ui_textfield_finish_composition(data, true);
            uph__ui_end_edit(data);
            active = false;
            result = true;
        }
        else
        {
            result = true;
        }
    }

    if (active && hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
    {
        data->select_start = 0;
        data->cursor = value->length;
        data->select_active = (value->length > 0);
    }

    if (active && hovered && naui_mouse_dragging(NAUI_MOUSE_LEFT))
    {
        Leaf_BoundingBox box = leaf_get_bounding_box(id);
        float local_x = (float)naui_mouse_x() - box.x - text_offset + data->scroll_x;

        size_t new_cursor = uph__ui_cursor_from_x(value, local_x, font_size);
        if (!data->select_active)
        {
            data->select_start = data->cursor;
            data->select_active = true;
        }
        data->cursor = new_cursor;
        if (data->cursor == data->select_start)
            data->select_active = false;
    }

    if (active)
    {
        const bool ctrl = uph_ui_ctrl_down();
        const bool shift = uph_ui_shift_down();

        Naui_ImeComposition ime;
        const bool composing = naui_ime_get_composition(&ime);
        if (composing && data->select_active)
            uph__ui_textfield_delete_selection(data);

        uint32_t codepoint = naui_app_codepoint();
        while (codepoint)
        {
            if (codepoint != '\b' && codepoint != 0x7F && codepoint != '\t' && codepoint != '\x1b' && codepoint >= 0x20)
            {
                if (uph__ui_char_allowed(codepoint, flags, value, data->cursor))
                {
                    uph__ui_textfield_delete_selection(data);
                    data->cursor += naui_string_insert_codepoint(value, data->cursor, codepoint);
                }
            }
            codepoint = naui_app_codepoint();
        }

        if (!composing && naui_key_pressed_repeat(NAUI_KEY_BACKSPACE))
        {
            if (data->select_active)
                uph__ui_textfield_delete_selection(data);
            else if (data->cursor > 0)
            {
                size_t remove_from = ctrl ? uph__ui_prev_word(value, data->cursor) : naui_string_prev_char(value, data->cursor);
                naui_string_remove(value, remove_from, data->cursor - remove_from);
                data->cursor = remove_from;
            }
        }
        else if (naui_key_pressed_repeat(NAUI_KEY_DELETE))
        {
            if (data->select_active)
                uph__ui_textfield_delete_selection(data);
            else if (data->cursor < value->length)
            {
                size_t remove_to = ctrl ? uph__ui_next_word(value, data->cursor) : naui_string_next_char(value, data->cursor);
                naui_string_remove(value, data->cursor, remove_to - data->cursor);
            }
        }
        else if (naui_key_pressed(NAUI_KEY_ENTER))
        {
            uph__ui_end_edit(data);
            active = false;
            result = true;
        }
        else if (naui_key_pressed(NAUI_KEY_ESCAPE) && !(flags & UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE))
        {
            uph__ui_end_edit(data);
            active = false;
        }
        else if (naui_key_pressed(NAUI_KEY_LEFT))
        {
            size_t new_cursor = ctrl ? uph__ui_prev_word(value, data->cursor) : naui_string_prev_char(value, data->cursor);
            if (shift)
            {
                if (!data->select_active)
                {
                    data->select_start = data->cursor;
                    data->select_active = true;
                }
                data->cursor = new_cursor;
            }
            else
            {
                data->cursor = data->select_active ? NAUI_MIN(data->cursor, data->select_start) : new_cursor;
                data->select_active = false;
            }
        }
        else if (naui_key_pressed(NAUI_KEY_RIGHT))
        {
            size_t new_cursor = ctrl ? uph__ui_next_word(value, data->cursor) : naui_string_next_char(value, data->cursor);
            if (shift)
            {
                if (!data->select_active)
                {
                    data->select_start = data->cursor;
                    data->select_active = true;
                }
                data->cursor = new_cursor;
            }
            else
            {
                data->cursor = data->select_active ? NAUI_MAX(data->cursor, data->select_start) : new_cursor;
                data->select_active = false;
            }
        }
        else if (naui_key_pressed(NAUI_KEY_HOME))
        {
            if (shift)
            {
                if (!data->select_active)
                {
                    data->select_start = data->cursor;
                    data->select_active = true;
                }
            }
            else data->select_active = false;
            data->cursor = 0;
        }
        else if (naui_key_pressed(NAUI_KEY_END))
        {
            if (shift)
            {
                if (!data->select_active)
                {
                    data->select_start = data->cursor;
                    data->select_active = true;
                }
            }
            else data->select_active = false;
            data->cursor = value->length;
        }
        else if (ctrl && naui_key_pressed(NAUI_KEY_A))
        {
            data->select_start = 0;
            data->cursor = value->length;
            data->select_active = (value->length > 0);
        }
        else if (ctrl && naui_key_pressed(NAUI_KEY_C))
        {
            uph__ui_textfield_copy_selection(data);
        }
        else if (ctrl && naui_key_pressed(NAUI_KEY_X))
        {
            uph__ui_textfield_copy_selection(data);
            uph__ui_textfield_delete_selection(data);
        }
        else if (ctrl && naui_key_pressed(NAUI_KEY_V))
        {
            uph__ui_textfield_paste(data, value, flags);
        }
    }

    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const int32_t padding_x = NAUI_DPI(padding.x);
    const bool inline_field = (flags & UPH_UI_TEXTFIELD_FLAG_INLINE) != 0;
    const bool tinted = (flags & UPH_UI_TEXTFIELD_FLAG_TINTED) != 0;
    const float padding_y = inline_field ? NAUI_DPI(padding.y) : 0.0f;

    Leaf_BoundingBox field_box = leaf_get_bounding_box(id);
    const float visible_width = NAUI_MAX(field_box.width - padding_x * 2.0f - text_offset, 0.0f);

    if (active)
    {
        Naui_Vec2 cursor_pos = naui_measure_text(value->data, data->cursor, font_size);
        Naui_Vec2 full_pos = naui_measure_text(value->data, value->length, font_size);

        Naui_ImeComposition ime;
        if (naui_ime_get_composition(&ime))
        {
            cursor_pos.x += naui_measure_text(ime.text, (uint32_t)ime.cursor, font_size).x;
            full_pos.x += naui_measure_text(ime.text, (uint32_t)ime.length, font_size).x;
        }

        if (cursor_pos.x - data->scroll_x < 0.0f)
            data->scroll_x = cursor_pos.x;
        else if (cursor_pos.x - data->scroll_x > visible_width)
            data->scroll_x = cursor_pos.x - visible_width;

        const float max_scroll = NAUI_MAX(full_pos.x - visible_width, 0.0f);
        data->scroll_x = NAUI_CLAMP(data->scroll_x, 0.0f, max_scroll);
    }
    else
    {
        data->scroll_x = 0.0f;
    }

    if (active)
    {
        uph_global_widget_state.ime_mode = (flags & UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY) ? NAUI_IME_MODE_NUMBER : NAUI_IME_MODE_TEXT;
        Leaf_BoundingBox line_box = field_box;
        line_box.x += (int32_t)(padding_x + text_offset);
        line_box.width = (int32_t)visible_width;
        line_box = uph__ui_textfield_line_box(line_box, font_size);
        const float caret_x = naui_measure_text(value->data, data->cursor, font_size).x - data->scroll_x;
        naui_ime_set_area(line_box.x, line_box.y, line_box.width, line_box.height, (int32_t)caret_x);
    }

    leaf({
        .id = id,
        .size = {
            .width = LEAF_SIZE_GROW,
            .height = inline_field ? LEAF_SIZE_FIXED(font_size) : UPH_UI_SIZE_EXACT(uph__ui_frame_height())
        },
        .padding = LEAF_PADDING_AXES(padding_x, padding_y),
        .border = inline_field ? (Leaf_Border){0} : uph__ui_border(active ? uph_state.colors.panel_tab_accent : (tinted ? uph_state.colors.ui_tinted_frame_border : uph_state.colors.ui_frame_border)),
        .color = tinted ? uph_state.colors.ui_tinted_frame_bg : uph_state.colors.ui_frame_bg,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
        .child_gap = (flags & UPH_UI_TEXTFIELD_FLAG_SEARCH) ? text_offset - font_size : 0,
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        Uph_TextfieldDrawData draw_data = { value, placeholder, data->scroll_x, data->cursor, active && !(flags & UPH_UI_TEXTFIELD_FLAG_NUMBER_ONLY) };

        if (flags & UPH_UI_TEXTFIELD_FLAG_SEARCH)
        {
            leaf({
                .image = uph_state.icons.zoom,
                .size = { .width = LEAF_SIZE_FIXED(font_size), .height = LEAF_SIZE_FIXED(font_size) },
                .color = uph_state.colors.panel_text
            });
            leaf({
                .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL }
            })
            {
                uph__ui_textfield_layers(data, &draw_data, active);
            }
        }
        else uph__ui_textfield_layers(data, &draw_data, active);
    }

    if (data->id.value == id.value && data->mode == UPH_UI_EDIT_MODE_TEXT)
        uph_global_widget_state.textfield_submitted_this_frame = true;

    return result;
}
