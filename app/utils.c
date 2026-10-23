void uph_grid_lines(float origin_x, float width, float zoom_x, float scroll_x, double division, Uph_GridLineFn fn, void *user_data)
{
    const double beat_length = uph_time_signature_beat_length();
    const int64_t beats_per_bar = (int64_t)llround(uph_time_signature_bar_length() / beat_length);
    const float min_spacing = NAUI_DPI(3.0f);

    const float division_px = (float)(division * zoom_x);
    if (division < beat_length && division_px >= min_spacing)
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

    const bool draw_beats = beat_length * zoom_x >= min_spacing;
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
