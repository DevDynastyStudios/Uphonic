void uph_grid_lines(float origin_x, float width, float zoom_x, float scroll_x, double division, Uph_GridLineFn fn, void *user_data)
{
    const double beat_length = uph_time_signature_beat_length();
    const int64_t beats_per_bar = (int64_t)llround(uph_time_signature_bar_length() / beat_length);
    const float min_sub_spacing = NAUI_DPI(UPH_GRID_MIN_SUB_SPACING);
    const float min_beat_spacing = NAUI_DPI(UPH_GRID_MIN_BEAT_SPACING);

    while (division > 0.0 && division < beat_length && division * zoom_x < min_sub_spacing)
        division *= 2.0;

    const float division_px = (float)(division * zoom_x);
    if (division > 0.0 && division < beat_length && division_px >= min_sub_spacing)
    {
        const int64_t first = (int64_t)floor((double)scroll_x / division_px) - 1;
        const int64_t last = (int64_t)ceil((double)(scroll_x + width) / division_px);
        for (int64_t i = NAUI_MAX(first, 0); i <= last; i++)
        {
            const double beat = (double)i * division;
            const double phase = fmod(beat, beat_length);
            if (phase < 1e-6 || beat_length - phase < 1e-6)
                continue;

            fn(origin_x + (float)(beat * zoom_x) - scroll_x, UPH_GRID_LINE_SUBDIVISION, 0, user_data);
        }
    }

    const bool draw_beats = beat_length * zoom_x >= min_beat_spacing;
    const double step = draw_beats ? beat_length : beat_length * (double)beats_per_bar;
    const float step_px = (float)(step * zoom_x);
    const int64_t first = (int64_t)floor((double)scroll_x / step_px) - 1;
    const int64_t last = (int64_t)ceil((double)(scroll_x + width) / step_px);
    for (int64_t i = NAUI_MAX(first, 0); i <= last; i++)
    {
        const int64_t beat_index = draw_beats ? i : i * beats_per_bar;
        const bool is_bar = beat_index % beats_per_bar == 0;
        fn(
            origin_x + (float)((double)i * step * zoom_x) - scroll_x,
            is_bar ? UPH_GRID_LINE_BAR : UPH_GRID_LINE_BEAT,
            beat_index / beats_per_bar,
            user_data
        );
    }
}

typedef struct
{
    Leaf_BoundingBox bbox;
    Leaf_Color sub_color;
    Leaf_Color beat_color;
    Leaf_Color bar_color;
    Leaf_Color number_color;
}
Uph_GridStyle;

static void _uph_grid_draw_beat_line(float x, Uph_GridLineKind kind, int64_t bar_index, void *user_data)
{
    const Uph_GridStyle *style = user_data;
    if (x < style->bbox.x || x > style->bbox.x + style->bbox.width)
        return;

    const Leaf_Color color = kind == UPH_GRID_LINE_BAR ? style->bar_color : kind == UPH_GRID_LINE_BEAT ? style->beat_color : style->sub_color;
    naui_draw_line((Naui_Vec2){x, style->bbox.y}, (Naui_Vec2){x, style->bbox.y + style->bbox.height}, color, 1.0f);
}

static void _uph_grid_draw_ruler_line(float x, Uph_GridLineKind kind, int64_t bar_index, void *user_data)
{
    const Uph_GridStyle *style = user_data;
    const Leaf_BoundingBox bbox = style->bbox;
    if (x > bbox.x + bbox.width)
        return;

    if (kind == UPH_GRID_LINE_SUBDIVISION)
    {
        naui_draw_line((Naui_Vec2){x, bbox.y + bbox.height * 0.6f}, (Naui_Vec2){x, bbox.y + bbox.height}, style->sub_color, 1.0f);
        return;
    }

    const bool is_bar = kind == UPH_GRID_LINE_BAR;
    naui_draw_line(
        (Naui_Vec2){x, bbox.y + bbox.height * (is_bar ? 0.4f : 0.6f)},
        (Naui_Vec2){x, bbox.y + bbox.height},
        is_bar ? style->bar_color : style->beat_color,
        1.0f
    );

    if (is_bar)
    {
        char label[16];
        snprintf(label, sizeof(label), "%d", (int)bar_index);
        naui_draw_text((Naui_Vec2){x + 4.0f, bbox.y + bbox.height * 0.4f}, label, NAUI_DPI(13.0f), 0, style->number_color);
    }
}

