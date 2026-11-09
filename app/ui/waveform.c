void uph_ui_waveform_zoomable(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Naui_Color color,
    double zoom,
    double start_offset,
    Leaf_BoundingBox visible_bbox,
    Uph_Sample* sample,
    const Uph_TimelineBlock* block
) {
    if (!uph_state.project.sample_data)
        return;

    const Uph_SampleData* sample_data = &uph_state.project.sample_data[sample->data_index];
    if (!sample_data || !sample_data->frames || sample_data->frame_count == 0)
        return;

    const uint32_t peak_count = (uint32_t)naui_list_len(sample_data->waveform_peaks);
    if (peak_count == 0)
        return;

    const float bpm = uph_state.project.bpm;
    if (bpm <= 0.0f)
        return;

    const double time_scale = uph_sample_effective_time_scale(sample, (double)bpm);

    const uint32_t sample_rate = uph_state.settings.audio.sample_rate;
    const float mid_y = position.y + size.y * 0.5f;
    const float half_height = size.y * 0.5f;
    const float gain = uph_sample_gain(sample, sample_data);

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

    const uint32_t max_frame_count = UPH_WAVEFORM_BIN_FRAMES;

    for (int32_t x = x_start; x < x_end; x++)
    {
        double normalized_zoom = (double)(x) / zoom;
        double source_seconds;
        if (block)
        {
            source_seconds = uph_sample_block_source_seconds(sample, block->start_beat, start_offset, block->start_beat + normalized_zoom);
        }
        else
        {
            double source_beats = normalized_zoom / time_scale + start_offset;
            source_seconds = uph_beats_to_seconds(source_beats, bpm);
        }
        double source_frame_pos = source_seconds * (double)sample_rate;

        if (source_frame_pos >= (double)sample_data->frame_count)
        {
            x_end = x;
            break;
        }

        double bin_pos = source_frame_pos / (double)max_frame_count;

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

        float max_v = NAUI_CLAMP((p0_max + (p1_max - p0_max) * (float)frac) * gain, -1.0f, 1.0f);
        float min_v = NAUI_CLAMP((p0_min + (p1_min - p0_min) * (float)frac) * gain, -1.0f, 1.0f);

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

void uph_ui_waveform_peaks(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Naui_Color color,
    const Uph_WaveformPeak *peaks,
    uint64_t peak_count,
    uint64_t peak_total,
    float gain
) {
    if (!peaks || peak_count == 0 || peak_total == 0 || size.x < 2.0f || size.y <= 0.0f)
        return;

    if (peak_count > peak_total)
        peak_count = peak_total;

    const int32_t columns = (int32_t)ceilf(size.x) + 1;
    const float mid_y = position.y + size.y * 0.5f;
    const float half_height = size.y * 0.5f;
    const double bins_per_pixel = (double)peak_total / (double)size.x;

    float *max_y = (float*)alloca(sizeof(float) * (size_t)columns);
    float *min_y = (float*)alloca(sizeof(float) * (size_t)columns);

    int32_t column_count = 0;
    for (int32_t x = 0; x < columns; x++)
    {
        float max_v;
        float min_v;

        if (bins_per_pixel > 1.0)
        {
            const uint64_t begin = (uint64_t)((double)x * bins_per_pixel);
            uint64_t end = (uint64_t)((double)(x + 1) * bins_per_pixel);
            if (begin >= peak_count)
                break;
            if (end <= begin)
                end = begin + 1;
            if (end > peak_count)
                end = peak_count;

            max_v = -1.0f;
            min_v = 1.0f;
            for (uint64_t b = begin; b < end; b++)
            {
                max_v = fmaxf(max_v, uph_waveform_decode_uint16(peaks[b].max));
                min_v = fminf(min_v, uph_waveform_decode_uint16(peaks[b].min));
            }
        }
        else
        {
            double bin_pos = (double)x * bins_per_pixel;
            if (bin_pos >= (double)peak_count)
                break;

            const uint64_t index = (uint64_t)bin_pos;
            const uint64_t next = index + 1 < peak_count ? index + 1 : index;
            const float frac = (float)(bin_pos - (double)index);

            const float p0_max = uph_waveform_decode_uint16(peaks[index].max);
            const float p0_min = uph_waveform_decode_uint16(peaks[index].min);
            const float p1_max = uph_waveform_decode_uint16(peaks[next].max);
            const float p1_min = uph_waveform_decode_uint16(peaks[next].min);

            max_v = p0_max + (p1_max - p0_max) * frac;
            min_v = p0_min + (p1_min - p0_min) * frac;
        }

        max_v = NAUI_CLAMP(max_v * gain, -1.0f, 1.0f);
        min_v = NAUI_CLAMP(min_v * gain, -1.0f, 1.0f);

        float top = mid_y - max_v * half_height;
        float bottom = mid_y - min_v * half_height;
        if (bottom - top < 1.0f)
        {
            const float center = (top + bottom) * 0.5f;
            top = center - 0.5f;
            bottom = center + 0.5f;
        }

        max_y[column_count] = top;
        min_y[column_count] = bottom;
        column_count++;
    }

    for (int32_t i = 0; i + 1 < column_count; i++)
    {
        const float x0 = position.x + (float)i;
        const float x1 = fminf(position.x + (float)(i + 1), position.x + size.x);

        Naui_Vec2 quad[4] = {
            (Naui_Vec2) { x0, max_y[i] },
            (Naui_Vec2) { x1, max_y[i + 1] },
            (Naui_Vec2) { x1, min_y[i + 1] },
            (Naui_Vec2) { x0, min_y[i] },
        };

        naui_fill_polygon(quad, 4, color);
    }
}
