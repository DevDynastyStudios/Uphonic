#ifndef UPH_INVALID_TIMELINE_BLOCK
#define UPH_INVALID_TIMELINE_BLOCK NULL
#endif

typedef struct
{
    ma_context context;
    bool context_ready;
    ma_device device;
}
Uph_AudioEngineData;

static Uph_AudioEngineData uph_audio_engine_data;

static void uph_read_sample_frame(const Uph_SampleData *sample, double pos, float *out_left, float *out_right)
{
    if (pos < 0.0 || pos >= (double)sample->frame_count || !sample->frames)
    {
        *out_left = 0.0f;
        *out_right = 0.0f;
        return;
    }

    const float *src = (const float*)sample->frames;
    const int src_channels = (sample->channel_type == UPH_SAMPLE_STEREO) ? 2 : 1;

    uint64_t frame0 = (uint64_t)pos;
    uint64_t frame1 = frame0 + 1;
    float frac = (float)(pos - (double)frame0);
    bool has_frame1 = frame1 < sample->frame_count;

    if (src_channels == 1)
    {
        float s0 = src[frame0];
        float s1 = has_frame1 ? src[frame1] : s0;
        float s = s0 + (s1 - s0) * frac;
        *out_left = *out_right = s;
    }
    else
    {
        float l0 = src[frame0 * 2 + 0];
        float r0 = src[frame0 * 2 + 1];
        float l1 = has_frame1 ? src[frame1 * 2 + 0] : l0;
        float r1 = has_frame1 ? src[frame1 * 2 + 1] : r0;
        *out_left  = l0 + (l1 - l0) * frac;
        *out_right = r0 + (r1 - r0) * frac;
    }
}

#define UPH_WSOLA_GRAIN_FRAMES  2048
#define UPH_WSOLA_HOP_FRAMES    (UPH_WSOLA_GRAIN_FRAMES / 2)
#define UPH_WSOLA_TOLERANCE     512
#define UPH_WSOLA_COARSE_STEP   4
#define UPH_WSOLA_RING_FRAMES   4096
#define UPH_WSOLA_RING_MASK     (UPH_WSOLA_RING_FRAMES - 1)
#define UPH_WSOLA_MAX_STATES    32

#define UPH_WSOLA_COARSE_REF    (UPH_WSOLA_HOP_FRAMES / UPH_WSOLA_COARSE_STEP)
#define UPH_WSOLA_COARSE_CANDS  (2 * UPH_WSOLA_TOLERANCE / UPH_WSOLA_COARSE_STEP + 1)

static float uph_stretch_window[UPH_WSOLA_GRAIN_FRAMES];

static void uph_stretch_init_window(void)
{
    for (uint32_t i = 0; i < UPH_WSOLA_GRAIN_FRAMES; i++)
    {
        uph_stretch_window[i] = 0.5f - 0.5f * cosf(
            2.0f * (float)NAUI_PI * (float)i / (float)UPH_WSOLA_GRAIN_FRAMES);
    }
}

typedef struct
{
    const void *key;
    uint64_t    last_used;
    bool        valid;

    const void *frames;
    uint64_t    frame_count;
    double      start_offset_frames;
    double      beta;
    double      read_rate;

    double      next_out_pos;
    double      q;

    int64_t     next_grain;
    int64_t     complete_up_to;
    int64_t     prev_start;
    bool        has_prev;

    float       ring_l[UPH_WSOLA_RING_FRAMES];
    float       ring_r[UPH_WSOLA_RING_FRAMES];
}
Uph_Stretcher;

static Uph_Stretcher uph_stretchers[UPH_WSOLA_MAX_STATES];
static uint64_t      uph_stretcher_clock;

static inline float uph_sample_mono_at(const Uph_SampleData *s, int64_t i)
{
    if (i < 0 || (uint64_t)i >= s->frame_count || !s->frames)
        return 0.0f;

    const float *src = (const float*)s->frames;
    if (s->channel_type == UPH_SAMPLE_STEREO)
        return 0.5f * (src[i * 2 + 0] + src[i * 2 + 1]);
    return src[i];
}

static inline void uph_sample_stereo_at(const Uph_SampleData *s, int64_t i, float *l, float *r)
{
    if (i < 0 || (uint64_t)i >= s->frame_count || !s->frames)
    {
        *l = 0.0f;
        *r = 0.0f;
        return;
    }

    const float *src = (const float*)s->frames;
    if (s->channel_type == UPH_SAMPLE_STEREO)
    {
        *l = src[i * 2 + 0];
        *r = src[i * 2 + 1];
    }
    else
    {
        *l = *r = src[i];
    }
}

static inline float uph_hermite4(float ym1, float y0, float y1, float y2, float t)
{
    const float c1 = 0.5f * (y1 - ym1);
    const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return ((c3 * t + c2) * t + c1) * t + y0;
}

static int64_t uph_wsola_find_start(const Uph_SampleData *s, int64_t ref_start, int64_t nominal, int64_t min_start)
{
    enum
    {
        STEP   = UPH_WSOLA_COARSE_STEP,
        REF_N  = UPH_WSOLA_COARSE_REF,
        CANDS  = UPH_WSOLA_COARSE_CANDS,
        TOL    = UPH_WSOLA_TOLERANCE,
        HOP    = UPH_WSOLA_HOP_FRAMES,
    };

    float dref[REF_N];
    float d[CANDS + REF_N - 1];

    const int64_t base = nominal - TOL;

    for (int m = 0; m < REF_N; m++)
    {
        float acc = 0.0f;
        for (int u = 0; u < STEP; u++)
            acc += uph_sample_mono_at(s, ref_start + (int64_t)m * STEP + u);
        dref[m] = acc;
    }
    for (int m = 0; m < CANDS + REF_N - 1; m++)
    {
        float acc = 0.0f;
        for (int u = 0; u < STEP; u++)
            acc += uph_sample_mono_at(s, base + (int64_t)m * STEP + u);
        d[m] = acc;
    }

    const int center_j = CANDS / 2;
    int   best_j = center_j;
    float best_score = -1e30f;

    float energy = 0.0f;
    for (int m = 0; m < REF_N; m++)
        energy += d[m] * d[m];

    for (int j = 0; j < CANDS; j++)
    {
        if (j > 0)
            energy += d[j + REF_N - 1] * d[j + REF_N - 1] - d[j - 1] * d[j - 1];

        if (base + (int64_t)j * STEP < min_start)
            continue;

        float dot = 0.0f;
        for (int m = 0; m < REF_N; m++)
            dot += dref[m] * d[j + m];

        float score = dot / sqrtf((energy > 0.0f ? energy : 0.0f) + 1e-6f);

        if (score > best_score
            || (score == best_score && abs(j - center_j) < abs(best_j - center_j)))
        {
            best_score = score;
            best_j = j;
        }
    }

    const int64_t coarse_best = base + (int64_t)best_j * STEP;

    float ref[HOP];
    for (int i = 0; i < HOP; i++)
        ref[i] = uph_sample_mono_at(s, ref_start + i);

    int64_t best = coarse_best;
    best_score = -1e30f;

    for (int64_t a = coarse_best - STEP; a <= coarse_best + STEP; a++)
    {
        if (a < min_start)
            continue;

        float dot = 0.0f;
        float en  = 0.0f;
        for (int i = 0; i < HOP; i += 2)
        {
            float c = uph_sample_mono_at(s, a + i);
            dot += ref[i] * c;
            en  += c * c;
        }

        float score = dot / sqrtf(en + 1e-6f);
        if (score > best_score)
        {
            best_score = score;
            best = a;
        }
    }

    return best;
}