void uph_grid_render_beats(Leaf_BoundingBox bbox, float zoom_x, float scroll_x, double division)
{
    Uph_GridStyle style = {
        .bbox = bbox,
        .sub_color = naui_theme_color("uph_timeline_grid_subbeat_color"),
        .beat_color = naui_theme_color("uph_timeline_grid_beat_color"),
        .bar_color = naui_theme_color("uph_timeline_grid_bar_color")
    };

    uph_grid_lines(bbox.x, bbox.width, zoom_x, scroll_x, division, _uph_grid_draw_beat_line, &style);
}

void uph_grid_render_ruler(Leaf_BoundingBox bbox, float zoom_x, float scroll_x, double division)
{
    Uph_GridStyle style = {
        .bbox = bbox,
        .sub_color = naui_theme_color("uph_timeline_top_ruler_grid_subbeat_color"),
        .beat_color = naui_theme_color("uph_timeline_top_ruler_grid_beat_color"),
        .bar_color = naui_theme_color("uph_timeline_top_ruler_grid_bar_color"),
        .number_color = naui_theme_color("uph_timeline_top_ruler_grid_text_color")
    };

    naui_push_clip_rect(bbox.x - 0.5f, bbox.y, bbox.width, bbox.height);
    uph_grid_lines(bbox.x, bbox.width, zoom_x, scroll_x, division, _uph_grid_draw_ruler_line, &style);
    naui_pop_clip_rect();
}

double uph_calculate_pattern_length(const Uph_MidiPattern *pattern)
{
    double length = uph_time_signature_bar_length();
    for (uint32_t i = 0; i < naui_list_len(pattern->notes); i++)
    {
        const double note_end = pattern->notes[i].start_beat + pattern->notes[i].length_beats;
        if (note_end > length)
            length = note_end;
    }
    return length;
}

double uph_calculate_automation_length(const Uph_Automation *automation)
{
    double length = uph_time_signature_bar_length();
    for (uint32_t i = 0; i < naui_list_len(automation->points); i++)
    {
        const double note_end = automation->points[i].beat;
        if (note_end > length)
            length = note_end;
    }
    return length;
}

void uph_automation_segment_controls(const Uph_AutomationPoint *from, float out[4])
{
    const float k = NAUI_CLAMP(from->tension, -1.0f, 1.0f);
    if (from->curve == UPH_AUTOMATION_CURVE_S)
    {
        if (k >= 0.0f)
        {
            out[0] = k;        out[1] = 0.0f;
            out[2] = 1.0f - k; out[3] = 1.0f;
        }
        else
        {
            out[0] = 0.0f; out[1] = -k;
            out[2] = 1.0f; out[3] = 1.0f + k;
        }
        return;
    }

    out[0] = out[2] = 0.5f - 0.5f * k;
    out[1] = out[3] = 0.5f + 0.5f * k;
}

float uph_automation_segment_value(const Uph_AutomationPoint *from, const Uph_AutomationPoint *to, double beat)
{
    const double span = to->beat - from->beat;
    if (span <= 0.0 || beat <= from->beat)
        return from->value;

    if (beat >= to->beat)
        return to->value;

    if (from->curve == UPH_AUTOMATION_CURVE_STEP)
        return from->value;

    float t = (float)((beat - from->beat) / span);
    if (from->tension != 0.0f)
    {
        float controls[4];
        uph_automation_segment_controls(from, controls);
        t = naui_cubic_bezier(controls[0], controls[1], controls[2], controls[3], t);
    }

    return from->value + (to->value - from->value) * t;
}

