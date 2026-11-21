#define UPH_GRID_MIN_SUB_SPACING 8
#define UPH_GRID_MIN_BEAT_SPACING 5
#define UPH_SNAP_LINE_MIN_DIVISION 0.25

typedef enum
{
	UPH_SNAP_BEAT,
	UPH_SNAP_HALF,
	UPH_SNAP_QUARTER,
	UPH_SNAP_EIGHTH,
	UPH_SNAP_SIXTEENTH,
	UPH_SNAP_LINE
}
Uph_SnapResolution;

typedef uint8_t Uph_LoopRegionDragMode;
enum
{
	UPH_LOOP_DRAG_NONE,
	UPH_LOOP_DRAG_CREATE,
	UPH_LOOP_DRAG_START,
	UPH_LOOP_DRAG_END,
	UPH_LOOP_DRAG_MOVE
};

typedef struct
{
	double anchor;
	Uph_LoopRegionDragMode mode;
	bool moved;
	bool created;
}
Uph_LoopRegionDrag;

typedef enum
{
	UPH_GRID_LINE_SUBDIVISION,
	UPH_GRID_LINE_BEAT,
	UPH_GRID_LINE_BAR
}
Uph_GridLineKind;

typedef void (*Uph_GridLineFn)(float x, Uph_GridLineKind kind, int64_t bar_index, void *user_data);

static inline double uph_time_signature_beat_length(void)
{
	const uint32_t denominator = uph_state.project.time_signature.denominator;
	return denominator > 0 ? 4.0 / (double)denominator : 1.0;
}

static inline double uph_time_signature_bar_length(void)
{
	const uint32_t numerator = uph_state.project.time_signature.numerator;
	return (double)(numerator > 0 ? numerator : 1) * uph_time_signature_beat_length();
}

static inline Leaf_Color uph_color_mix(Leaf_Color a, Leaf_Color b, float t)
{
	t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	return (Leaf_Color) {
		(uint8_t)((float)a.r + ((float)b.r - (float)a.r) * t + 0.5f),
		(uint8_t)((float)a.g + ((float)b.g - (float)a.g) * t + 0.5f),
		(uint8_t)((float)a.b + ((float)b.b - (float)a.b) * t + 0.5f),
		(uint8_t)((float)a.a + ((float)b.a - (float)a.a) * t + 0.5f)
	};
}

static inline double uph_snap_line_division(float zoom_x)
{
	const double beat_length = uph_time_signature_beat_length();
	if (zoom_x <= 0.0f)
		return beat_length;

	if (beat_length * zoom_x < NAUI_DPI(UPH_GRID_MIN_BEAT_SPACING))
		return uph_time_signature_bar_length();

	double division = beat_length;
	while (division * 0.5 >= UPH_SNAP_LINE_MIN_DIVISION && division * 0.5 * zoom_x >= NAUI_DPI(UPH_GRID_MIN_SUB_SPACING))
		division *= 0.5;

	return division;
}

static inline double uph_snap_division(Uph_SnapResolution res, float zoom_x)
{
	switch (res)
	{
		case UPH_SNAP_BEAT:      return uph_time_signature_beat_length();
		case UPH_SNAP_HALF:      return 0.5;
		case UPH_SNAP_QUARTER:   return 0.25;
		case UPH_SNAP_EIGHTH:    return 0.125;
		case UPH_SNAP_SIXTEENTH: return 0.0625;
		case UPH_SNAP_LINE:      return uph_snap_line_division(zoom_x);
	}

	return 1.0;
}

static inline double uph_snap_beat_round(double beat, Uph_SnapResolution resolution, float zoom_x)
{
	const double division = uph_snap_division(resolution, zoom_x);
	return round(beat / division) * division;
}

static inline double uph_snap_beat_floor(double beat, Uph_SnapResolution resolution, float zoom_x)
{
	const double division = uph_snap_division(resolution, zoom_x);
	return floor(beat / division) * division;
}

static inline bool uph_loop_region_valid(const Uph_LoopRegion *region)
{
	return region->end > region->start;
}

static void uph__grid_lines(float origin_x, float width, float zoom_x, float scroll_x, double division, Uph_GridLineFn fn, void *user_data)
{
    const double beat_length = uph_time_signature_beat_length();
    const int64_t beats_per_bar = (int64_t)llround(uph_time_signature_bar_length() / beat_length);
    const int32_t min_sub_spacing = NAUI_DPI(UPH_GRID_MIN_SUB_SPACING);
    const int32_t min_beat_spacing = NAUI_DPI(UPH_GRID_MIN_BEAT_SPACING);

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
    Leaf_Color sub_color;
    Leaf_Color beat_color;
    Leaf_Color bar_color;
    Leaf_Color number_color;
}
Uph_GridColors;