static void uph_wsola_add_grain(Uph_Stretcher *st, const Uph_SampleData *sample, int64_t k)
{
    const int64_t hop   = UPH_WSOLA_HOP_FRAMES;
    const int64_t grain = UPH_WSOLA_GRAIN_FRAMES;

    const int64_t start = (int64_t)llround(st->start_offset_frames);

    int64_t nominal = start;
    if (k > 0)
        nominal = start + (int64_t)llround((double)(k + 1) * (double)hop * st->beta) - hop;
    if (nominal < start)
        nominal = start;

    int64_t a = nominal;
    if (st->has_prev)
        a = uph_wsola_find_start(sample, st->prev_start + hop, nominal, start);

    const int64_t y0 = k * hop;

    for (int64_t i = hop; i < grain; i++)
    {
        const int64_t idx = (y0 + i) & UPH_WSOLA_RING_MASK;
        st->ring_l[idx] = 0.0f;
        st->ring_r[idx] = 0.0f;
    }

    for (int64_t i = 0; i < grain; i++)
    {
        const float w = (k == 0 && i < hop) ? 1.0f : uph_stretch_window[i];

        float l, r;
        uph_sample_stereo_at(sample, a + i, &l, &r);

        const int64_t idx = (y0 + i) & UPH_WSOLA_RING_MASK;
        st->ring_l[idx] += l * w;
        st->ring_r[idx] += r * w;
    }

    st->prev_start     = a;
    st->has_prev       = true;
    st->next_grain     = k + 1;
    st->complete_up_to = (k + 1) * hop;
}

static void uph_stretcher_reset(Uph_Stretcher *st, double out_pos)
{
    memset(st->ring_l, 0, sizeof(st->ring_l));
    memset(st->ring_r, 0, sizeof(st->ring_r));

    st->q            = out_pos * st->read_rate;
    st->next_out_pos = out_pos;

    int64_t k = (int64_t)floor(st->q / (double)UPH_WSOLA_HOP_FRAMES) - 2;
    if (k < 0)
        k = 0;

    st->next_grain     = k;
    st->complete_up_to = k * UPH_WSOLA_HOP_FRAMES;
    st->prev_start     = 0;
    st->has_prev       = false;
}

static Uph_Stretcher *uph_stretcher_acquire(const void *key)
{
    Uph_Stretcher *oldest = &uph_stretchers[0];

    for (int i = 0; i < UPH_WSOLA_MAX_STATES; i++)
    {
        Uph_Stretcher *st = &uph_stretchers[i];
        if (st->key == key)
        {
            st->last_used = ++uph_stretcher_clock;
            return st;
        }
        if (st->last_used < oldest->last_used)
            oldest = st;
    }

    oldest->key       = key;
    oldest->valid     = false;
    oldest->last_used = ++uph_stretcher_clock;
    return oldest;
}

static void uph_stretcher_prepare(
    Uph_Stretcher *st,
    const Uph_SampleData *data,
    double start_offset_frames,
    double beta,
    double read_rate,
    double out_pos
)
{
    const bool same = st->valid
        && st->frames == data->frames
        && st->frame_count == data->frame_count
        && st->start_offset_frames == start_offset_frames
        && st->beta == beta
        && st->read_rate == read_rate
        && fabs(st->next_out_pos - out_pos) < 1.5;

    if (same)
        return;

    st->valid               = true;
    st->frames              = data->frames;
    st->frame_count         = data->frame_count;
    st->start_offset_frames = start_offset_frames;
    st->beta                = beta;
    st->read_rate           = read_rate;

    uph_stretcher_reset(st, out_pos);
}

static void uph_stretcher_read(Uph_Stretcher *st, const Uph_SampleData *sample, float *out_left, float *out_right)
{
    const int64_t i0 = (int64_t)floor(st->q);
    const float   t  = (float)(st->q - (double)i0);

    while (st->complete_up_to < i0 + 3)
        uph_wsola_add_grain(st, sample, st->next_grain);

    float l[4], r[4];
    for (int n = 0; n < 4; n++)
    {
        const int64_t pos = i0 - 1 + n;
        if (pos < 0)
        {
            l[n] = 0.0f;
            r[n] = 0.0f;
        }
        else
        {
            l[n] = st->ring_l[pos & UPH_WSOLA_RING_MASK];
            r[n] = st->ring_r[pos & UPH_WSOLA_RING_MASK];
        }
    }

    *out_left  = uph_hermite4(l[0], l[1], l[2], l[3], t);
    *out_right = uph_hermite4(r[0], r[1], r[2], r[3], t);

    st->q            += st->read_rate;
    st->next_out_pos += 1.0;
}

static void uph_render_sample_block(
    const Uph_TimelineBlock *block,
    const Uph_Sample *sample,
    const Uph_SampleData *data,
    double buffer_start_beat,
    ma_uint32 frame_count,
    uint32_t engine_sample_rate,
    float bpm,
    float *track_l,
    float *track_r
)
{
    const double frames_per_beat = uph_beats_to_seconds(1.0, bpm) * (double)engine_sample_rate;

    const double time_scale  = (sample->time_scale > 0.0f) ? (double)sample->time_scale : 1.0;
    const double pitch_ratio = pow(2.0, (double)sample->pitch / 1200.0);

    const double start_offset_frames = block->start_offset_beats * frames_per_beat;
    const double block_length_frames = block->length_beats * frames_per_beat;
    const double first_out_pos       = (buffer_start_beat - block->start_beat) * frames_per_beat;

    uint32_t f = 0;
    if (first_out_pos < 0.0)
    {
        double skip = ceil(-first_out_pos);
        if (skip >= (double)frame_count)
            return;
        f = (uint32_t)skip;
    }

    if (first_out_pos + (double)f >= block_length_frames)
        return;

    const double speed = 1.0 / time_scale;

    const double read_rate = ((sample->mode == UPH_SAMPLE_RESAMPLE) ? speed : 1.0) * pitch_ratio;

    const bool use_wsola = (sample->pitch != 0)
        || (sample->mode == UPH_SAMPLE_STRETCH && time_scale != 1.0);

    Uph_Stretcher *stretcher = NULL;
    if (use_wsola)
    {
        stretcher = uph_stretcher_acquire(block);
        uph_stretcher_prepare(
            stretcher, data,
            start_offset_frames, speed / read_rate, read_rate,
            first_out_pos + (double)f
        );
    }

    for (; f < frame_count; f++)
    {
        double out_pos = first_out_pos + (double)f;
        if (out_pos >= block_length_frames)
            break;

        float left, right;
        if (stretcher)
            uph_stretcher_read(stretcher, data, &left, &right);
        else
            uph_read_sample_frame(data, start_offset_frames + out_pos * speed, &left, &right);

        track_l[f] += left;
        track_r[f] += right;
    }
}

double uph_sample_playback_length_beats(const Uph_Sample *sample, const Uph_SampleData *data, float bpm)
{
    if (bpm <= 0.0f || data->frame_count == 0)
        return 0.0;

    const double time_scale = (sample->time_scale > 0.0) ? (double)sample->time_scale : 1.0;
    const double seconds = (double)data->frame_count / (double)uph_audio_engine_data.device.sampleRate;

    return uph_seconds_to_beats(seconds * time_scale, bpm);
}

