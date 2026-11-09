void uph_ui_waveform_zoomable(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Naui_Color color,
    double zoom,
    double start_offset,
    Leaf_BoundingBox visible_bbox,
    Uph_Sample* sample,
    const Uph_TimelineBlock* block
);

void uph_ui_waveform_peaks(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Naui_Color color,
    const Uph_WaveformPeak *peaks,
    uint64_t peak_count,
    uint64_t peak_total,
    float gain
);