typedef struct
{
    Leaf_BoundingBox bbox;
    Uph_GridColors colors;
}
Uph_GridStyle;

static void uph__grid_draw_beat_line(float x, Uph_GridLineKind kind, int64_t bar_index, void *user_data)
{
    const Uph_GridStyle *style = user_data;
    if (x < style->bbox.x || x > style->bbox.x + style->bbox.width)
        return;

    const Leaf_Color color = kind == UPH_GRID_LINE_BAR ? style->colors.bar_color : kind == UPH_GRID_LINE_BEAT ? style->colors.beat_color : style->colors.sub_color;
    naui_draw_line((Naui_Vec2){x, style->bbox.y}, (Naui_Vec2){x, style->bbox.y + style->bbox.height}, color, 1.0f);
}

static void uph__grid_draw_ruler_line(float x, Uph_GridLineKind kind, int64_t bar_index, void *user_data)
{
    const Uph_GridStyle *style = user_data;
    const Leaf_BoundingBox bbox = style->bbox;
    if (x > bbox.x + bbox.width)
        return;

    if (kind == UPH_GRID_LINE_SUBDIVISION)
    {
        naui_draw_line((Naui_Vec2){x, bbox.y + bbox.height * 0.6f}, (Naui_Vec2){x, bbox.y + bbox.height}, style->colors.sub_color, 1.0f);
        return;
    }

    const bool is_bar = kind == UPH_GRID_LINE_BAR;
    naui_draw_line(
        (Naui_Vec2){x, bbox.y + bbox.height * (is_bar ? 0.4f : 0.6f)},
        (Naui_Vec2){x, bbox.y + bbox.height},
        is_bar ? style->colors.bar_color : style->colors.beat_color,
        1.0f
    );

    if (is_bar)
    {
        char label[16];
        snprintf(label, sizeof(label), "%d", (int)bar_index);
        naui_draw_text((Naui_Vec2){x + 4.0f, bbox.y + bbox.height * 0.4f}, label, NAUI_DPI(13), style->colors.number_color);
    }
}

void uph_grid_render_beats(Leaf_BoundingBox bbox, float zoom_x, float scroll_x, double division, Uph_GridColors colors)
{
    Uph_GridStyle style = { .bbox = bbox, .colors = colors };

    uph__grid_lines(bbox.x, bbox.width, zoom_x, scroll_x, division, uph__grid_draw_beat_line, &style);
}

void uph_grid_render_ruler(Leaf_BoundingBox bbox, float zoom_x, float scroll_x, double division, Uph_GridColors colors)
{
    Uph_GridStyle style = { .bbox = bbox, .colors = colors };

    naui_push_clip_rect(bbox.x - 0.5f, bbox.y, bbox.width, bbox.height);
    uph__grid_lines(bbox.x, bbox.width, zoom_x, scroll_x, division, uph__grid_draw_ruler_line, &style);
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
    for (uint32_t i = 0; i < naui_list_len(pattern->pedals); i++)
    {
        if (pattern->pedals[i].beat > length)
            length = pattern->pedals[i].beat;
    }
    return length;
}

uint8_t uph_midi_pedal_cc(Uph_MidiPedalType type)
{
    switch (type)
    {
        case UPH_MIDI_PEDAL_SOSTENUTO: return 66;
        case UPH_MIDI_PEDAL_SOFT: return 67;
        default: return 64;
    }
}

bool uph_midi_pedal_from_cc(uint8_t cc, Uph_MidiPedalType *out_type)
{
    for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
    {
        if (uph_midi_pedal_cc(type) == cc)
        {
            *out_type = type;
            return true;
        }
    }
    return false;
}

static int uph__midi_pedal_compare(const void *a, const void *b)
{
    const Uph_MidiPedalEvent *left = a;
    const Uph_MidiPedalEvent *right = b;
    if (left->beat != right->beat)
        return left->beat < right->beat ? -1 : 1;
    return (int)left->type - (int)right->type;
}

void uph_midi_pedals_sort(Naui_List(Uph_MidiPedalEvent) pedals)
{
    const size_t count = naui_list_len(pedals);
    for (size_t i = 1; i < count; i++)
    {
        if (uph__midi_pedal_compare(&pedals[i - 1], &pedals[i]) > 0)
        {
            qsort(pedals, count, sizeof(Uph_MidiPedalEvent), uph__midi_pedal_compare);
            return;
        }
    }
}

