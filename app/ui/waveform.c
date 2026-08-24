void uph_ui_waveform_zoomable(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Naui_Color color,
    double zoom,
    double start_offset,
    Leaf_BoundingBox visible_bbox,
    Uph_Sample* sample
) {
    if (!uph_state.project.sample_data)
        return;

    const Uph_SampleData* sample_data = &uph_state.project.sample_data[sample->data_index];
    if (!sample_data || !sample_data->frames || sample_data->frame_count == 0)
        return;

    const uint32_t peak_count = (uint32_t)naui_list_len(sample_data->waveform_peaks);
    if (peak_count == 0)
        return;

    const double stretch_scale = (sample->stretch_scale > 0.0) ? sample->stretch_scale : 1.0;
    const float bpm = uph_state.project.bpm;
    if (bpm <= 0.0f)
        return;

    const uint32_t sample_rate = uph_state.settings.audio.sample_rate;
    const float mid_y = position.y + size.y * 0.5f;
    const float half_height = size.y * 0.5f;

    int32_t x_start = 0;
    int32_t x_end = (int32_t)size.x;
    if (x_end <= x_start)
        return;

    const int32_t visible_left  = (int32_t)floorf(visible_bbox.x - position.x) - 1;
    const int32_t visible_right = (int32_t)ceilf(visible_bbox.x + visible_bbox.width - position.x) + 1;

    x_start = NAUI_MAX(x_start, visible_left);
    x_end = NAUI_MIN(x_end, visible_right);

    if (x_end <= x_start)
        return;

    float *max_y = (float*)alloca(sizeof(float) * (size_t)(x_end - x_start));
    float *min_y = (float*)alloca(sizeof(float) * (size_t)(x_end - x_start));

    for (int32_t x = x_start; x < x_end; x++)
    {
        double normalized_zoom = (double)(x) / zoom;
        double source_beats = normalized_zoom / stretch_scale + start_offset;
        double source_seconds = uph_beats_to_seconds(source_beats, bpm);
        double source_frame_pos = source_seconds * (double)sample_rate;

        double bin_pos = source_frame_pos / (double)UPH_SAMPLE_FRAME_COUNT;

        int64_t peak_index = (int64_t)bin_pos;
        double frac = bin_pos - (double)peak_index;

        if (peak_index < 0)
        {
            peak_index = 0;
            frac = 0.0;
        }
        if (peak_index >= (int64_t)peak_count)
        {
            peak_index = (int64_t)peak_count - 1;
            frac = 0.0;
        }

        int64_t next_index = peak_index + 1;
        if (next_index >= (int64_t)peak_count)
            next_index = peak_index;

        Uph_WaveformPeak p0 = sample_data->waveform_peaks[peak_index];
        Uph_WaveformPeak p1 = sample_data->waveform_peaks[next_index];

        float p0_max = uph_waveform_decode_uint16(p0.max);
        float p0_min = uph_waveform_decode_uint16(p0.min);
        float p1_max = uph_waveform_decode_uint16(p1.max);
        float p1_min = uph_waveform_decode_uint16(p1.min);

        float max_v = p0_max + (p1_max - p0_max) * (float)frac;
        float min_v = p0_min + (p1_min - p0_min) * (float)frac;

        max_y[x - x_start] = mid_y - max_v * half_height;
        min_y[x - x_start] = mid_y - min_v * half_height;
    }

    for (int32_t x = x_start; x < x_end - 1; x++)
    {
        int32_t i0 = x - x_start;
        int32_t i1 = i0 + 1;

        Naui_Vec2 quad[4] = {
            (Naui_Vec2) { position.x + (float)x,     max_y[i0] },
            (Naui_Vec2) { position.x + (float)(x+1), max_y[i1] },
            (Naui_Vec2) { position.x + (float)(x+1), min_y[i1] },
            (Naui_Vec2) { position.x + (float)x,     min_y[i0] },
        };

        naui_fill_polygon(quad, 4, color);
    }
}