static void uph_queue_pattern_block_notes(
    Uph_Track *track,
    Uph_TimelineBlock *block,
    double buffer_start_beat,
    double buffer_end_beat,
    uint32_t engine_sample_rate,
    float bpm
)
{
    Uph_Project *project = &uph_state.project;

    if (block->resource_index >= naui_list_len(project->midi_patterns))
        return;

    Uph_MidiPattern *pattern = &project->midi_patterns[block->resource_index];
    uint64_t note_count = naui_list_len(pattern->notes);

    for (uint64_t n = 0; n < note_count; n++)
    {
        Uph_MidiNote *note = &pattern->notes[n];

        double note_start_beat = block->start_beat
            + (note->start_beat - block->start_offset_beats);
        double note_end_beat = note_start_beat + note->length_beats;

        double block_end_beat = block->start_beat + block->length_beats;
        if (note_start_beat < block->start_beat)
            note_start_beat = block->start_beat;
        if (note_end_beat > block_end_beat)
            note_end_beat = block_end_beat;

        if (note_end_beat <= note_start_beat)
            continue;

        if (note_start_beat < buffer_end_beat && note_end_beat > buffer_start_beat)
        {
            if (!uph_plugin_note_active(&track->instrument, note->key_number))
            {
                double trigger_beat = (note_start_beat > buffer_start_beat) ? note_start_beat : buffer_start_beat;
                double beats_from_buffer_start = trigger_beat - buffer_start_beat;
                double seconds_from_buffer_start = uph_beats_to_seconds(beats_from_buffer_start, bpm);
                uint32_t sample_offset = (uint32_t)(seconds_from_buffer_start * (double)engine_sample_rate);

                uph_plugin_queue_note_event(
                    &track->instrument,
                    true,
                    note->key_number,
                    0,
                    note->velocity,
                    sample_offset
                );
            }
        }

        if (note_end_beat >= buffer_start_beat && note_end_beat < buffer_end_beat)
        {
            double beats_from_buffer_start = note_end_beat - buffer_start_beat;
            double seconds_from_buffer_start = uph_beats_to_seconds(beats_from_buffer_start, bpm);
            uint32_t sample_offset = (uint32_t)(seconds_from_buffer_start * (double)engine_sample_rate);

            uph_plugin_queue_note_event(
                &track->instrument,
                false,
                note->key_number,
                0,
                note->velocity,
                sample_offset
            );
        }
    }
}

static void uph_render_track_instrument(
    Uph_Track *track,
    ma_uint32 frame_count,
    float *track_l,
    float *track_r,
    double playhead_beat,
    bool is_playing
)
{
    const uint32_t max_frame_count = uph_state.settings.audio.buffer_size;

    float *input_l = alloca(max_frame_count * sizeof(float));
    float *input_r = alloca(max_frame_count * sizeof(float));

    memset(input_l, 0, max_frame_count * sizeof(float));
    memset(input_r, 0, max_frame_count * sizeof(float));

    float *inputs[2]  = { input_l, input_r };
    float *outputs[2] = { track_l, track_r };

    uph_plugin_process(
        &track->instrument,
        (float**)inputs,
        (float**)outputs,
        frame_count,
        playhead_beat,
        is_playing
    );
}

static void uph_apply_effect_chain(
    Naui_List(Uph_EffectPlugin) effects,
    float *left,
    float *right,
    ma_uint32 frame_count,
    double playhead_beat,
    bool is_playing
)
{
    uint64_t effect_count = naui_list_len(effects);
    if (effect_count == 0)
        return;

    const uint32_t max_frame_count = uph_state.settings.audio.buffer_size;
    float *scratch_l = alloca(max_frame_count * sizeof(float));
    float *scratch_r = alloca(max_frame_count * sizeof(float));

    float *src[2] = { left, right };
    float *dst[2] = { scratch_l, scratch_r };

    const size_t bytes = sizeof(float) * frame_count;

    for (uint64_t e = 0; e < effect_count; e++)
    {
        Uph_EffectPlugin *effect = &effects[e];
        if (!effect->enabled || !effect->plugin.loaded)
            continue;

        memset(dst[0], 0, bytes);
        memset(dst[1], 0, bytes);

        uph_plugin_process(
            &effect->plugin,
            (float**)src,
            (float**)dst,
            frame_count,
            playhead_beat,
            is_playing
        );

        float *tmp_l = src[0];
        float *tmp_r = src[1];
        src[0] = dst[0];
        src[1] = dst[1];
        dst[0] = tmp_l;
        dst[1] = tmp_r;
    }

    if (src[0] != left)
    {
        memcpy(left,  src[0], bytes);
        memcpy(right, src[1], bytes);
    }
}

static void uph_mark_block_held_notes(
    Uph_Track *track,
    Uph_TimelineBlock *block,
    double buffer_end_beat,
    bool should_be_held[128]
)
{
    (void)track;
    Uph_Project *project = &uph_state.project;

    if (block->resource_index >= naui_list_len(project->midi_patterns))
        return;

    Uph_MidiPattern *pattern = &project->midi_patterns[block->resource_index];
    uint64_t note_count = naui_list_len(pattern->notes);
    double block_end_beat = block->start_beat + block->length_beats;

    for (uint64_t n = 0; n < note_count; n++)
    {
        Uph_MidiNote *note = &pattern->notes[n];
        double note_start_beat = block->start_beat + (note->start_beat - block->start_offset_beats);
        double note_end_beat = note_start_beat + note->length_beats;

        if (note_start_beat < block->start_beat)
            note_start_beat = block->start_beat;
        if (note_end_beat > block_end_beat)
            note_end_beat = block_end_beat;

        if (note_end_beat <= note_start_beat)
            continue;

        if (note_start_beat < buffer_end_beat && note_end_beat >= buffer_end_beat)
            should_be_held[note->key_number] = true;
    }
}

static void uph_queue_automation_block_params(
    Uph_Track *automation_track,
    Uph_Track *parent_track,
    Uph_TimelineBlock *block,
    double buffer_start_beat,
    double buffer_end_beat,
    uint32_t engine_sample_rate,
    float bpm,
    uint32_t samples_per_control_point
)
{
    Uph_Project *project = &uph_state.project;

    if (block->resource_index >= naui_list_len(project->automations))
        return;

    Uph_Automation *automation = &project->automations[block->resource_index];

    const Uph_PluginParam *param = automation_track->automation_param;

    int32_t effect_index;
    if (!uph_resources_param_owner_index(parent_track, param, &effect_index))
        return;

    Uph_Plugin *target_plugin;
    if (effect_index < 0)
    {
        target_plugin = &parent_track->instrument;
    }
    else
    {
        Uph_EffectPlugin *effect = &parent_track->effects[effect_index];
        if (!effect->enabled)
            return;

        target_plugin = &effect->plugin;
    }

    if (!target_plugin->loaded)
        return;

    double block_end_beat = block->start_beat + block->length_beats;

    const uint32_t max_frame_count = uph_state.settings.audio.buffer_size;
    for (uint32_t offset = 0; offset < max_frame_count; offset += samples_per_control_point)
    {
        double frame_beat = buffer_start_beat
            + uph_seconds_to_beats((double)offset / (double)engine_sample_rate, bpm);

        if (frame_beat < block->start_beat || frame_beat >= block_end_beat)
            continue;
        if (frame_beat < buffer_start_beat || frame_beat >= buffer_end_beat)
            continue;

        double automation_beat = (frame_beat - block->start_beat) + block->start_offset_beats;

        float value;
        if (!uph_automation_value_at_beat(automation, automation_beat, &value))
            continue;

        uph_plugin_queue_param_change(
            target_plugin,
            param->id,
            (double)value,
            offset
        );
    }
}