uint8_t uph_midi_pedal_value_before(const Uph_MidiPattern *pattern, Uph_MidiPedalType type, double beat, bool inclusive)
{
    uint8_t value = 0;
    for (uint32_t i = 0; i < naui_list_len(pattern->pedals); i++)
    {
        const Uph_MidiPedalEvent *pedal = &pattern->pedals[i];
        if (pedal->beat > beat || (!inclusive && pedal->beat == beat))
            break;
        if (pedal->type == type)
            value = pedal->value;
    }
    return value;
}

void uph_midi_pattern_free_events(Uph_MidiPattern *pattern)
{
    naui_list_free(pattern->notes);
    naui_list_free(pattern->pedals);
    pattern->notes = NULL;
    pattern->pedals = NULL;
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
    if (linear <= 0.0f) return UPH_MIXER_DB_MIN;
    const float db = 20.0f * log10f(linear);
    return db < UPH_MIXER_DB_MIN ? UPH_MIXER_DB_MIN : db;
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

float uph_automation_to_bpm(float value)
{
    const float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    return UPH_TEMPO_MIN_BPM + clamped * (UPH_TEMPO_MAX_BPM - UPH_TEMPO_MIN_BPM);
}

#define UPH_LOOP_REGION_HANDLE_WIDTH 5
#define UPH_LOOP_REGION_STRIP 0.4f

static float uph__loop_region_x(float origin_x, float zoom_x, float scroll_x, double beat)
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
            const float start_x = uph__loop_region_x((float)bbox.x, zoom_x, scroll_x, region->start);
            const float end_x = uph__loop_region_x((float)bbox.x, zoom_x, scroll_x, region->end);
            const int32_t handle = NAUI_DPI(UPH_LOOP_REGION_HANDLE_WIDTH);
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

static Leaf_Color uph__loop_region_color(Leaf_Color color, bool enabled, uint8_t alpha)
{
    color.a = enabled ? alpha : (uint8_t)(alpha / 3);
    return color;
}

void uph_loop_region_render_ruler(const Uph_LoopRegion *region, Leaf_BoundingBox bbox, float zoom_x, float scroll_x, bool enabled, Leaf_Color outline_color)
{
    if (!uph_loop_region_valid(region))
        return;

    const float start_x = uph__loop_region_x((float)bbox.x, zoom_x, scroll_x, region->start);
    const float end_x = uph__loop_region_x((float)bbox.x, zoom_x, scroll_x, region->end);
    const float strip_height = bbox.height * UPH_LOOP_REGION_STRIP;

    naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);
    naui_fill_rect((Naui_Vec2){ start_x, bbox.y }, (Naui_Vec2){ end_x - start_x, bbox.height }, uph__loop_region_color(outline_color, enabled, 30), 0.0f, NAUI_CORNER_NONE);
    naui_fill_rect((Naui_Vec2){ start_x, bbox.y }, (Naui_Vec2){ end_x - start_x, strip_height }, uph__loop_region_color(outline_color, enabled, 170), 0.0f, NAUI_CORNER_NONE);
    naui_draw_line((Naui_Vec2){ start_x, bbox.y }, (Naui_Vec2){ start_x, bbox.y + bbox.height }, uph__loop_region_color(outline_color, enabled, 220), NAUI_DPI(1));
    naui_draw_line((Naui_Vec2){ end_x, bbox.y }, (Naui_Vec2){ end_x, bbox.y + bbox.height }, uph__loop_region_color(outline_color, enabled, 220), NAUI_DPI(1));
    naui_pop_clip_rect();
}

void uph_loop_region_render_bounds(const Uph_LoopRegion *region, float origin_x, float top, float bottom, float zoom_x, float scroll_x, bool enabled, Leaf_Color outline_color)
{
    if (!uph_loop_region_valid(region))
        return;

    const float start_x = uph__loop_region_x(origin_x, zoom_x, scroll_x, region->start);
    const float end_x = uph__loop_region_x(origin_x, zoom_x, scroll_x, region->end);
    const Leaf_Color color = uph__loop_region_color(outline_color, enabled, 110);

    naui_draw_line((Naui_Vec2){ start_x, top }, (Naui_Vec2){ start_x, bottom }, color, NAUI_DPI(1));
    naui_draw_line((Naui_Vec2){ end_x, top }, (Naui_Vec2){ end_x, bottom }, color, NAUI_DPI(1));
}

static inline bool uph_ui_ctrl_down(void)
{
    return naui_key_down(NAUI_KEY_CONTROL) || naui_key_down(NAUI_KEY_LCONTROL) || naui_key_down(NAUI_KEY_RCONTROL);
}

static inline bool uph_ui_shift_down(void)
{
    return naui_key_down(NAUI_KEY_SHIFT) || naui_key_down(NAUI_KEY_LSHIFT) || naui_key_down(NAUI_KEY_RSHIFT);
}