bool uph_automation_value_at_beat(const Uph_Automation *automation, double beat, float *out_value)
{
    const uint64_t count = naui_list_len(automation->points);
    if (count == 0)
        return false;

    if (beat <= automation->points[0].beat)
    {
        *out_value = automation->points[0].value;
        return true;
    }

    if (beat >= automation->points[count - 1].beat)
    {
        *out_value = automation->points[count - 1].value;
        return true;
    }

    for (uint64_t i = 0; i + 1 < count; i++)
    {
        const Uph_AutomationPoint *a = &automation->points[i];
        const Uph_AutomationPoint *b = &automation->points[i + 1];
        if (beat >= a->beat && beat < b->beat)
        {
            *out_value = uph_automation_segment_value(a, b, beat);
            return true;
        }
    }

    *out_value = automation->points[count - 1].value;
    return true;
}

float uph_linear_to_db(float linear)
{
    if (linear <= 0.0001f) return -80.0f;
    return 20.0f * log10f(linear);
}

float uph_db_to_linear(float db)
{
    return powf(10.0f, db / 20.0f);
}

float uph_automation_to_volume(float value)
{
    if (value <= 0.0f)
        return 0.0f;
    const float clamped = value > 1.0f ? 1.0f : value;
    return uph_db_to_linear(UPH_MIXER_DB_MIN + clamped * (UPH_MIXER_DB_MAX - UPH_MIXER_DB_MIN));
}

float uph_automation_to_pan(float value)
{
    const float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    return clamped * 2.0f - 1.0f;
}

#define UPH_LOOP_REGION_HANDLE_WIDTH 5.0f
#define UPH_LOOP_REGION_STRIP 0.4f

static float _uph_loop_region_x(float origin_x, float zoom_x, float scroll_x, double beat)
{
    return origin_x + (float)(beat * zoom_x) - scroll_x;
}

bool uph_loop_region_update(Uph_LoopRegion *region, Uph_LoopRegionDrag *drag, Leaf_BoundingBox bbox, float zoom_x, float scroll_x, double division, bool hovered)
{
    const float mouse_x = (float)naui_mouse_x();
    const float mouse_y = (float)naui_mouse_y();
    const bool over = hovered
        && mouse_x >= bbox.x && mouse_x < bbox.x + bbox.width
        && mouse_y >= bbox.y && mouse_y < bbox.y + bbox.height;
    const double beat = ((double)mouse_x - bbox.x + scroll_x) / zoom_x;
    const double snapped = fmax(0.0, round(beat / division) * division);

    if (drag->mode == UPH_LOOP_DRAG_NONE)
    {
        if (!over)
            return false;

        Uph_LoopRegionDragMode hover = UPH_LOOP_DRAG_NONE;
        if (uph_loop_region_valid(region))
        {
            const float start_x = _uph_loop_region_x((float)bbox.x, zoom_x, scroll_x, region->start);
            const float end_x = _uph_loop_region_x((float)bbox.x, zoom_x, scroll_x, region->end);
            const float handle = NAUI_DPI(UPH_LOOP_REGION_HANDLE_WIDTH);
            const bool in_strip = mouse_y < bbox.y + bbox.height * UPH_LOOP_REGION_STRIP;

            if (in_strip && fabsf(mouse_x - end_x) <= handle)
                hover = UPH_LOOP_DRAG_END;
            else if (in_strip && fabsf(mouse_x - start_x) <= handle)
                hover = UPH_LOOP_DRAG_START;
            else if (in_strip && mouse_x > start_x && mouse_x < end_x)
                hover = UPH_LOOP_DRAG_MOVE;
        }

        if (hover != UPH_LOOP_DRAG_NONE)
            naui_set_cursor(hover == UPH_LOOP_DRAG_MOVE ? NAUI_CURSOR_HAND : NAUI_CURSOR_RESIZE_EW);

        if (hover != UPH_LOOP_DRAG_NONE && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            drag->mode = hover;
            drag->anchor = beat - region->start;
        }
        else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
        {
            drag->mode = UPH_LOOP_DRAG_CREATE;
            drag->anchor = snapped;
            drag->moved = false;
        }

        if (drag->mode == UPH_LOOP_DRAG_NONE)
            return hover != UPH_LOOP_DRAG_NONE;
    }

    switch (drag->mode)
    {
        case UPH_LOOP_DRAG_CREATE:
        {
            if (snapped != drag->anchor)
                drag->moved = true;

            if (drag->moved)
            {
                region->start = fmin(drag->anchor, snapped);
                region->end = fmax(drag->anchor, snapped);
            }

            naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            if (!naui_mouse_down(NAUI_MOUSE_RIGHT))
            {
                if (!drag->moved || !uph_loop_region_valid(region))
                    *region = (Uph_LoopRegion){ 0 };
                else
                    drag->created = true;
                drag->mode = UPH_LOOP_DRAG_NONE;
            }
            return true;
        }
        case UPH_LOOP_DRAG_START:
            region->start = fmax(0.0, fmin(snapped, region->end - division));
            naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            break;
        case UPH_LOOP_DRAG_END:
            region->end = fmax(snapped, region->start + division);
            naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            break;
        case UPH_LOOP_DRAG_MOVE:
        {
            const double length = region->end - region->start;
            const double start = fmax(0.0, round((beat - drag->anchor) / division) * division);
            region->start = start;
            region->end = start + length;
            naui_set_cursor(NAUI_CURSOR_HAND);
            break;
        }
    }

    if (!naui_mouse_down(NAUI_MOUSE_LEFT))
        drag->mode = UPH_LOOP_DRAG_NONE;

    return true;
}

