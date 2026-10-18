double uph_calculate_pattern_length(const Uph_MidiPattern *pattern)
{
    double length = (double)uph_state.project.time_signature.denominator; // minimum size
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
    double length = (double)uph_state.project.time_signature.denominator; // minimum size
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
