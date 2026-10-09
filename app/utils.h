typedef enum
{
    UPH_SNAP_BEAT,
    UPH_SNAP_HALF,
    UPH_SNAP_QUARTER,
    UPH_SNAP_EIGHTH,
    UPH_SNAP_SIXTEENTH
}
Uph_SnapResolution;

static inline double uph_snap_division(Uph_SnapResolution res)
{
    switch (res)
    {
    case UPH_SNAP_BEAT:      return 1.0;
    case UPH_SNAP_HALF:      return 0.5;
    case UPH_SNAP_QUARTER:   return 0.25;
    case UPH_SNAP_EIGHTH:    return 0.125;
    case UPH_SNAP_SIXTEENTH: return 0.0625;
    }
    return 1.0;
}

static inline double uph_snap_beat_round(double beat, Uph_SnapResolution resolution)
{
    const double division = uph_snap_division(resolution);
    return round(beat / division) * division;
}

static inline double uph_snap_beat_floor(double beat, Uph_SnapResolution resolution)
{
    const double division = uph_snap_division(resolution);
    return floor(beat / division) * division;
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

double uph_calculate_pattern_length(const Uph_MidiPattern *pattern);
double uph_calculate_automation_length(const Uph_Automation *automation);