static void uph_queue_track_automation(
    Uph_Track *track,
    double buffer_start_beat,
    double buffer_end_beat,
    uint32_t engine_sample_rate,
    float bpm
)
{
    uint64_t subtrack_count = naui_list_len(track->subtracks);
    for (uint64_t s = 0; s < subtrack_count; s++)
    {
        Uph_Track *sub = &track->subtracks[s];
        if (sub->type != UPH_RESOURCE_AUTOMATION)
            continue;

        if (sub->state & UPH_TRACK_MUTED)
            continue;

        uint64_t automation_block_count = naui_list_len(sub->blocks);
        for (uint64_t b = 0; b < automation_block_count; b++)
        {
            Uph_TimelineBlock *block = &sub->blocks[b];

            uph_queue_automation_block_params(
                sub, track, block,
                buffer_start_beat, buffer_end_beat,
                engine_sample_rate, bpm,
                32
            );
        }
    }
}

typedef struct
{
    Uph_Project *project;
    const float *input;
    double playhead_start_beat;
    double buffer_start_beat;
    double buffer_end_beat;
    float bpm;
    uint32_t engine_sample_rate;
    uint32_t max_frame_count;
    ma_uint32 frame_count;
    bool timeline_playing;
}
Uph_RenderContext;

static void uph_queue_preview_notes(const Uph_RenderContext *ctx, Uph_Track *track, bool should_be_held[128])
{
    uph_midi_preview_queue_audition(track, should_be_held);

    Uph_MidiPreviewRange range;
    if (!uph_midi_preview_playback_range(track, &range))
        return;

    Uph_TimelineBlock block = {
        .length_beats = range.length,
        .resource_index = range.pattern_index,
        .type = UPH_RESOURCE_PATTERN
    };

    for (block.start_beat = 0.0; block.start_beat < range.end_beat; block.start_beat += range.length)
    {
        uph_queue_pattern_block_notes(
            track, &block,
            range.start_beat, range.end_beat,
            ctx->engine_sample_rate, ctx->bpm
        );

        uph_mark_block_held_notes(track, &block, range.end_beat, should_be_held);

        if (!range.loop)
            break;
    }
}

static void uph_clear_track_peaks(Uph_Track *track)
{
    track->peak_left = 0.0f;
    track->peak_right = 0.0f;

    if (track->type != UPH_RESOURCE_GROUP)
        return;

    uint64_t child_count = naui_list_len(track->subtracks);
    for (uint64_t c = 0; c < child_count; c++)
        uph_clear_track_peaks(&track->subtracks[c]);
}

static void uph_render_track(const Uph_RenderContext *ctx, Uph_Track *track, float *dest_l, float *dest_r)
{
    Uph_Project *project = ctx->project;
    const ma_uint32 frame_count = ctx->frame_count;
    const uint32_t max_frame_count = ctx->max_frame_count;

    track->peak_left = 0.0f;
    track->peak_right = 0.0f;

    if (!uph_track_is_audible(project, track))
    {
        uph_clear_track_peaks(track);
        return;
    }

    float *track_l = alloca(max_frame_count * sizeof(float));
    float *track_r = alloca(max_frame_count * sizeof(float));
    memset(track_l, 0, max_frame_count * sizeof(float));
    memset(track_r, 0, max_frame_count * sizeof(float));

    uint64_t block_count = naui_list_len(track->blocks);

    float volume = (float)fabs(track->volume);
    float pan = NAUI_CLAMP(track->pan, -1.0f, 1.0f);

    float pan_angle = (pan + 1.0f) * 0.25f * (float)NAUI_PI;
    float gain_left  = volume * cosf(pan_angle) * NAUI_SQRT2;
    float gain_right = volume * sinf(pan_angle) * NAUI_SQRT2;

    if (ctx->timeline_playing)
    {
        uph_queue_track_automation(
            track,
            ctx->buffer_start_beat, ctx->buffer_end_beat,
            ctx->engine_sample_rate, ctx->bpm
        );
    }

    if (track->type == UPH_RESOURCE_GROUP)
    {
        uint64_t child_count = naui_list_len(track->subtracks);
        for (uint64_t c = 0; c < child_count; c++)
        {
            Uph_Track *child = &track->subtracks[c];

            if (child->type == UPH_RESOURCE_AUTOMATION)
                continue;

            uph_render_track(ctx, child, track_l, track_r);
        }
    }
    else if (track->type == UPH_RESOURCE_SAMPLE)
    {
        if (ctx->timeline_playing)
        {
            for (uint64_t b = 0; b < block_count; b++)
            {
                Uph_TimelineBlock *block = &track->blocks[b];

                if (block == track->armed_block)
                    continue;

                if (block->resource_index >= naui_list_len(project->samples))
                    continue;

                Uph_Sample *sample = &project->samples[block->resource_index];
                if (sample->data_index >= naui_list_len(project->sample_data))
                    continue;

                uph_render_sample_block(
                    block, sample, &project->sample_data[sample->data_index],
                    ctx->playhead_start_beat, frame_count, ctx->engine_sample_rate, ctx->bpm,
                    track_l, track_r
                );
            }
        }
    }
    else if (track->type == UPH_RESOURCE_PATTERN)
    {
        if (track->instrument.loaded)
        {
            bool should_be_held[128] = { false };

            uph_queue_preview_notes(ctx, track, should_be_held);

            if (ctx->timeline_playing)
            {
                for (uint64_t b = 0; b < block_count; b++)
                {
                    Uph_TimelineBlock *block = &track->blocks[b];
                    if (block->type != UPH_RESOURCE_PATTERN || block == track->armed_block)
                        continue;

                    uph_queue_pattern_block_notes(
                        track, block,
                        ctx->buffer_start_beat, ctx->buffer_end_beat,
                        ctx->engine_sample_rate, ctx->bpm
                    );

                    uph_mark_block_held_notes(
                        track, block,
                        ctx->buffer_end_beat,
                        should_be_held
                    );
                }
            }

            for (int key = 0; key < 128; key++)
            {
                if (uph_plugin_note_active(&track->instrument, (uint8_t)key) && !should_be_held[key] && !(uph_track_monitors_instrument(track) && uph_midi_recorder_key_held((uint8_t)key)))
                    uph_plugin_queue_note_event(&track->instrument, false, (uint8_t)key, 0, 0, 0);
            }

            uph_render_track_instrument(
                track, frame_count,
                track_l, track_r,
                ctx->playhead_start_beat, ctx->timeline_playing
            );
        }
    }

    if (ctx->input
        && (track->state & UPH_TRACK_MONITORED)
        && (track->type == UPH_RESOURCE_SAMPLE || track->type == UPH_RESOURCE_NONE))
    {
        for (ma_uint32 f = 0; f < frame_count; f++)
        {
            track_l[f] += ctx->input[f * 2 + 0];
            track_r[f] += ctx->input[f * 2 + 1];
        }
    }

    uph_apply_effect_chain(
        track->effects,
        track_l, track_r,
        frame_count,
        ctx->playhead_start_beat, ctx->timeline_playing
    );

    float track_peak_left = 0.0f;
    float track_peak_right = 0.0f;

    for (ma_uint32 f = 0; f < frame_count; f++)
    {
        float out_left  = track_l[f] * gain_left;
        float out_right = track_r[f] * gain_right;

        dest_l[f] += out_left;
        dest_r[f] += out_right;

        float abs_left  = fabsf(out_left);
        float abs_right = fabsf(out_right);
        if (abs_left  > track_peak_left)  track_peak_left  = abs_left;
        if (abs_right > track_peak_right) track_peak_right = abs_right;
    }

    track->peak_left  = track_peak_left;
    track->peak_right = track_peak_right;
}

