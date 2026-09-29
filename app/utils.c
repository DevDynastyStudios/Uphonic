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