static Leaf_Color _uph_loop_region_color(bool enabled, uint8_t alpha)
{
    Leaf_Color color = naui_theme_color("uph_selection_outline_color");
    color.a = enabled ? alpha : (uint8_t)(alpha / 3);
    return color;
}

void uph_loop_region_render_ruler(const Uph_LoopRegion *region, Leaf_BoundingBox bbox, float zoom_x, float scroll_x, bool enabled)
{
    if (!uph_loop_region_valid(region))
        return;

    const float start_x = _uph_loop_region_x((float)bbox.x, zoom_x, scroll_x, region->start);
    const float end_x = _uph_loop_region_x((float)bbox.x, zoom_x, scroll_x, region->end);
    const float strip_height = bbox.height * UPH_LOOP_REGION_STRIP;

    naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);
    naui_fill_rect((Naui_Vec2){ start_x, bbox.y }, (Naui_Vec2){ end_x - start_x, bbox.height }, _uph_loop_region_color(enabled, 30), 0.0f, NAUI_CORNER_NONE);
    naui_fill_rect((Naui_Vec2){ start_x, bbox.y }, (Naui_Vec2){ end_x - start_x, strip_height }, _uph_loop_region_color(enabled, 170), 0.0f, NAUI_CORNER_NONE);
    naui_draw_line((Naui_Vec2){ start_x, bbox.y }, (Naui_Vec2){ start_x, bbox.y + bbox.height }, _uph_loop_region_color(enabled, 220), NAUI_DPI(1.0f));
    naui_draw_line((Naui_Vec2){ end_x, bbox.y }, (Naui_Vec2){ end_x, bbox.y + bbox.height }, _uph_loop_region_color(enabled, 220), NAUI_DPI(1.0f));
    naui_pop_clip_rect();
}

void uph_loop_region_render_bounds(const Uph_LoopRegion *region, float origin_x, float top, float bottom, float zoom_x, float scroll_x, bool enabled)
{
    if (!uph_loop_region_valid(region))
        return;

    const float start_x = _uph_loop_region_x(origin_x, zoom_x, scroll_x, region->start);
    const float end_x = _uph_loop_region_x(origin_x, zoom_x, scroll_x, region->end);
    const Leaf_Color color = _uph_loop_region_color(enabled, 110);

    naui_draw_line((Naui_Vec2){ start_x, top }, (Naui_Vec2){ start_x, bottom }, color, NAUI_DPI(1.0f));
    naui_draw_line((Naui_Vec2){ end_x, top }, (Naui_Vec2){ end_x, bottom }, color, NAUI_DPI(1.0f));
}