static void uph_render_audio(double playhead_start_beat, uint32_t engine_sample_rate, float *out, ma_uint32 frame_count, bool timeline_playing, const float *input)
{
    memset(out, 0, sizeof(float) * 2 * frame_count);

    Uph_Project *project = &uph_state.project;
    float bpm = project->bpm;
    if (bpm <= 0.0f)
        return;

    double buffer_start_beat = playhead_start_beat;
    double buffer_end_beat = playhead_start_beat
        + uph_seconds_to_beats((double)frame_count / (double)engine_sample_rate, bpm);

    const uint32_t max_frame_count = uph_state.settings.audio.buffer_size;

    float *master_l = alloca(max_frame_count * sizeof(float));
    float *master_r = alloca(max_frame_count * sizeof(float));
    memset(master_l, 0, max_frame_count * sizeof(float));
    memset(master_r, 0, max_frame_count * sizeof(float));

    const Uph_RenderContext ctx = {
        .project = project,
        .input = input,
        .playhead_start_beat = playhead_start_beat,
        .buffer_start_beat = buffer_start_beat,
        .buffer_end_beat = buffer_end_beat,
        .bpm = bpm,
        .engine_sample_rate = engine_sample_rate,
        .max_frame_count = max_frame_count,
        .frame_count = frame_count,
        .timeline_playing = timeline_playing
    };

    uint64_t track_count = naui_list_len(project->tracks);
    for (uint64_t t = 0; t < track_count; t++)
        uph_render_track(&ctx, &project->tracks[t], master_l, master_r);

    Uph_MasterTrack *master = &project->master_track;

    uph_apply_effect_chain(
        master->effects,
        master_l, master_r,
        frame_count,
        playhead_start_beat, timeline_playing
    );

    if (master->state & UPH_TRACK_MUTED)
    {
        master->peak_left = 0.0f;
        master->peak_right = 0.0f;
        return;
    }

    float master_volume = (float)fabs(master->volume);
    float master_pan    = NAUI_CLAMP(master->pan, -1.0f, 1.0f);
    float master_pan_angle = (master_pan + 1.0f) * 0.25f * (float)NAUI_PI;
    float master_gain_left  = master_volume * cosf(master_pan_angle) * NAUI_SQRT2;
    float master_gain_right = master_volume * sinf(master_pan_angle) * NAUI_SQRT2;

    float master_peak_left  = 0.0f;
    float master_peak_right = 0.0f;

    for (ma_uint32 f = 0; f < frame_count; f++)
    {
        float out_left  = master_l[f] * master_gain_left;
        float out_right = master_r[f] * master_gain_right;

        out[f * 2 + 0] = out_left;
        out[f * 2 + 1] = out_right;

        float abs_left  = fabsf(out_left);
        float abs_right = fabsf(out_right);
        if (abs_left  > master_peak_left)  master_peak_left  = abs_left;
        if (abs_right > master_peak_right) master_peak_right = abs_right;
    }

    master->peak_left  = master_peak_left;
    master->peak_right = master_peak_right;
}

void uph_audio_engine_stop(void)
{
    ma_device_stop(&uph_audio_engine_data.device);
}

void uph_audio_engine_start(void)
{
    ma_device_start(&uph_audio_engine_data.device);
}

static void uph_update_waveform_peaks(Uph_SampleData *sample_data, uint64_t old_frame_count)
{
    if (!sample_data->frames || sample_data->frame_count == 0)
        return;

    const float *samples = (const float*)sample_data->frames;
    const int channel_count = (sample_data->channel_type == UPH_SAMPLE_STEREO) ? 2 : 1;

    const uint32_t max_frame_count = UPH_WAVEFORM_BIN_FRAMES;

    const uint64_t first_bin = old_frame_count / max_frame_count;
    const uint64_t bin_count = (sample_data->frame_count + max_frame_count - 1) / max_frame_count;

    for (uint64_t bin = first_bin; bin < bin_count; bin++)
    {
        uint64_t frame_start = bin * max_frame_count;
        uint64_t frame_end = frame_start + max_frame_count;
        if (frame_end > sample_data->frame_count)
            frame_end = sample_data->frame_count;

        float min_v = 1.0f;
        float max_v = -1.0f;

        for (uint64_t f = frame_start; f < frame_end; f++)
        {
            float value = 0.0f;
            for (int c = 0; c < channel_count; c++)
            {
                value += samples[f * channel_count + c];
            }
            value /= (float)channel_count;

            if (value < min_v) min_v = value;
            if (value > max_v) max_v = value;
        }

        Uph_WaveformPeak peak = {
            .min = uph_waveform_encode_uint16(min_v),
            .max = uph_waveform_encode_uint16(max_v),
        };

        if (bin < naui_list_len(sample_data->waveform_peaks))
            sample_data->waveform_peaks[bin] = peak;
        else
            naui_list_push(sample_data->waveform_peaks, peak);
    }
}

static void uph_build_waveform_peaks(Uph_SampleData *sample_data)
{
    if (!sample_data->frames || sample_data->frame_count == 0)
        return;

    const uint32_t max_frame_count = UPH_WAVEFORM_BIN_FRAMES;
    const uint64_t bin_count = (sample_data->frame_count + max_frame_count - 1) / max_frame_count;
    naui_list_reserve(sample_data->waveform_peaks, bin_count);

    uph_update_waveform_peaks(sample_data, 0);
}

static void uph_begin_recording(Uph_Track *track)
{
    if (track->type == UPH_RESOURCE_NONE)
        track->type = UPH_RESOURCE_SAMPLE;
    if (track->type != UPH_RESOURCE_SAMPLE)
        return;

    const uint32_t rate = uph_audio_engine_data.device.sampleRate;

    Uph_SampleData data = {0};
    data.channel_type = UPH_SAMPLE_STEREO;
    data.original_sample_rate = rate;
    data.frame_capacity = (uint64_t)rate * 30;
    data.frames = malloc((size_t)(data.frame_capacity * 2 * sizeof(float)));
    if (!data.frames)
        return;

    Uph_ResourceIndex sample_index = uph_resources_add_sample_data(
        data,
        naui_string_from_cstr(NAUI_TR("sample.recording.default.name"))
    );

    Uph_TimelineBlock block = {
        .type = UPH_RESOURCE_SAMPLE,
        .start_beat = uph_state.shared.song_timeline_playhead_position,
        .length_beats = 0,
        .resource_index = sample_index,
    };

    naui_list_push(track->blocks, block);
    track->armed_block = &track->blocks[naui_list_len(track->blocks) - 1];
}

