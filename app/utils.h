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

static inline double uph_snap_beat(double beat, Uph_SnapResolution resolution)
{
    const double division = uph_snap_division(resolution);
    return round(beat / division) * division;
}

double uph_calculate_pattern_length(const Uph_MidiPattern *pattern);
double uph_calculate_automation_length(const Uph_Automation *automation);