static bool uph_write_wav(const char *filepath, const float *frames, uint64_t frame_count, uint32_t channels, uint32_t sample_rate)
{
    if (!frames || frame_count == 0)
        return false;

    ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, channels, sample_rate);
    ma_encoder encoder;
    if (ma_encoder_init_file(filepath, &config, &encoder) != MA_SUCCESS)
    {
        fprintf(stderr, "uph_write_wav: failed to open '%s' for writing\n", filepath);
        return false;
    }

    ma_uint64 frames_written = 0;
    ma_result result = ma_encoder_write_pcm_frames(&encoder, frames, frame_count, &frames_written);
    ma_encoder_uninit(&encoder);

    if (result != MA_SUCCESS || frames_written != frame_count)
    {
        fprintf(stderr, "uph_write_wav: failed writing frames to '%s'\n", filepath);
        return false;
    }

    return true;
}

static bool uph_track_armed_block_in_range(const Uph_Track *track)
{
    const Uph_TimelineBlock *block = track->armed_block;
    return block >= track->blocks && block < track->blocks + naui_list_len(track->blocks);
}

static void uph_finish_take(Uph_Track *track)
{
    if (track->armed_block == UPH_INVALID_TIMELINE_BLOCK)
        return;

    if (track->type == UPH_RESOURCE_PATTERN)
    {
        uph_midi_recorder_finish_take(track);
        return;
    }

    Uph_Project *project = &uph_state.project;

    if (uph_track_armed_block_in_range(track))
    {
        Uph_TimelineBlock *block = track->armed_block;

        if (block->resource_index < naui_list_len(project->samples))
        {
            Uph_Sample *sample = &project->samples[block->resource_index];
            Uph_SampleData *data = &project->sample_data[sample->data_index];

            if (data->frame_count > 0 && data->frames)
            {
                float *trimmed = (float*)realloc(data->frames, (size_t)(data->frame_count * 2 * sizeof(float)));
                if (trimmed)
                    data->frames = trimmed;
                data->frame_capacity = data->frame_count;

                Naui_Path export_path = naui_path_join(uph_project_get_path(&uph_state.project), NAUI_PATH(UPH_IO_FOLDER_SAMPLES));
                export_path = naui_file_unique_name(
                    naui_path_join(export_path, NAUI_PATH(naui_string_format("%s.wav", sample->name.data).data)),
                    export_path
                );

                if (uph_write_wav(export_path.data, (const float*)data->frames, data->frame_count, 2, data->original_sample_rate))
                {
                    data->file_path = export_path;
                    fprintf(stdout, "uph_finish_take: saved take to '%s'\n", export_path.data);
                }
            }
        }
    }
    track->armed_block = UPH_INVALID_TIMELINE_BLOCK;
}

static bool uph_stop_recording_all(Naui_List(Uph_Track) tracks)
{
    bool was_recording = false;

    for (uint64_t i = 0; i < naui_list_len(tracks); i++)
    {
        Uph_Track *track = &tracks[i];

        if (uph_stop_recording_all(track->subtracks))
            was_recording = true;

        if (track->armed_block == UPH_INVALID_TIMELINE_BLOCK)
            continue;

        uph_finish_take(track);
        track->state &= (Uph_TrackState)~UPH_TRACK_ARMED;
        was_recording = true;
    }

    return was_recording;
}

static bool uph_has_active_take(Naui_List(Uph_Track) tracks)
{
    for (uint64_t i = 0; i < naui_list_len(tracks); i++)
    {
        if (tracks[i].armed_block != UPH_INVALID_TIMELINE_BLOCK)
            return true;
        if (uph_has_active_take(tracks[i].subtracks))
            return true;
    }
    return false;
}

static void uph_record_input(
    Naui_List(Uph_Track) tracks,
    const float *input,
    ma_uint32 frame_count,
    uint32_t sample_rate,
    float bpm
)
{
    Uph_Project *project = &uph_state.project;

    for (uint64_t i = 0; i < naui_list_len(tracks); i++)
    {
        Uph_Track *track = &tracks[i];
        uph_record_input(track->subtracks, input, frame_count, sample_rate, bpm);

        if (track->type == UPH_RESOURCE_PATTERN)
            continue;

        if (!(track->state & UPH_TRACK_ARMED))
        {
            uph_finish_take(track);
            continue;
        }

        if (track->armed_block == UPH_INVALID_TIMELINE_BLOCK)
        {
            uph_begin_recording(track);
            if (track->armed_block == UPH_INVALID_TIMELINE_BLOCK)
                continue;
        }

        if (!uph_track_armed_block_in_range(track))
        {
            track->armed_block = UPH_INVALID_TIMELINE_BLOCK;
            continue;
        }

        Uph_TimelineBlock *block = track->armed_block;

        if (block->resource_index >= naui_list_len(project->samples))
        {
            track->armed_block = UPH_INVALID_TIMELINE_BLOCK;
            continue;
        }

        Uph_Sample *sample = &project->samples[block->resource_index];
        Uph_SampleData *data = &project->sample_data[sample->data_index];

        uint64_t needed = data->frame_count + frame_count;
        if (needed > data->frame_capacity)
        {
            uint64_t new_capacity = data->frame_capacity ? data->frame_capacity * 2 : (uint64_t)sample_rate * 30;
            while (new_capacity < needed)
                new_capacity *= 2;

            float *grown = (float*)realloc(data->frames, (size_t)(new_capacity * 2 * sizeof(float)));
            if (!grown)
            {
                uph_finish_take(track);
                continue;
            }
            data->frames = grown;
            data->frame_capacity = new_capacity;
        }

        float *dst = (float*)data->frames + data->frame_count * 2;
        memcpy(dst, input, sizeof(float) * 2 * frame_count);

        uint64_t old_frame_count = data->frame_count;
        data->frame_count += frame_count;

        block->length_beats = uph_seconds_to_beats(
            (double)data->frame_count / (double)sample_rate, bpm);

        uph_update_waveform_peaks(data, old_frame_count);
    }
}

static void uph_audio_engine_data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
    static bool was_playing = false;

    float *out = (float*)output;
    uint32_t engine_sample_rate = device->sampleRate;
    double playhead_start_beat = uph_state.shared.song_timeline_playhead_position;
    bool is_playing = uph_state.shared.song_timeline_playing;
    float bpm = uph_state.project.bpm;

    const int input_channel = 0; // make it so the user can choose between channel 1 and 2 in settings
    const float *input_f = (const float*)input;
    const ma_uint32 input_channels = device->capture.channels;
    const ma_uint32 output_channels = device->playback.channels;

    if (input_f && input_channels > 0)
    {
        const ma_uint32 channel = (ma_uint32)input_channel < input_channels ? (ma_uint32)input_channel : 0;
        float *routed = alloca(frame_count * 2 * sizeof(float));
        for (ma_uint32 f = 0; f < frame_count; f++)
        {
            float s = input_f[f * input_channels + channel];
            routed[f * 2 + 0] = s;
            routed[f * 2 + 1] = s;
        }
        input_f = routed;
    }

    uph_midi_recorder_process(playhead_start_beat, frame_count, engine_sample_rate, bpm, is_playing);

    float *rendered = output_channels == 2 ? out : alloca(frame_count * 2 * sizeof(float));
    uph_midi_preview_begin_buffer(frame_count, engine_sample_rate, bpm, is_playing);
    uph_render_audio(playhead_start_beat, engine_sample_rate, rendered, frame_count, is_playing, input_f);
    uph_midi_preview_end_buffer();

    if (output_channels != 2)
    {
        for (ma_uint32 f = 0; f < frame_count; f++)
        {
            const float left = rendered[f * 2 + 0];
            const float right = rendered[f * 2 + 1];

            for (ma_uint32 c = 0; c < output_channels; c++)
            {
                float value = 0.0f;
                if (output_channels == 1)
                    value = 0.5f * (left + right);
                else if (c < 2)
                    value = c == 0 ? left : right;

                out[f * output_channels + c] = value;
            }
        }
    }

    if (is_playing && input_f && bpm > 0.0f)
        uph_record_input(uph_state.project.tracks, input_f, frame_count, engine_sample_rate, bpm);

    if (was_playing && !is_playing)
    {
        uph_audio_engine_stop_all_notes();
		
        if (uph_stop_recording_all(uph_state.project.tracks))
            uph_state.shared.song_timeline_playhead_position = 0.0;
    }

    was_playing = is_playing;
    if (!is_playing)
        return;

    if (bpm > 0.0f)
    {
        double buffer_beats = uph_seconds_to_beats((double)frame_count / (double)engine_sample_rate, bpm);
        double new_beat = playhead_start_beat + buffer_beats;
        if (uph_state.project.loop_enabled && new_beat >= uph_audio_engine_get_song_length() && !uph_has_active_take(uph_state.project.tracks))
            new_beat = 0.0;

        uph_state.shared.song_timeline_playhead_position = new_beat;
    }
}

static bool uph_audio_engine_find_device(const ma_device_type type, const char *name, ma_device_id *id)
{
    if (!uph_audio_engine_data.context_ready || !name[0])
        return false;

    ma_device_info *playback = NULL;
    ma_device_info *capture = NULL;
    ma_uint32 playback_count = 0;
    ma_uint32 capture_count = 0;

    if (ma_context_get_devices(&uph_audio_engine_data.context, &playback, &playback_count, &capture, &capture_count) != MA_SUCCESS)
        return false;

    const ma_device_info *devices = type == ma_device_type_playback ? playback : capture;
    const ma_uint32 count = type == ma_device_type_playback ? playback_count : capture_count;

    for (ma_uint32 i = 0; i < count; i++)
    {
        if (strcmp(devices[i].name, name) == 0)
        {
            *id = devices[i].id;
            return true;
        }
    }

    fprintf(stderr, "uph_audio_engine_find_device: device '%s' not found, using system default\n", name);
    return false;
}

static bool uph_audio_engine_init_device(void)
{
    Uph_AudioSettings settings = uph_state.settings.audio;

    ma_device_id playback_id;
    ma_device_id capture_id;
    const bool has_playback_id = uph_audio_engine_find_device(ma_device_type_playback, settings.output_device.name.data, &playback_id);
    const bool has_capture_id = uph_audio_engine_find_device(ma_device_type_capture, settings.input_device.name.data, &capture_id);

    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.capture.format     = ma_format_f32;
    config.capture.channels   = settings.channels;
    config.capture.pDeviceID  = has_capture_id ? &capture_id : NULL;
    config.playback.format    = ma_format_f32;
    config.playback.channels  = settings.channels;
    config.playback.pDeviceID = has_playback_id ? &playback_id : NULL;
    config.sampleRate         = settings.sample_rate;
    config.dataCallback       = uph_audio_engine_data_callback;

    config.performanceProfile  = ma_performance_profile_low_latency;
    config.playback.shareMode  = settings.exclusive_mode ? ma_share_mode_exclusive : ma_share_mode_shared;
    config.capture.shareMode   = settings.exclusive_mode ? ma_share_mode_exclusive : ma_share_mode_shared;
    config.periodSizeInFrames = settings.buffer_size;
    //config.periods            = 2;

    ma_context *context = uph_audio_engine_get_context();

    ma_result result = ma_device_init(context, &config, &uph_audio_engine_data.device);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_init: failed to init duplex device (%s), falling back to playback only\n", ma_result_description(result));

        config.deviceType = ma_device_type_playback;
        result = ma_device_init(context, &config, &uph_audio_engine_data.device);
        if (result != MA_SUCCESS)
        {
            fprintf(stderr, "uph_audio_engine_init: failed to init playback device (%s)\n", ma_result_description(result));
            return false;
        }
    }

    ma_device_start(&uph_audio_engine_data.device);
    return true;
}

void uph_audio_engine_init(void)
{
    uph_stretch_init_window();

    if (ma_context_init(NULL, 0, NULL, &uph_audio_engine_data.context) == MA_SUCCESS)
        uph_audio_engine_data.context_ready = true;
    else
        fprintf(stderr, "uph_audio_engine_init: failed to init context, using a temporary one per device\n");

    uph_audio_engine_init_device();
}

void uph_audio_engine_shutdown(void)
{
    ma_device_uninit(&uph_audio_engine_data.device);

    if (uph_audio_engine_data.context_ready)
    {
        ma_context_uninit(&uph_audio_engine_data.context);
        uph_audio_engine_data.context_ready = false;
    }
}

ma_context *uph_audio_engine_get_context(void)
{
    return uph_audio_engine_data.context_ready ? &uph_audio_engine_data.context : NULL;
}

bool uph_audio_engine_recreate(void)
{
    ma_device_uninit(&uph_audio_engine_data.device);
    return uph_audio_engine_init_device();
}

bool uph_audio_engine_device_running(void)
{
    return ma_device_is_started(&uph_audio_engine_data.device);
}

Uph_SampleData uph_audio_engine_load_sample_data(Naui_Path path)
{
    Uph_SampleData sample_data = {0};

    size_t file_size;
    void *file_data = naui_file_read_all(path, &file_size);
    if (!file_data)
    {
        fprintf(stderr, "uph_audio_engine_load_sample: failed to read file '%s'\n", path.data);
        return sample_data;
    }

    const uint32_t device_rate = uph_audio_engine_data.device.sampleRate;

    uint32_t original_sample_rate = device_rate;
    ma_uint32 native_channels = 2;
    {
        ma_decoder probe;
        ma_decoder_config probe_config = ma_decoder_config_init(ma_format_f32, 0, 0);
        if (ma_decoder_init_memory(file_data, file_size, &probe_config, &probe) == MA_SUCCESS)
        {
            original_sample_rate = probe.outputSampleRate;
            native_channels = probe.outputChannels;
            ma_decoder_uninit(&probe);
        }
    }

    const ma_uint32 channels = (native_channels == 1) ? 1 : 2;

    ma_decoder decoder;
    ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, channels, device_rate);

    ma_result result = ma_decoder_init_memory(file_data, file_size, &decoder_config, &decoder);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_load_sample: failed to open '%s' (%s)\n", path.data, ma_result_description(result));
        free(file_data);
        return sample_data;
    }

    ma_uint64 capacity = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &capacity);
    capacity = capacity ? capacity + 4096 : (ma_uint64)device_rate * 10;

    float *frames = (float*)malloc((size_t)(capacity * channels * sizeof(float)));
    ma_uint64 total_frames = 0;

    while (frames)
    {
        if (total_frames == capacity)
        {
            capacity *= 2;
            float *grown = (float*)realloc(frames, (size_t)(capacity * channels * sizeof(float)));
            if (!grown)
            {
                free(frames);
                frames = NULL;
                break;
            }
            frames = grown;
        }

        ma_uint64 frames_read = 0;
        result = ma_decoder_read_pcm_frames(
            &decoder,
            frames + total_frames * channels,
            capacity - total_frames,
            &frames_read
        );
        total_frames += frames_read;

        if (result != MA_SUCCESS || frames_read == 0)
            break;
    }

    const ma_uint32 sample_rate = decoder.outputSampleRate;

    ma_decoder_uninit(&decoder);
    free(file_data);

    if (!frames)
    {
        fprintf(stderr, "uph_audio_engine_load_sample: out of memory loading '%s'\n", path.data);
        return sample_data;
    }

    if (total_frames == 0)
    {
        fprintf(stderr, "uph_audio_engine_load_sample: failed to decode '%s'\n", path.data);
        free(frames);
        return sample_data;
    }

    float *trimmed = (float*)realloc(frames, (size_t)(total_frames * channels * sizeof(float)));
    if (trimmed)
        frames = trimmed;

    sample_data.file_path = path;
    sample_data.frames = frames;
    sample_data.frame_count = total_frames;
    sample_data.frame_capacity = total_frames;
    sample_data.original_sample_rate = original_sample_rate;
    sample_data.channel_type = (channels == 1) ? UPH_SAMPLE_MONO : UPH_SAMPLE_STEREO;

    if (original_sample_rate != sample_rate)
        fprintf(stdout, "uph_audio_engine_load_sample: loaded '%s' (%llu frames, %u channels, %u Hz -> %u Hz [resampled])\n",
                path.data, (unsigned long long)total_frames, channels, original_sample_rate, sample_rate);
    else
        fprintf(stdout, "uph_audio_engine_load_sample: loaded '%s' (%llu frames, %u channels, %u Hz)\n",
                path.data, (unsigned long long)total_frames, channels, sample_rate);

    uph_build_waveform_peaks(&sample_data);

    return sample_data;
}

static double uph_tracks_end_beat(const Uph_Project *project, Naui_List(Uph_Track) tracks)
{
    double max_end_beat = 0.0;
    uint64_t track_count = naui_list_len(tracks);

    for (uint64_t t = 0; t < track_count; t++)
    {
        Uph_Track *track = &tracks[t];
        if (!uph_track_is_audible(project, track))
            continue;

        uint64_t block_count = naui_list_len(track->blocks);

        for (uint64_t b = 0; b < block_count; b++)
        {
            Uph_TimelineBlock *block = &track->blocks[b];
            double block_end = block->start_beat + block->length_beats;

            if (block_end > max_end_beat)
                max_end_beat = block_end;
        }

        // Groups hold other tracks; on any other track the subtracks are automation lanes, which don't extend the song.
        if (track->type == UPH_RESOURCE_GROUP)
        {
            double children_end = uph_tracks_end_beat(project, track->subtracks);
            if (children_end > max_end_beat)
                max_end_beat = children_end;
        }
    }

    return max_end_beat;
}

double uph_audio_engine_get_song_length_beats(void)
{
    Uph_Project *project = &uph_state.project;
    return uph_tracks_end_beat(project, project->tracks);
}

double uph_audio_engine_get_song_length_seconds(void)
{
    Uph_Project *project = &uph_state.project;
    float bpm = project->bpm;
    if (bpm <= 0.0f)
        return 0.0;

    return uph_beats_to_seconds(uph_audio_engine_get_song_length_beats(), bpm);
}

double uph_audio_engine_get_song_length(void)
{
    const double bpm = (double)uph_state.project.time_signature.numerator;
    double length_beats = uph_audio_engine_get_song_length_beats();
    return length_beats > 0.0 ? ceil(length_beats / bpm) * bpm : bpm;
}

bool uph_audio_engine_export_to_wav(const char *filepath, double start_beat, double end_beat)
{
    if (end_beat <= start_beat)
    {
        fprintf(stderr, "uph_audio_engine_export_to_wav: invalid beat range\n");
        return false;
    }

    Uph_Project *project = &uph_state.project;
    float bpm = project->bpm;
    if (bpm <= 0.0f)
    {
        fprintf(stderr, "uph_audio_engine_export_to_wav: invalid bpm\n");
        return false;
    }

    double duration_beats   = end_beat - start_beat;
    double duration_seconds = uph_beats_to_seconds(duration_beats, bpm);

    uint32_t sample_rate = uph_audio_engine_data.device.sampleRate;
    uint64_t total_frames = (uint64_t)(duration_seconds * (double)sample_rate);

    ma_encoder_config encoder_config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, sample_rate);
    ma_encoder encoder;
    if (ma_encoder_init_file(filepath, &encoder_config, &encoder) != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_export_to_wav: failed to init WAV encoder for '%s'\n", filepath);
        return false;
    }

    const ma_uint32 chunk_frames = uph_state.settings.audio.buffer_size;
    float *buffer = (float*)malloc(sizeof(float) * 2 * chunk_frames);
    if (!buffer)
    {
        fprintf(stderr, "uph_audio_engine_export_to_wav: out of memory\n");
        ma_encoder_uninit(&encoder);
        return false;
    }

    uint64_t frames_processed = 0;
    bool ok = true;

    ma_device_stop(&uph_audio_engine_data.device);
    while (frames_processed < total_frames)
    {
        ma_uint32 frames_to_process = (ma_uint32)((total_frames - frames_processed) < chunk_frames
            ? (total_frames - frames_processed)
            : chunk_frames);

        double playhead_beat = start_beat + uph_seconds_to_beats((double)frames_processed / (double)sample_rate, bpm);

        uph_render_audio(playhead_beat, sample_rate, buffer, frames_to_process, true, NULL);

        ma_uint64 frames_written = 0;
        if (ma_encoder_write_pcm_frames(&encoder, buffer, frames_to_process, &frames_written) != MA_SUCCESS)
        {
            fprintf(stderr, "uph_audio_engine_export_to_wav: failed writing frames to '%s'\n", filepath);
            ok = false;
            break;
        }

        frames_processed += frames_written;
    }

    free(buffer);
    ma_encoder_uninit(&encoder);
    ma_device_start(&uph_audio_engine_data.device);

    uph_audio_engine_stop_all_notes();

    if (ok)
        fprintf(stdout, "uph_audio_engine_export_to_wav: exported %llu frames to '%s'\n",
                (unsigned long long)frames_processed, filepath);

    return ok;
}

void uph_audio_engine_unload_sample_data(Uph_SampleData *sample_data)
{
    if (!sample_data)
        return;
    free(sample_data->frames);
    naui_list_free(sample_data->waveform_peaks);
}

bool uph_audio_engine_sample_data_valid(const Uph_SampleData *sample_data)
{
    return sample_data->frame_count != 0;
}

static void uph_stop_all_notes_recursive(Naui_List(Uph_Track) tracks)
{
    uint64_t track_count = naui_list_len(tracks);

    for (uint64_t t = 0; t < track_count; t++)
    {
        Uph_Track *track = &tracks[t];

        if (track->type == UPH_RESOURCE_PATTERN && track->instrument.loaded)
            uph_plugin_queue_stop_all(&track->instrument, 0);

        if (track->type == UPH_RESOURCE_GROUP)
            uph_stop_all_notes_recursive(track->subtracks);
    }
}

void uph_audio_engine_stop_all_notes(void)
{
    uph_stop_all_notes_recursive(uph_state.project.tracks);
}