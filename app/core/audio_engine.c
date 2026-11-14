#ifndef UPH_INVALID_TIMELINE_BLOCK
#define UPH_INVALID_TIMELINE_BLOCK NULL
#endif

enum
{
    UPH_RENDER_RUNNING,
    UPH_RENDER_REQUESTED,
    UPH_RENDER_PARKED
};

#define UPH_WSOLA_GRAIN_FRAMES  2048
#define UPH_WSOLA_HOP_FRAMES    (UPH_WSOLA_GRAIN_FRAMES / 2)
#define UPH_WSOLA_TOLERANCE     512
#define UPH_WSOLA_COARSE_STEP   4
#define UPH_WSOLA_RING_FRAMES   4096
#define UPH_WSOLA_RING_MASK     (UPH_WSOLA_RING_FRAMES - 1)
#define UPH_WSOLA_MAX_STATES    32

#define UPH_WSOLA_COARSE_REF    (UPH_WSOLA_HOP_FRAMES / UPH_WSOLA_COARSE_STEP)
#define UPH_WSOLA_COARSE_CANDS  (2 * UPH_WSOLA_TOLERANCE / UPH_WSOLA_COARSE_STEP + 1)

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
    bool        reversed;

    double      next_out_pos;
    double      q;
    double      anchor_q;
    double      anchor_src;
    uint64_t    last_callback;
    bool        follow;

    int64_t     next_grain;
    int64_t     complete_up_to;
    int64_t     prev_start;
    bool        has_prev;

    float       ring_l[UPH_WSOLA_RING_FRAMES];
    float       ring_r[UPH_WSOLA_RING_FRAMES];
}
Uph_Stretcher;

typedef struct
{
    ma_context context;
    bool context_ready;
    Uph_AudioDriver context_driver;
    bool duplex;
    ma_device device;
    ma_device capture_device;
    ma_pcm_rb input_rb;
    bool capture_ready;
    bool input_primed;

    _Atomic int render_state;
    int render_lock_depth;
    bool render_lock_stopped_device;
    float render_gain;

    float stretch_window[UPH_WSOLA_GRAIN_FRAMES];
    Uph_Stretcher stretchers[UPH_WSOLA_MAX_STATES];
    uint64_t stretcher_clock;
    uint64_t stretcher_callback;
}
Uph_AudioEngineData;

static Uph_AudioEngineData uph_audio_engine_data = {
    .render_state = UPH_RENDER_RUNNING,
    .render_gain = 1.0f
};

static void uph_read_sample_frame(const Uph_SampleData *sample, double pos, bool reversed, float *out_left, float *out_right)
{
    if (pos < 0.0 || pos >= (double)sample->frame_count || !sample->frames)
    {
        *out_left = 0.0f;
        *out_right = 0.0f;
        return;
    }

    if (reversed)
        pos = NAUI_MAX((double)(sample->frame_count - 1) - pos, 0.0);

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

static void uph_stretch_init_window(void)
{
    for (uint32_t i = 0; i < UPH_WSOLA_GRAIN_FRAMES; i++)
    {
        uph_audio_engine_data.stretch_window[i] = 0.5f - 0.5f * cosf(
            2.0f * (float)NAUI_PI * (float)i / (float)UPH_WSOLA_GRAIN_FRAMES);
    }
}

static inline float uph_sample_mono_at(const Uph_SampleData *s, int64_t i, bool reversed)
{
    if (i < 0 || (uint64_t)i >= s->frame_count || !s->frames)
        return 0.0f;

    if (reversed)
        i = (int64_t)s->frame_count - 1 - i;

    const float *src = (const float*)s->frames;
    if (s->channel_type == UPH_SAMPLE_STEREO)
        return 0.5f * (src[i * 2 + 0] + src[i * 2 + 1]);
    return src[i];
}

static inline void uph_sample_stereo_at(const Uph_SampleData *s, int64_t i, bool reversed, float *l, float *r)
{
    if (i < 0 || (uint64_t)i >= s->frame_count || !s->frames)
    {
        *l = 0.0f;
        *r = 0.0f;
        return;
    }

    if (reversed)
        i = (int64_t)s->frame_count - 1 - i;

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

static int64_t uph_wsola_find_start(const Uph_SampleData *s, bool reversed, int64_t ref_start, int64_t nominal, int64_t min_start)
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
            acc += uph_sample_mono_at(s, ref_start + (int64_t)m * STEP + u, reversed);
        dref[m] = acc;
    }
    for (int m = 0; m < CANDS + REF_N - 1; m++)
    {
        float acc = 0.0f;
        for (int u = 0; u < STEP; u++)
            acc += uph_sample_mono_at(s, base + (int64_t)m * STEP + u, reversed);
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
        ref[i] = uph_sample_mono_at(s, ref_start + i, reversed);

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
            float c = uph_sample_mono_at(s, a + i, reversed);
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
        nominal = (int64_t)llround(st->anchor_src + ((double)(k + 1) * (double)hop - st->anchor_q) * st->beta) - hop;
    if (nominal < start)
        nominal = start;

    int64_t a = nominal;
    if (st->has_prev)
        a = uph_wsola_find_start(sample, st->reversed, st->prev_start + hop, nominal, start);

    const int64_t y0 = k * hop;

    for (int64_t i = hop; i < grain; i++)
    {
        const int64_t idx = (y0 + i) & UPH_WSOLA_RING_MASK;
        st->ring_l[idx] = 0.0f;
        st->ring_r[idx] = 0.0f;
    }

    for (int64_t i = 0; i < grain; i++)
    {
        const float w = (k == 0 && i < hop) ? 1.0f : uph_audio_engine_data.stretch_window[i];

        float l, r;
        uph_sample_stereo_at(sample, a + i, st->reversed, &l, &r);

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
    st->anchor_q     = 0.0;
    st->anchor_src   = (double)llround(st->start_offset_frames);

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
    Uph_Stretcher *oldest = &uph_audio_engine_data.stretchers[0];

    for (int i = 0; i < UPH_WSOLA_MAX_STATES; i++)
    {
        Uph_Stretcher *st = &uph_audio_engine_data.stretchers[i];
        if (st->key == key)
        {
            st->last_used = ++uph_audio_engine_data.stretcher_clock;
            return st;
        }
        if (st->last_used < oldest->last_used)
            oldest = st;
    }

    oldest->key       = key;
    oldest->valid     = false;
    oldest->last_used = ++uph_audio_engine_data.stretcher_clock;
    return oldest;
}

static void uph_stretcher_prepare(
    Uph_Stretcher *st,
    const Uph_SampleData *data,
    bool reversed,
    double start_offset_frames,
    double beta,
    double read_rate,
    double out_pos
)
{
    const bool same = st->valid
        && st->frames == data->frames
        && st->reversed == reversed
        && st->frame_count == data->frame_count
        && st->start_offset_frames == start_offset_frames
        && st->beta == beta
        && st->read_rate == read_rate
        && !st->follow
        && fabs(st->next_out_pos - out_pos) < 1.5;

    if (same)
        return;

    st->valid               = true;
    st->frames              = data->frames;
    st->frame_count         = data->frame_count;
    st->reversed            = reversed;
    st->start_offset_frames = start_offset_frames;
    st->beta                = beta;
    st->read_rate           = read_rate;
    st->follow              = false;

    uph_stretcher_reset(st, out_pos);
}

static void uph_stretcher_prepare_follow(
    Uph_Stretcher *st,
    const Uph_SampleData *data,
    bool reversed,
    double start_offset_frames,
    double beta,
    double read_rate,
    double out_pos,
    double src_pos
)
{
    const double predicted_src = st->anchor_src + (st->q - st->anchor_q) * st->beta;
    const bool continuing = st->valid
        && st->follow
        && st->frames == data->frames
        && st->reversed == reversed
        && st->frame_count == data->frame_count
        && st->start_offset_frames == start_offset_frames
        && st->last_callback + 1 == uph_audio_engine_data.stretcher_callback
        && fabs(predicted_src - src_pos) < (double)UPH_WSOLA_GRAIN_FRAMES;

    st->last_callback = uph_audio_engine_data.stretcher_callback;

    st->beta      = beta;
    st->read_rate = read_rate;

    if (!continuing)
    {
        st->valid               = true;
        st->follow              = true;
        st->frames              = data->frames;
        st->frame_count         = data->frame_count;
        st->reversed            = reversed;
        st->start_offset_frames = start_offset_frames;

        uph_stretcher_reset(st, out_pos);
    }

    st->anchor_q   = st->q;
    st->anchor_src = src_pos;
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

static double uph_sample_block_beat_to_seconds(double beat, float fixed_bpm)
{
    return fixed_bpm > 0.0f ? beat * 60.0 / (double)fixed_bpm : uph_tempo_beat_to_seconds(beat);
}

static double uph_sample_block_seconds_to_beat(double seconds, float fixed_bpm)
{
    return fixed_bpm > 0.0f ? seconds * (double)fixed_bpm / 60.0 : uph_tempo_seconds_to_beat(seconds);
}

static void uph_render_sample_block(
    const Uph_TimelineBlock *block,
    const Uph_Sample *sample,
    const Uph_SampleData *data,
    double buffer_start_beat,
    ma_uint32 frame_count,
    uint32_t engine_sample_rate,
    float fixed_bpm,
    float *track_l,
    float *track_r
)
{
    const double sample_rate = (double)engine_sample_rate;

    const double time_scale  = (sample->time_scale > 0.0f) ? (double)sample->time_scale : 1.0;
    const double pitch_ratio = pow(2.0, (double)sample->pitch / 1200.0);
    const float gain = uph_sample_gain(sample, data);

    const double block_start_seconds = uph_sample_block_beat_to_seconds(block->start_beat, fixed_bpm);
    const double start_offset_frames = (block_start_seconds - uph_sample_block_beat_to_seconds(block->start_beat - block->start_offset_beats, fixed_bpm)) * sample_rate;
    const double block_length_frames = (uph_sample_block_beat_to_seconds(block->start_beat + block->length_beats, fixed_bpm) - block_start_seconds) * sample_rate;
    const double first_out_pos       = (uph_sample_block_beat_to_seconds(buffer_start_beat, fixed_bpm) - block_start_seconds) * sample_rate;

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

    if (uph_sample_follows_tempo(sample))
    {
        const double frames_per_beat = 60.0 / (uph_sample_tempo_bpm(sample) * time_scale) * sample_rate;
        const double offset_frames = block->start_offset_beats * frames_per_beat;
        const double buffer_start_seconds = uph_sample_block_beat_to_seconds(buffer_start_beat, fixed_bpm);
        const double first_beat = (f == 0) ? buffer_start_beat : uph_sample_block_seconds_to_beat(buffer_start_seconds + (double)f / sample_rate, fixed_bpm);
        const double last_beat = uph_sample_block_seconds_to_beat(buffer_start_seconds + (double)frame_count / sample_rate, fixed_bpm);
        const double src_start = offset_frames + (first_beat - block->start_beat) * frames_per_beat;
        const double src_rate = (last_beat - first_beat) * frames_per_beat / (double)(frame_count - f);
        const uint32_t f0 = f;
        const bool resample = sample->mode == UPH_SAMPLE_RESAMPLE;
        const double follow_read_rate = (resample ? src_rate : 1.0) * pitch_ratio;

        Uph_Stretcher *follow_stretcher = NULL;
        if (sample->pitch != 0 || !resample)
        {
            follow_stretcher = uph_stretcher_acquire(block);
            uph_stretcher_prepare_follow(
                follow_stretcher, data, sample->reversed,
                offset_frames, src_rate / follow_read_rate, follow_read_rate,
                first_out_pos + (double)f, src_start
            );
        }

        for (; f < frame_count; f++)
        {
            if (first_out_pos + (double)f >= block_length_frames)
                break;

            float left, right;
            if (follow_stretcher)
                uph_stretcher_read(follow_stretcher, data, &left, &right);
            else
                uph_read_sample_frame(data, src_start + (double)(f - f0) * src_rate, sample->reversed, &left, &right);

            track_l[f] += left * gain;
            track_r[f] += right * gain;
        }
        return;
    }

    const double speed = 1.0 / time_scale;

    const double read_rate = ((sample->mode == UPH_SAMPLE_RESAMPLE) ? speed : 1.0) * pitch_ratio;

    const bool use_wsola = (sample->pitch != 0)
        || (sample->mode == UPH_SAMPLE_STRETCH && time_scale != 1.0);

    Uph_Stretcher *stretcher = NULL;
    if (use_wsola)
    {
        stretcher = uph_stretcher_acquire(block);
        uph_stretcher_prepare(
            stretcher, data, sample->reversed,
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
            uph_read_sample_frame(data, start_offset_frames + out_pos * speed, sample->reversed, &left, &right);

        track_l[f] += left * gain;
        track_r[f] += right * gain;
    }
}

double uph_sample_playback_length_beats(const Uph_Sample *sample, const Uph_SampleData *data, float bpm)
{
    if (bpm <= 0.0f || data->frame_count == 0)
        return 0.0;

    const double time_scale = uph_sample_effective_time_scale(sample, (double)bpm);
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

#define UPH_PEDAL_FRAME_MAX_CHANGES 128

typedef struct
{
    uint32_t offset;
    uint8_t value;
    uint8_t priority;
    Uph_MidiPedalType type;
}
Uph_PedalChange;

typedef struct
{
    uint8_t start_value[UPH_MIDI_PEDAL_COUNT];
    Uph_PedalChange changes[UPH_PEDAL_FRAME_MAX_CHANGES];
    uint32_t change_count;
}
Uph_PedalFrame;

static uint32_t uph_pedal_beat_to_offset(double beat, double buffer_start_beat, uint32_t engine_sample_rate, float bpm)
{
    const double seconds = uph_beats_to_seconds(fmax(0.0, beat - buffer_start_beat), bpm);
    return (uint32_t)(seconds * (double)engine_sample_rate);
}

static void uph_pedal_frame_push(Uph_PedalFrame *frame, Uph_PedalChange change)
{
    if (frame->change_count >= UPH_PEDAL_FRAME_MAX_CHANGES)
        return;

    uint32_t i = frame->change_count++;
    while (i > 0)
    {
        const Uph_PedalChange *prev = &frame->changes[i - 1];
        if (prev->offset < change.offset || (prev->offset == change.offset && prev->priority <= change.priority))
            break;

        frame->changes[i] = *prev;
        i--;
    }
    frame->changes[i] = change;
}

static void uph_collect_pattern_block_pedals(
    const Uph_TimelineBlock *block,
    double buffer_start_beat,
    double buffer_end_beat,
    uint32_t engine_sample_rate,
    float bpm,
    Uph_PedalFrame *frame
)
{
    Uph_Project *project = &uph_state.project;
    if (block->resource_index >= naui_list_len(project->midi_patterns))
        return;

    const Uph_MidiPattern *pattern = &project->midi_patterns[block->resource_index];
    const uint64_t pedal_count = naui_list_len(pattern->pedals);
    if (pedal_count == 0)
        return;

    const double block_end_beat = block->start_beat + block->length_beats;
    if (block_end_beat < buffer_start_beat || block->start_beat >= buffer_end_beat)
        return;

    bool used[UPH_MIDI_PEDAL_COUNT] = { false };
    for (uint64_t i = 0; i < pedal_count; i++)
    {
        if (pattern->pedals[i].type < UPH_MIDI_PEDAL_COUNT)
            used[pattern->pedals[i].type] = true;
    }

    if (block->start_beat <= buffer_start_beat && buffer_start_beat < block_end_beat)
    {
        const double local_beat = buffer_start_beat - block->start_beat + block->start_offset_beats;
        for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
        {
            const uint8_t value = uph_midi_pedal_value_before(pattern, type, local_beat, false);
            if (value > frame->start_value[type])
                frame->start_value[type] = value;
        }
    }
    else if (block->start_beat > buffer_start_beat)
    {
        const uint32_t offset = uph_pedal_beat_to_offset(block->start_beat, buffer_start_beat, engine_sample_rate, bpm);
        for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
        {
            const uint8_t value = uph_midi_pedal_value_before(pattern, type, block->start_offset_beats, false);
            if (value > 0)
                uph_pedal_frame_push(frame, (Uph_PedalChange){ .offset = offset, .value = value, .priority = 1, .type = type });
        }
    }

    for (uint64_t i = 0; i < pedal_count; i++)
    {
        const Uph_MidiPedalEvent *pedal = &pattern->pedals[i];
        const double beat = block->start_beat + (pedal->beat - block->start_offset_beats);
        if (beat < block->start_beat || beat >= block_end_beat || beat < buffer_start_beat || beat >= buffer_end_beat || pedal->type >= UPH_MIDI_PEDAL_COUNT)
            continue;

        uph_pedal_frame_push(frame, (Uph_PedalChange){
            .offset = uph_pedal_beat_to_offset(beat, buffer_start_beat, engine_sample_rate, bpm),
            .value = pedal->value,
            .priority = 1,
            .type = pedal->type
        });
    }

    if (block_end_beat >= buffer_start_beat && block_end_beat < buffer_end_beat)
    {
        const uint32_t offset = uph_pedal_beat_to_offset(block_end_beat, buffer_start_beat, engine_sample_rate, bpm);
        for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
        {
            if (used[type])
                uph_pedal_frame_push(frame, (Uph_PedalChange){ .offset = offset, .value = 0, .priority = 0, .type = type });
        }
    }
}

static void uph_flush_track_pedals(Uph_Track *track, const Uph_PedalFrame *frame)
{
    Uph_Plugin *plugin = &track->instrument;
    const bool monitors = uph_track_monitors_instrument(track);

    uint8_t live[UPH_MIDI_PEDAL_COUNT] = { 0 };
    for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
    {
        if (monitors)
            live[type] = uph_midi_recorder_pedal_value(type);

        const uint8_t cc = uph_midi_pedal_cc(type);
        const uint8_t target = NAUI_MAX(frame->start_value[type], live[type]);
        if (uph_plugin_cc_value(plugin, cc) != target)
            uph_plugin_queue_midi_cc(plugin, 0, cc, target, 0);
    }

    for (uint32_t i = 0; i < frame->change_count; i++)
    {
        const Uph_PedalChange *change = &frame->changes[i];
        const uint8_t cc = uph_midi_pedal_cc(change->type);
        const uint8_t value = NAUI_MAX(change->value, live[change->type]);
        if (uph_plugin_cc_value(plugin, cc) != value)
            uph_plugin_queue_midi_cc(plugin, 0, cc, value, change->offset);
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
        if (sub->type != UPH_RESOURCE_AUTOMATION || sub->automation_target != UPH_AUTOMATION_TARGET_PLUGIN_PARAM)
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

#define UPH_MIX_AUTOMATION_STEP 32

static bool uph_automation_lane_has_content(const Uph_Track *lane)
{
    const Uph_Project *project = &uph_state.project;
    uint64_t block_count = naui_list_len(lane->blocks);
    for (uint64_t b = 0; b < block_count; b++)
    {
        const Uph_TimelineBlock *block = &lane->blocks[b];
        if (block->resource_index < naui_list_len(project->automations) &&
            naui_list_len(project->automations[block->resource_index].points) > 0)
            return true;
    }
    return false;
}

static const Uph_Track *uph_track_find_automation_lane(const Uph_Track *track, Uph_AutomationTarget target)
{
    uint64_t subtrack_count = naui_list_len(track->subtracks);
    for (uint64_t s = 0; s < subtrack_count; s++)
    {
        const Uph_Track *sub = &track->subtracks[s];
        if (sub->type == UPH_RESOURCE_AUTOMATION && sub->automation_target == target && !(sub->state & UPH_TRACK_MUTED) && uph_automation_lane_has_content(sub))
            return sub;
    }
    return NULL;
}

static bool uph_automation_lane_value_at_beat(const Uph_Track *lane, double beat, float *out_value)
{
    if (!lane)
        return false;

    const Uph_Project *project = &uph_state.project;
    const Uph_TimelineBlock *previous = NULL;
    double previous_end = 0.0;
    uint64_t block_count = naui_list_len(lane->blocks);
    for (uint64_t b = 0; b < block_count; b++)
    {
        const Uph_TimelineBlock *block = &lane->blocks[b];
        if (block->resource_index >= naui_list_len(project->automations))
            continue;
        if (naui_list_len(project->automations[block->resource_index].points) == 0)
            continue;

        double block_end = block->start_beat + block->length_beats;
        if (beat >= block->start_beat && beat < block_end)
        {
            double automation_beat = (beat - block->start_beat) + block->start_offset_beats;
            return uph_automation_value_at_beat(&project->automations[block->resource_index], automation_beat, out_value);
        }

        if (block_end <= beat && (!previous || block_end > previous_end))
        {
            previous = block;
            previous_end = block_end;
        }
    }

    if (!previous)
        return false;

    double automation_beat = previous->length_beats + previous->start_offset_beats;
    return uph_automation_value_at_beat(&project->automations[previous->resource_index], automation_beat, out_value);
}

static void uph_pan_gains(float volume, float pan, float *out_left, float *out_right)
{
    float clamped_pan = NAUI_CLAMP(pan, -1.0f, 1.0f);
    float pan_angle = (clamped_pan + 1.0f) * 0.25f * (float)NAUI_PI;
    *out_left  = volume * cosf(pan_angle) * NAUI_SQRT2;
    *out_right = volume * sinf(pan_angle) * NAUI_SQRT2;
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

bool uph_track_mix_automation_value(const Uph_Track *lanes_owner, Uph_AutomationTarget target, double beat, float *out_value)
{
    if (!lanes_owner)
        return false;

    float value;
    if (!uph_automation_lane_value_at_beat(uph_track_find_automation_lane(lanes_owner, target), beat, &value))
        return false;

    *out_value = target == UPH_AUTOMATION_TARGET_TRACK_PAN ? uph_automation_to_pan(value) : uph_automation_to_volume(value);
    return true;
}

static bool uph_track_has_mix_lanes(const Uph_Track *lanes_owner)
{
    return lanes_owner && (uph_track_find_automation_lane(lanes_owner, UPH_AUTOMATION_TARGET_TRACK_VOLUME) || uph_track_find_automation_lane(lanes_owner, UPH_AUTOMATION_TARGET_TRACK_PAN));
}

static bool uph_mix_automation_gains(
    const Uph_RenderContext *ctx,
    const Uph_Track *lanes_owner,
    float volume,
    float pan,
    float *gains_left,
    float *gains_right
)
{
    if (!ctx->timeline_playing || !uph_track_has_mix_lanes(lanes_owner))
        return false;

    const ma_uint32 frame_count = ctx->frame_count;
    float prev_left = 0.0f, prev_right = 0.0f;
    uint32_t prev_offset = 0;
    for (uint32_t offset = 0;; offset += UPH_MIX_AUTOMATION_STEP)
    {
        if (offset > frame_count)
            offset = frame_count;

        double beat = ctx->buffer_start_beat
            + uph_seconds_to_beats((double)offset / (double)ctx->engine_sample_rate, ctx->bpm);

        float point_volume, point_pan;
        if (!uph_track_mix_automation_value(lanes_owner, UPH_AUTOMATION_TARGET_TRACK_VOLUME, beat, &point_volume))
            point_volume = volume;
        if (!uph_track_mix_automation_value(lanes_owner, UPH_AUTOMATION_TARGET_TRACK_PAN, beat, &point_pan))
            point_pan = pan;

        float point_left, point_right;
        uph_pan_gains(point_volume, point_pan, &point_left, &point_right);

        if (offset > 0)
        {
            const float span = (float)(offset - prev_offset);
            for (uint32_t f = prev_offset; f < offset; f++)
            {
                const float t = (float)(f - prev_offset) / span;
                gains_left[f] = prev_left + (point_left - prev_left) * t;
                gains_right[f] = prev_right + (point_right - prev_right) * t;
            }
        }

        prev_left = point_left;
        prev_right = point_right;
        prev_offset = offset;

        if (offset == frame_count)
            break;
    }

    return true;
}

static void uph_queue_preview_notes(const Uph_RenderContext *ctx, Uph_Track *track, bool should_be_held[128], Uph_PedalFrame *pedal_frame)
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
        uph_collect_pattern_block_pedals(&block, range.start_beat, range.end_beat, ctx->engine_sample_rate, ctx->bpm, pedal_frame);

        if (!range.repeat)
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


    const bool audible = uph_track_is_audible(project, track);
    if (!audible && track->type != UPH_RESOURCE_PATTERN && track->type != UPH_RESOURCE_GROUP)
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

    float gain_left, gain_right;
    uph_pan_gains(volume, pan, &gain_left, &gain_right);

    const bool has_mix_lanes = ctx->timeline_playing && uph_track_has_mix_lanes(track);
    float *gains_left = has_mix_lanes ? alloca(max_frame_count * sizeof(float)) : NULL;
    float *gains_right = has_mix_lanes ? alloca(max_frame_count * sizeof(float)) : NULL;
    if (!uph_mix_automation_gains(ctx, track, volume, pan, gains_left, gains_right))
    {
        gains_left = NULL;
        gains_right = NULL;
    }

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
                    ctx->playhead_start_beat, frame_count, ctx->engine_sample_rate, 0.0f,
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
            Uph_PedalFrame pedal_frame = { 0 };

            uph_queue_preview_notes(ctx, track, should_be_held, &pedal_frame);

            if (ctx->timeline_playing && audible)
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

                    uph_collect_pattern_block_pedals(
                        block,
                        ctx->buffer_start_beat, ctx->buffer_end_beat,
                        ctx->engine_sample_rate, ctx->bpm,
                        &pedal_frame
                    );
                }
            }

            for (int key = 0; key < 128; key++)
            {
                if (uph_plugin_note_active(&track->instrument, (uint8_t)key) && !should_be_held[key] && !(uph_track_monitors_instrument(track) && uph_midi_recorder_key_held((uint8_t)key)))
                    uph_plugin_queue_note_event(&track->instrument, false, (uint8_t)key, 0, 0, 0);
            }

            uph_flush_track_pedals(track, &pedal_frame);

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
        float out_left  = track_l[f] * (gains_left ? gains_left[f] : gain_left);
        float out_right = track_r[f] * (gains_right ? gains_right[f] : gain_right);

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
    uph_audio_engine_data.stretcher_callback++;

    Uph_Project *project = &uph_state.project;
    if (project->bpm <= 0.0f || frame_count == 0)
        return;

    const double buffer_seconds = (double)frame_count / (double)engine_sample_rate;
    double buffer_start_beat = playhead_start_beat;
    double buffer_end_beat = uph_tempo_beat_after_seconds(playhead_start_beat, buffer_seconds);
    float bpm = (float)((buffer_end_beat - buffer_start_beat) * 60.0 / buffer_seconds);
    if (bpm <= 0.0f)
        return;

    uph_tempo_set_current_bpm(uph_tempo_bpm_at(playhead_start_beat));

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
    {
        if (project->tracks[t].type == UPH_RESOURCE_AUTOMATION || project->tracks[t].type == UPH_RESOURCE_MASTER)
            continue;
        uph_render_track(&ctx, &project->tracks[t], master_l, master_r);
    }

    Uph_Track *master = uph_resources_master_track();
    if (!master)
        return;

    if (timeline_playing)
        uph_queue_track_automation(master, buffer_start_beat, buffer_end_beat, engine_sample_rate, bpm);

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
    float master_gain_left, master_gain_right;
    uph_pan_gains(master_volume, master_pan, &master_gain_left, &master_gain_right);

    const bool master_has_mix_lanes = timeline_playing && uph_track_has_mix_lanes(master);
    float *master_gains_left = master_has_mix_lanes ? alloca(max_frame_count * sizeof(float)) : NULL;
    float *master_gains_right = master_has_mix_lanes ? alloca(max_frame_count * sizeof(float)) : NULL;
    if (!uph_mix_automation_gains(&ctx, master, master_volume, master_pan, master_gains_left, master_gains_right))
    {
        master_gains_left = NULL;
        master_gains_right = NULL;
    }

    float master_peak_left  = 0.0f;
    float master_peak_right = 0.0f;

    for (ma_uint32 f = 0; f < frame_count; f++)
    {
        float out_left  = master_l[f] * (master_gains_left ? master_gains_left[f] : master_gain_left);
        float out_right = master_r[f] * (master_gains_right ? master_gains_right[f] : master_gain_right);

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
    if (uph_audio_engine_data.capture_ready)
        ma_device_stop(&uph_audio_engine_data.capture_device);
}

void uph_audio_engine_start(void)
{
    if (uph_audio_engine_data.capture_ready)
    {
        uph_audio_engine_data.input_primed = false;
        ma_device_start(&uph_audio_engine_data.capture_device);
    }
    ma_device_start(&uph_audio_engine_data.device);
}

void uph_audio_engine_lock(void)
{
    if (uph_audio_engine_data.render_lock_depth++ > 0)
        return;

    if (!ma_device_is_started(&uph_audio_engine_data.device))
    {
        atomic_store(&uph_audio_engine_data.render_state, UPH_RENDER_PARKED);
        return;
    }

    atomic_store(&uph_audio_engine_data.render_state, UPH_RENDER_REQUESTED);

    const float deadline = naui_time() + 0.25f;
    while (atomic_load(&uph_audio_engine_data.render_state) != UPH_RENDER_PARKED)
    {
        if (naui_time() > deadline)
        {
            fprintf(stderr, "uph_audio_engine_lock: audio thread did not park in time, stopping device\n");
            ma_device_stop(&uph_audio_engine_data.device);
            uph_audio_engine_data.render_lock_stopped_device = true;
            uph_audio_engine_data.render_gain = 0.0f;
            atomic_store(&uph_audio_engine_data.render_state, UPH_RENDER_PARKED);
            break;
        }

        ma_sleep(0);
    }
}

void uph_audio_engine_unlock(void)
{
    if (uph_audio_engine_data.render_lock_depth == 0 || --uph_audio_engine_data.render_lock_depth > 0)
        return;

    atomic_store(&uph_audio_engine_data.render_state, UPH_RENDER_RUNNING);

    if (uph_audio_engine_data.render_lock_stopped_device)
    {
        uph_audio_engine_data.render_lock_stopped_device = false;
        ma_device_start(&uph_audio_engine_data.device);
    }
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

    float abs_peak = (old_frame_count == 0) ? 0.0f : sample_data->peak;
    double sum_squares = (old_frame_count == 0) ? 0.0 : sample_data->sum_squares;

    for (uint64_t bin = first_bin; bin < bin_count; bin++)
    {
        uint64_t frame_start = bin * max_frame_count;
        uint64_t frame_end = frame_start + max_frame_count;
        if (frame_end > sample_data->frame_count)
            frame_end = sample_data->frame_count;

        float min_v = 1.0f;
        float max_v = -1.0f;
        Uph_ChannelPeak channel_peaks[2] = { { FLT_MAX, -FLT_MAX }, { FLT_MAX, -FLT_MAX } };

        for (uint64_t f = frame_start; f < frame_end; f++)
        {
            float value = 0.0f;
            for (int c = 0; c < channel_count; c++)
            {
                const float s = samples[f * channel_count + c];
                const float a = fabsf(s);
                if (a > abs_peak) abs_peak = a;
                if (f >= old_frame_count) sum_squares += (double)s * (double)s;
                if (s < channel_peaks[c].min) channel_peaks[c].min = s;
                if (s > channel_peaks[c].max) channel_peaks[c].max = s;
                value += s;
            }
            value /= (float)channel_count;

            if (value < min_v) min_v = value;
            if (value > max_v) max_v = value;
        }

        for (int c = 0; c < channel_count; c++)
        {
            const uint64_t index = bin * (uint64_t)channel_count + (uint64_t)c;
            if (index < naui_list_len(sample_data->channel_peaks))
                sample_data->channel_peaks[index] = channel_peaks[c];
            else
                naui_list_push(sample_data->channel_peaks, channel_peaks[c]);
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

    sample_data->peak = abs_peak;
    sample_data->sum_squares = sum_squares;
}

float uph_sample_gain(const Uph_Sample *sample, const Uph_SampleData *data)
{
    const float gain = sample->gain;
    if (!sample->normalized || data->peak <= 1e-6f)
        return gain;
    return gain / data->peak;
}

bool uph_sample_follows_tempo(const Uph_Sample *sample)
{
    return sample->follow_tempo;
}

double uph_sample_tempo_bpm(const Uph_Sample *sample)
{
    if (sample->tempo_bpm > 0.0f)
        return (double)sample->tempo_bpm;
    return uph_state.project.bpm > 0.0f ? (double)uph_state.project.bpm : 120.0;
}

double uph_sample_effective_time_scale(const Uph_Sample *sample, double bpm)
{
    const double time_scale = (sample->time_scale > 0.0f) ? (double)sample->time_scale : 1.0;
    if (!uph_sample_follows_tempo(sample) || bpm <= 0.0)
        return time_scale;
    return time_scale * uph_sample_tempo_bpm(sample) / bpm;
}

double uph_sample_block_length_beats(const Uph_Sample *sample, const Uph_SampleData *data, double start_beat)
{
    uint32_t rate = uph_audio_engine_data.device.sampleRate;
    if (rate == 0)
        rate = uph_state.settings.audio.sample_rate;
    if (rate == 0 || data->frame_count == 0)
        return 0.0;

    const double source_seconds = (double)data->frame_count / (double)rate;
    const double time_scale = (sample->time_scale > 0.0f) ? (double)sample->time_scale : 1.0;
    if (uph_sample_follows_tempo(sample))
        return source_seconds * uph_sample_tempo_bpm(sample) * time_scale / 60.0;

    return uph_tempo_seconds_to_beat(uph_tempo_beat_to_seconds(start_beat) + source_seconds * time_scale) - start_beat;
}

double uph_sample_block_source_seconds(const Uph_Sample *sample, double block_start_beat, double start_offset_beats, double beat)
{
    const double time_scale = (sample->time_scale > 0.0f) ? (double)sample->time_scale : 1.0;
    if (uph_sample_follows_tempo(sample))
        return (beat - block_start_beat + start_offset_beats) * 60.0 / (uph_sample_tempo_bpm(sample) * time_scale);

    const double block_start_seconds = uph_tempo_beat_to_seconds(block_start_beat);
    const double offset_seconds = block_start_seconds - uph_tempo_beat_to_seconds(block_start_beat - start_offset_beats);
    return (uph_tempo_beat_to_seconds(beat) - block_start_seconds) / time_scale + offset_seconds;
}

static void uph_build_waveform_peaks(Uph_SampleData *sample_data)
{
    if (!sample_data->frames || sample_data->frame_count == 0)
        return;

    const uint32_t max_frame_count = UPH_WAVEFORM_BIN_FRAMES;
    const uint64_t bin_count = (sample_data->frame_count + max_frame_count - 1) / max_frame_count;
    naui_list_reserve(sample_data->waveform_peaks, bin_count);
    naui_list_reserve(sample_data->channel_peaks, bin_count * (sample_data->channel_type == UPH_SAMPLE_STEREO ? 2 : 1));

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

        block->length_beats = uph_tempo_beat_after_seconds(
            block->start_beat, (double)data->frame_count / (double)sample_rate) - block->start_beat;

        uph_update_waveform_peaks(data, old_frame_count);
    }
}

static void uph_audio_engine_capture_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
    ma_pcm_rb *rb = &uph_audio_engine_data.input_rb;
    const ma_uint32 channels = device->capture.channels;
    const float *src = (const float*)input;

    ma_uint32 done = 0;
    while (done < frame_count)
    {
        ma_uint32 chunk = frame_count - done;
        void *dst;
        if (ma_pcm_rb_acquire_write(rb, &chunk, &dst) != MA_SUCCESS || chunk == 0)
            break;

        memcpy(dst, src + (size_t)done * channels, (size_t)chunk * channels * sizeof(float));
        ma_pcm_rb_commit_write(rb, chunk);
        done += chunk;
    }
}

static void uph_audio_engine_read_input(float *dst, ma_uint32 frame_count, ma_uint32 channels)
{
    ma_pcm_rb *rb = &uph_audio_engine_data.input_rb;
    const ma_uint32 capture_period = uph_audio_engine_data.capture_device.capture.internalPeriodSizeInFrames;
    const ma_uint32 prime_frames = 2 * (frame_count > capture_period ? frame_count : capture_period);

    ma_uint32 available = ma_pcm_rb_available_read(rb);

    if (!uph_audio_engine_data.input_primed)
    {
        if (available < prime_frames)
        {
            memset(dst, 0, (size_t)frame_count * channels * sizeof(float));
            return;
        }
        uph_audio_engine_data.input_primed = true;
    }

    if (available > prime_frames * 2)
        ma_pcm_rb_seek_read(rb, available - prime_frames);

    ma_uint32 done = 0;
    while (done < frame_count)
    {
        ma_uint32 chunk = frame_count - done;
        void *src;
        if (ma_pcm_rb_acquire_read(rb, &chunk, &src) != MA_SUCCESS || chunk == 0)
            break;

        memcpy(dst + (size_t)done * channels, src, (size_t)chunk * channels * sizeof(float));
        ma_pcm_rb_commit_read(rb, chunk);
        done += chunk;
    }

    if (done < frame_count)
    {
        memset(dst + (size_t)done * channels, 0, (size_t)(frame_count - done) * channels * sizeof(float));
        uph_audio_engine_data.input_primed = false;
    }
}

#define UPH_PREVIEW_STREAM_THRESHOLD_SECONDS 30
#define UPH_PREVIEW_STREAM_BUFFER_SECONDS 2
#define UPH_PREVIEW_STREAM_CHUNK_FRAMES 4096
#define UPH_PREVIEW_PEAK_CHUNK_FRAMES (UPH_WAVEFORM_BIN_FRAMES * 64)
#define UPH_PREVIEW_STREAM_IDLE_MS 5

struct Uph_PreviewStream
{
    ma_decoder decoder;
    ma_decoder peak_decoder;
    ma_pcm_rb buffer;
    Naui_Thread thread;
    Naui_Path path;
    float *peak_scratch;
    Uph_WaveformPeak *peaks;
    uint64_t peak_capacity;
    _Atomic uint64_t peak_count;
    uint64_t frame_count;
    uint32_t original_sample_rate;
    Uph_SampleChannelType channel_type;
    atomic_bool stop;
    atomic_bool seek_request;
    atomic_bool end_of_stream;
    bool decoder_ready;
    bool peak_decoder_ready;
    bool buffer_ready;
    bool peaks_done;
};

static struct
{
    volatile ma_spinlock lock;
    Uph_PreviewSource source;
    _Atomic uint64_t position;
    _Atomic bool playing;
}
uph_audio_engine_preview;

static ma_result uph_preview_decoder_init_file(const Naui_Path *path, const ma_decoder_config *config, ma_decoder *decoder)
{
#if defined(_WIN32) || defined(_WIN64)
    wchar_t wide_path[NAUI_PATH_MAX];
    if (MultiByteToWideChar(CP_UTF8, 0, path->data, -1, wide_path, NAUI_PATH_MAX) > 0)
        return ma_decoder_init_file_w(wide_path, config, decoder);
#endif
    return ma_decoder_init_file(path->data, config, decoder);
}

static bool uph_preview_stream_fill_chunk(Uph_PreviewStream *stream)
{
    if (atomic_load_explicit(&stream->end_of_stream, memory_order_acquire))
        return false;

    ma_uint32 frames = UPH_PREVIEW_STREAM_CHUNK_FRAMES;
    void *region = NULL;
    if (ma_pcm_rb_acquire_write(&stream->buffer, &frames, &region) != MA_SUCCESS || frames == 0)
        return false;

    ma_uint64 frames_read = 0;
    const ma_result result = ma_decoder_read_pcm_frames(&stream->decoder, region, frames, &frames_read);
    ma_pcm_rb_commit_write(&stream->buffer, (ma_uint32)frames_read);

    if (result != MA_SUCCESS || frames_read < frames)
        atomic_store_explicit(&stream->end_of_stream, true, memory_order_release);

    return frames_read > 0;
}

static void uph_preview_stream_prefill(Uph_PreviewStream *stream)
{
    while (uph_preview_stream_fill_chunk(stream))
    {
    }
}

static void uph_preview_stream_peaks_chunk(Uph_PreviewStream *stream)
{
    if (!stream->peak_decoder_ready || !stream->peaks || !stream->peak_scratch)
    {
        stream->peaks_done = true;
        return;
    }

    ma_uint64 frames_read = 0;
    const ma_result result = ma_decoder_read_pcm_frames(&stream->peak_decoder, stream->peak_scratch, UPH_PREVIEW_PEAK_CHUNK_FRAMES, &frames_read);

    uint64_t count = atomic_load_explicit(&stream->peak_count, memory_order_relaxed);
    for (uint64_t start = 0; start < frames_read && count < stream->peak_capacity; start += UPH_WAVEFORM_BIN_FRAMES, count++)
    {
        uint64_t end = start + UPH_WAVEFORM_BIN_FRAMES;
        if (end > frames_read)
            end = frames_read;

        float min_v = 1.0f;
        float max_v = -1.0f;
        for (uint64_t f = start; f < end; f++)
        {
            const float value = (stream->peak_scratch[f * 2 + 0] + stream->peak_scratch[f * 2 + 1]) * 0.5f;
            if (value < min_v) min_v = value;
            if (value > max_v) max_v = value;
        }

        stream->peaks[count] = (Uph_WaveformPeak){
            .min = uph_waveform_encode_uint16(min_v),
            .max = uph_waveform_encode_uint16(max_v)
        };
    }
    atomic_store_explicit(&stream->peak_count, count, memory_order_release);

    if (result != MA_SUCCESS || frames_read < UPH_PREVIEW_PEAK_CHUNK_FRAMES || count >= stream->peak_capacity)
        stream->peaks_done = true;
}

static int uph_preview_stream_thread(void *arg)
{
    Uph_PreviewStream *stream = arg;

    while (!atomic_load_explicit(&stream->stop, memory_order_acquire))
    {
        if (atomic_load_explicit(&stream->seek_request, memory_order_acquire))
        {
            ma_pcm_rb_reset(&stream->buffer);
            ma_decoder_seek_to_pcm_frame(&stream->decoder, 0);
            atomic_store_explicit(&stream->end_of_stream, false, memory_order_release);
            uph_preview_stream_prefill(stream);
            atomic_store_explicit(&stream->seek_request, false, memory_order_release);
        }

        if (uph_preview_stream_fill_chunk(stream))
            continue;

        if (!stream->peaks_done)
        {
            uph_preview_stream_peaks_chunk(stream);
            continue;
        }

        naui_thread_sleep_ms(UPH_PREVIEW_STREAM_IDLE_MS);
    }

    return 0;
}

static void uph_preview_stream_free(Uph_PreviewStream *stream)
{
    if (!stream)
        return;

    if (stream->thread)
    {
        atomic_store_explicit(&stream->stop, true, memory_order_release);
        naui_thread_join(stream->thread, NULL);
    }

    if (stream->decoder_ready)
        ma_decoder_uninit(&stream->decoder);

    if (stream->peak_decoder_ready)
        ma_decoder_uninit(&stream->peak_decoder);

    if (stream->buffer_ready)
        ma_pcm_rb_uninit(&stream->buffer);

    free(stream->peak_scratch);
    free(stream->peaks);
    free(stream);
}

static Uph_PreviewStream *uph_preview_stream_open(const Naui_Path path, uint32_t device_rate, uint64_t frame_count, uint32_t original_sample_rate, ma_uint32 native_channels)
{
    Uph_PreviewStream *stream = calloc(1, sizeof(Uph_PreviewStream));
    if (!stream)
        return NULL;

    stream->path = path;
    stream->frame_count = frame_count;
    stream->original_sample_rate = original_sample_rate;
    stream->channel_type = native_channels == 1 ? UPH_SAMPLE_MONO : UPH_SAMPLE_STEREO;
    atomic_init(&stream->peak_count, 0);
    atomic_init(&stream->stop, false);
    atomic_init(&stream->seek_request, false);
    atomic_init(&stream->end_of_stream, false);

    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, device_rate);
    stream->decoder_ready = uph_preview_decoder_init_file(&path, &config, &stream->decoder) == MA_SUCCESS;
    stream->buffer_ready = stream->decoder_ready &&
        ma_pcm_rb_init(ma_format_f32, 2, device_rate * UPH_PREVIEW_STREAM_BUFFER_SECONDS, NULL, NULL, &stream->buffer) == MA_SUCCESS;

    if (!stream->buffer_ready)
    {
        fprintf(stderr, "uph_preview_stream_open: failed to open '%s'\n", path.data);
        uph_preview_stream_free(stream);
        return NULL;
    }

    if (frame_count > 0)
    {
        stream->peak_capacity = (frame_count + UPH_WAVEFORM_BIN_FRAMES - 1) / UPH_WAVEFORM_BIN_FRAMES;
        stream->peaks = calloc((size_t)stream->peak_capacity, sizeof(Uph_WaveformPeak));
        stream->peak_scratch = malloc(UPH_PREVIEW_PEAK_CHUNK_FRAMES * 2 * sizeof(float));
        stream->peak_decoder_ready = stream->peaks && stream->peak_scratch &&
            uph_preview_decoder_init_file(&path, &config, &stream->peak_decoder) == MA_SUCCESS;
    }

    stream->peaks_done = !stream->peak_decoder_ready;
    uph_preview_stream_prefill(stream);
    return stream;
}

Uph_PreviewSource uph_audio_engine_preview_open(const Naui_Path path)
{
    Uph_PreviewSource source = { 0 };
    const uint32_t device_rate = uph_audio_engine_data.device.sampleRate;

    ma_decoder probe;
    const ma_decoder_config probe_config = ma_decoder_config_init(ma_format_f32, 0, 0);
    if (uph_preview_decoder_init_file(&path, &probe_config, &probe) == MA_SUCCESS)
    {
        const uint32_t original_sample_rate = probe.outputSampleRate;
        const ma_uint32 native_channels = probe.outputChannels;
        ma_uint64 native_length = 0;
        ma_decoder_get_length_in_pcm_frames(&probe, &native_length);
        ma_decoder_uninit(&probe);

        const uint64_t frame_count = original_sample_rate > 0
            ? (uint64_t)((double)native_length * (double)device_rate / (double)original_sample_rate)
            : 0;

        if (frame_count == 0 || frame_count > (uint64_t)device_rate * UPH_PREVIEW_STREAM_THRESHOLD_SECONDS)
        {
            source.stream = uph_preview_stream_open(path, device_rate, frame_count, original_sample_rate, native_channels);
            if (source.stream)
                return source;
        }
    }

    Uph_SampleData *data = malloc(sizeof(Uph_SampleData));
    if (!data)
        return source;

    *data = uph_audio_engine_load_sample_data(path);
    if (!uph_audio_engine_sample_data_valid(data))
    {
        uph_audio_engine_unload_sample_data(data);
        free(data);
        return source;
    }

    source.data = data;
    return source;
}

bool uph_audio_engine_preview_source_valid(const Uph_PreviewSource *source)
{
    return source->data || source->stream;
}

void uph_audio_engine_preview_close(Uph_PreviewSource *source)
{
    if (source->data)
    {
        uph_audio_engine_unload_sample_data(source->data);
        free(source->data);
    }

    uph_preview_stream_free(source->stream);
    *source = (Uph_PreviewSource){ 0 };
}

static void uph_audio_engine_preview_mix_stream(Uph_PreviewStream *stream, float *out, ma_uint32 frame_count, float gain)
{
    if (atomic_load_explicit(&stream->seek_request, memory_order_acquire))
        return;

    uint64_t position = atomic_load_explicit(&uph_audio_engine_preview.position, memory_order_relaxed);
    ma_uint32 mixed = 0;
    while (mixed < frame_count)
    {
        ma_uint32 frames = frame_count - mixed;
        void *region = NULL;
        if (ma_pcm_rb_acquire_read(&stream->buffer, &frames, &region) != MA_SUCCESS || frames == 0)
            break;

        const float *samples = region;
        for (ma_uint32 f = 0; f < frames; f++)
        {
            out[(mixed + f) * 2 + 0] += samples[f * 2 + 0] * gain;
            out[(mixed + f) * 2 + 1] += samples[f * 2 + 1] * gain;
        }

        ma_pcm_rb_commit_read(&stream->buffer, frames);
        mixed += frames;
        position += frames;
    }
    atomic_store_explicit(&uph_audio_engine_preview.position, position, memory_order_relaxed);

    if (mixed < frame_count && atomic_load_explicit(&stream->end_of_stream, memory_order_acquire) && ma_pcm_rb_available_read(&stream->buffer) == 0)
        atomic_store_explicit(&uph_audio_engine_preview.playing, false, memory_order_release);
}

static void uph_audio_engine_preview_mix_data(const Uph_SampleData *data, float *out, ma_uint32 frame_count, float gain)
{
    uint64_t position = atomic_load_explicit(&uph_audio_engine_preview.position, memory_order_relaxed);
    const bool stereo = data->channel_type == UPH_SAMPLE_STEREO;
    for (ma_uint32 f = 0; f < frame_count && position < data->frame_count; f++, position++)
    {
        const float left = stereo ? data->frames[position * 2 + 0] : data->frames[position];
        const float right = stereo ? data->frames[position * 2 + 1] : left;
        out[f * 2 + 0] += left * gain;
        out[f * 2 + 1] += right * gain;
    }
    atomic_store_explicit(&uph_audio_engine_preview.position, position, memory_order_relaxed);

    if (position >= data->frame_count)
        atomic_store_explicit(&uph_audio_engine_preview.playing, false, memory_order_release);
}

static void uph_audio_engine_preview_mix(float *out, ma_uint32 frame_count)
{
    if (!atomic_load_explicit(&uph_audio_engine_preview.playing, memory_order_acquire))
        return;

    ma_spinlock_lock(&uph_audio_engine_preview.lock);
    const float gain = NAUI_CLAMP(uph_state.settings.browser.preview_volume, 0.0f, 1.0f);
    const Uph_PreviewSource source = uph_audio_engine_preview.source;

    if (source.data && source.data->frames)
        uph_audio_engine_preview_mix_data(source.data, out, frame_count, gain);
    else if (source.stream)
        uph_audio_engine_preview_mix_stream(source.stream, out, frame_count, gain);
    else
        atomic_store_explicit(&uph_audio_engine_preview.playing, false, memory_order_release);

    ma_spinlock_unlock(&uph_audio_engine_preview.lock);
}

void uph_audio_engine_preview_play(Uph_PreviewSource source)
{
    if (source.stream && !source.stream->thread)
    {
        source.stream->thread = naui_thread_create(uph_preview_stream_thread, source.stream);
        if (!source.stream->thread)
        {
            uph_audio_engine_preview_close(&source);
            return;
        }
    }

    ma_spinlock_lock(&uph_audio_engine_preview.lock);
    Uph_PreviewSource old = uph_audio_engine_preview.source;
    uph_audio_engine_preview.source = source;
    atomic_store_explicit(&uph_audio_engine_preview.position, 0, memory_order_relaxed);
    atomic_store_explicit(&uph_audio_engine_preview.playing, uph_audio_engine_preview_source_valid(&source), memory_order_release);
    ma_spinlock_unlock(&uph_audio_engine_preview.lock);

    uph_audio_engine_preview_close(&old);
}

void uph_audio_engine_preview_restart(void)
{
    ma_spinlock_lock(&uph_audio_engine_preview.lock);
    const Uph_PreviewSource source = uph_audio_engine_preview.source;
    if (source.stream)
        atomic_store_explicit(&source.stream->seek_request, true, memory_order_release);

    atomic_store_explicit(&uph_audio_engine_preview.position, 0, memory_order_relaxed);
    atomic_store_explicit(&uph_audio_engine_preview.playing, uph_audio_engine_preview_source_valid(&source), memory_order_release);
    ma_spinlock_unlock(&uph_audio_engine_preview.lock);
}

void uph_audio_engine_preview_stop(void)
{
    atomic_store_explicit(&uph_audio_engine_preview.playing, false, memory_order_release);
}

bool uph_audio_engine_preview_playing(void)
{
    return atomic_load_explicit(&uph_audio_engine_preview.playing, memory_order_acquire);
}

double uph_audio_engine_preview_progress(void)
{
    const Uph_PreviewSource source = uph_audio_engine_preview.source;
    const uint64_t frame_count = source.data ? source.data->frame_count : source.stream ? source.stream->frame_count : 0;
    if (frame_count == 0)
        return 0.0;

    return (double)atomic_load_explicit(&uph_audio_engine_preview.position, memory_order_relaxed) / (double)frame_count;
}

bool uph_audio_engine_preview_info(Uph_PreviewInfo *out)
{
    const Uph_PreviewSource source = uph_audio_engine_preview.source;
    if (source.data)
    {
        *out = (Uph_PreviewInfo){
            .path = &source.data->file_path,
            .peaks = source.data->waveform_peaks,
            .peak_count = (uint64_t)naui_list_len(source.data->waveform_peaks),
            .peak_total = (uint64_t)naui_list_len(source.data->waveform_peaks),
            .frame_count = source.data->frame_count,
            .original_sample_rate = source.data->original_sample_rate,
            .channel_type = source.data->channel_type,
            .streaming = false
        };
        return true;
    }

    if (source.stream)
    {
        *out = (Uph_PreviewInfo){
            .path = &source.stream->path,
            .peaks = source.stream->peaks,
            .peak_count = atomic_load_explicit(&source.stream->peak_count, memory_order_acquire),
            .peak_total = source.stream->peak_capacity,
            .frame_count = source.stream->frame_count,
            .original_sample_rate = source.stream->original_sample_rate,
            .channel_type = source.stream->channel_type,
            .streaming = true
        };
        return true;
    }

    return false;
}

static void uph_audio_engine_render_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
    static bool was_playing = false;

    const int render_state = atomic_load(&uph_audio_engine_data.render_state);
    if (render_state == UPH_RENDER_PARKED)
    {
        memset(output, 0, (size_t)frame_count * device->playback.channels * sizeof(float));
        return;
    }

    float *out = (float*)output;
    uint32_t engine_sample_rate = device->sampleRate;
    double playhead_start_beat = uph_state.shared.song_timeline_playhead_position;
    bool is_playing = uph_state.shared.song_timeline_playing;
    const double buffer_seconds = (double)frame_count / (double)engine_sample_rate;
    const double buffer_end_beat = uph_tempo_beat_after_seconds(playhead_start_beat, buffer_seconds);
    float bpm = uph_state.project.bpm > 0.0f && buffer_seconds > 0.0
        ? (float)((buffer_end_beat - playhead_start_beat) * 60.0 / buffer_seconds)
        : 0.0f;

    const uint32_t input_channel = uph_state.settings.audio.input_channel;
    const float *input_f = NULL;
    const float *captured = NULL;
    ma_uint32 input_channels = 0;
    const ma_uint32 output_channels = device->playback.channels;

    if (uph_audio_engine_data.duplex)
    {
        if (input)
        {
            captured = (const float*)input;
            input_channels = device->capture.channels;
        }
    }
    else if (uph_audio_engine_data.capture_ready)
    {
        input_channels = uph_audio_engine_data.capture_device.capture.channels;
        float *buffer = alloca(frame_count * input_channels * sizeof(float));
        uph_audio_engine_read_input(buffer, frame_count, input_channels);
        captured = buffer;
    }

    if (input_channels > 0)
    {

        const ma_uint32 channel = input_channel < input_channels ? input_channel : 0;
        float *routed = alloca(frame_count * 2 * sizeof(float));
        for (ma_uint32 f = 0; f < frame_count; f++)
        {
            float s = captured[f * input_channels + channel];
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
    uph_sample_preview_mix(rendered, frame_count, engine_sample_rate, bpm, is_playing);
    uph_audio_engine_preview_mix(rendered, frame_count);

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

    const float gain_from = uph_audio_engine_data.render_gain;
    const float gain_to = render_state == UPH_RENDER_REQUESTED ? 0.0f : 1.0f;
    if (gain_from != 1.0f || gain_to != 1.0f)
    {
        const float gain_step = (gain_to - gain_from) / (float)frame_count;
        for (ma_uint32 f = 0; f < frame_count; f++)
        {
            const float gain = gain_from + gain_step * (float)f;
            for (ma_uint32 c = 0; c < output_channels; c++)
                out[f * output_channels + c] *= gain;
        }
    }
    uph_audio_engine_data.render_gain = gain_to;

    if (is_playing && input_f && bpm > 0.0f)
        uph_record_input(uph_state.project.tracks, input_f, frame_count, engine_sample_rate, bpm);

    if (was_playing && !is_playing)
    {
        uph_audio_engine_stop_all_notes();
		
        if (uph_stop_recording_all(uph_state.project.tracks))
            uph_state.shared.song_timeline_playhead_position = 0.0;
    }

    was_playing = is_playing;

    if (is_playing && bpm > 0.0f)
    {
        double new_beat = buffer_end_beat;
        if (uph_state.project.loop_enabled && !uph_has_active_take(uph_state.project.tracks))
        {
            const Uph_LoopRegion region = uph_state.project.loop_region;
            if (!uph_loop_region_valid(&region))
            {
                if (new_beat >= uph_audio_engine_get_song_length())
                    new_beat = 0.0;
            }
            else if (playhead_start_beat < region.end && new_beat >= region.end)
                new_beat = region.start;
        }

        uph_state.shared.song_timeline_playhead_position = new_beat;
    }

    if (render_state == UPH_RENDER_REQUESTED)
        atomic_store(&uph_audio_engine_data.render_state, UPH_RENDER_PARKED);
}

static void uph_audio_engine_data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
    uph_tempo_map_audio_begin();
    uph_audio_engine_render_callback(device, output, input, frame_count);
    uph_tempo_map_audio_end();
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

static void uph_audio_engine_init_capture_device(ma_context *context, const Uph_AudioSettings *settings, const ma_device_id *capture_id)
{
    uph_audio_engine_data.capture_ready = false;
    uph_audio_engine_data.input_primed = false;

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format     = ma_format_f32;
    config.capture.channels   = 2;
    config.capture.pDeviceID  = (ma_device_id*)capture_id;
    config.capture.shareMode  = settings->exclusive_mode ? ma_share_mode_exclusive : ma_share_mode_shared;
    config.sampleRate         = uph_audio_engine_data.device.sampleRate;
    config.dataCallback       = uph_audio_engine_capture_callback;
    config.performanceProfile = ma_performance_profile_low_latency;
    config.periodSizeInFrames = settings->buffer_size;

    ma_result result = ma_device_init(context, &config, &uph_audio_engine_data.capture_device);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_init: failed to init capture device (%s), recording disabled\n", ma_result_description(result));
        return;
    }

    const ma_uint32 rb_frames = uph_audio_engine_data.device.sampleRate / 2;
    result = ma_pcm_rb_init(ma_format_f32, uph_audio_engine_data.capture_device.capture.channels, rb_frames, NULL, NULL, &uph_audio_engine_data.input_rb);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_init: failed to init input ring buffer (%s), recording disabled\n", ma_result_description(result));
        ma_device_uninit(&uph_audio_engine_data.capture_device);
        return;
    }

    uph_audio_engine_data.capture_ready = true;
}

static bool uph_audio_engine_init_asio_device(const Uph_AudioSettings *settings)
{
    ma_device_id device_id;
    const bool has_device_id = uph_audio_engine_find_device(ma_device_type_playback, settings->asio_device.data, &device_id);

    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.playback.format    = ma_format_f32;
    config.playback.channels  = settings->channels;
    config.playback.pDeviceID = has_device_id ? &device_id : NULL;
    config.capture.format     = ma_format_f32;
    config.capture.channels   = 0; // native channel count so inputs map 1:1 without downmixing
    config.capture.pDeviceID  = has_device_id ? &device_id : NULL;
    config.sampleRate         = settings->sample_rate;
    config.dataCallback       = uph_audio_engine_data_callback;
    config.performanceProfile = ma_performance_profile_low_latency;
    config.periodSizeInFrames = settings->buffer_size;

    ma_result result = ma_device_init(uph_audio_engine_get_context(), &config, &uph_audio_engine_data.device);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_init: failed to init ASIO device (%s)\n", ma_result_description(result));
        return false;
    }

    uph_audio_engine_data.duplex = true;
    ma_device_start(&uph_audio_engine_data.device);
    return true;
}

static bool uph_audio_engine_init_device(void)
{
    Uph_AudioSettings settings = uph_state.settings.audio;

    if (uph_audio_engine_data.context_driver == UPH_AUDIO_DRIVER_ASIO)
        return uph_audio_engine_init_asio_device(&settings);

    ma_device_id playback_id;
    ma_device_id capture_id;
    const bool has_playback_id = uph_audio_engine_find_device(ma_device_type_playback, settings.output_device.name.data, &playback_id);
    const bool has_capture_id = uph_audio_engine_find_device(ma_device_type_capture, settings.input_device.name.data, &capture_id);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format    = ma_format_f32;
    config.playback.channels  = settings.channels;
    config.playback.pDeviceID = has_playback_id ? &playback_id : NULL;
    config.sampleRate         = settings.sample_rate;
    config.dataCallback       = uph_audio_engine_data_callback;

    config.performanceProfile  = ma_performance_profile_low_latency;
    config.playback.shareMode  = settings.exclusive_mode ? ma_share_mode_exclusive : ma_share_mode_shared;
    config.periodSizeInFrames = settings.buffer_size;
    //config.periods            = 2;

    ma_context *context = uph_audio_engine_get_context();

    ma_result result = ma_device_init(context, &config, &uph_audio_engine_data.device);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_init: failed to init playback device (%s)\n", ma_result_description(result));
        return false;
    }

    uph_audio_engine_init_capture_device(context, &settings, has_capture_id ? &capture_id : NULL);

    if (uph_audio_engine_data.capture_ready)
        ma_device_start(&uph_audio_engine_data.capture_device);
    ma_device_start(&uph_audio_engine_data.device);
    return true;
}

static void uph_audio_engine_uninit_device(void)
{
    ma_device_uninit(&uph_audio_engine_data.device);
    uph_audio_engine_data.duplex = false;

    if (uph_audio_engine_data.capture_ready)
    {
        ma_device_uninit(&uph_audio_engine_data.capture_device);
        ma_pcm_rb_uninit(&uph_audio_engine_data.input_rb);
        uph_audio_engine_data.capture_ready = false;
    }
}

static bool uph_audio_engine_init_asio_context(void)
{
    ma_context_config config = ma_context_config_init();
    uph_asio_get_backend_callbacks(&config.custom);

    const ma_backend backends[] = { ma_backend_custom };
    return ma_context_init(backends, 1, &config, &uph_audio_engine_data.context) == MA_SUCCESS;
}

static void uph_audio_engine_init_context(Uph_AudioDriver driver)
{
    if (driver == UPH_AUDIO_DRIVER_ASIO)
    {
        if (uph_asio_supported() && uph_audio_engine_init_asio_context())
        {
            uph_audio_engine_data.context_ready = true;
            uph_audio_engine_data.context_driver = UPH_AUDIO_DRIVER_ASIO;
            return;
        }

        fprintf(stderr, "uph_audio_engine_init: ASIO unavailable, falling back to the system driver\n");
    }

    uph_audio_engine_data.context_driver = UPH_AUDIO_DRIVER_SYSTEM;
    uph_audio_engine_data.context_ready = ma_context_init(NULL, 0, NULL, &uph_audio_engine_data.context) == MA_SUCCESS;

    if (!uph_audio_engine_data.context_ready)
        fprintf(stderr, "uph_audio_engine_init: failed to init context, using a temporary one per device\n");
}

static void uph_audio_engine_uninit_context(void)
{
    if (!uph_audio_engine_data.context_ready)
        return;

    ma_context_uninit(&uph_audio_engine_data.context);
    uph_audio_engine_data.context_ready = false;
}

void uph_audio_engine_init(void)
{
    uph_stretch_init_window();
    uph_audio_engine_init_context(uph_state.settings.audio.driver);

    if (!uph_audio_engine_init_device() && uph_audio_engine_data.context_driver == UPH_AUDIO_DRIVER_ASIO)
    {
        uph_audio_engine_uninit_context();
        uph_audio_engine_init_context(UPH_AUDIO_DRIVER_SYSTEM);
        uph_audio_engine_init_device();
    }
}

void uph_audio_engine_shutdown(void)
{
    uph_audio_engine_render_cancel();
    uph_audio_engine_uninit_device();
    uph_audio_engine_preview_play((Uph_PreviewSource){ 0 });
    uph_audio_engine_uninit_context();
}

ma_context *uph_audio_engine_get_context(void)
{
    return uph_audio_engine_data.context_ready ? &uph_audio_engine_data.context : NULL;
}

bool uph_audio_engine_recreate(void)
{
    uph_audio_engine_uninit_device();

    const Uph_AudioDriver driver = uph_state.settings.audio.driver;
    if (driver != uph_audio_engine_data.context_driver)
    {
        uph_audio_engine_uninit_context();
        uph_audio_engine_init_context(driver);
    }

    return uph_audio_engine_data.context_driver == driver && uph_audio_engine_init_device();
}

Uph_AudioDriver uph_audio_engine_active_driver(void)
{
    return uph_audio_engine_data.context_driver;
}

uint32_t uph_audio_engine_input_channel_count(void)
{
    if (uph_audio_engine_data.duplex)
        return uph_asio_input_count();

    if (uph_audio_engine_data.capture_ready)
        return uph_audio_engine_data.capture_device.capture.internalChannels;

    return 0;
}

void uph_audio_engine_update(void)
{
    if (!uph_asio_consume_reset_request() || uph_audio_engine_data.context_driver != UPH_AUDIO_DRIVER_ASIO)
        return;

    uph_audio_engine_stop();
    uph_audio_engine_recreate();
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

static double uph_tracks_end_beat(Naui_List(Uph_Track) tracks)
{
    double max_end_beat = 0.0;
    for (uint64_t t = 0; t < naui_list_len(tracks); t++)
    {
        const Uph_Track *track = &tracks[t];
        for (uint64_t b = 0; b < naui_list_len(track->blocks); b++)
        {
            const double block_end = track->blocks[b].start_beat + track->blocks[b].length_beats;
            if (block_end > max_end_beat)
                max_end_beat = block_end;
        }

        const double children_end = uph_tracks_end_beat(track->subtracks);
        if (children_end > max_end_beat)
            max_end_beat = children_end;
    }

    return max_end_beat;
}

double uph_audio_engine_get_song_length_beats(void)
{
    return uph_tracks_end_beat(uph_state.project.tracks);
}

double uph_audio_engine_get_song_length_seconds(void)
{
    Uph_Project *project = &uph_state.project;
    if (project->bpm <= 0.0f)
        return 0.0;

    return uph_tempo_beat_to_seconds(uph_audio_engine_get_song_length_beats());
}

double uph_audio_engine_get_song_length(void)
{
    const double bar = uph_time_signature_bar_length();
    double length_beats = uph_audio_engine_get_song_length_beats();
    return length_beats > 0.0 ? ceil(length_beats / bar - 1e-9) * bar : bar;
}

uint32_t uph_audio_engine_sample_rate(void)
{
    return uph_audio_engine_data.device.sampleRate;
}

static ma_format uph_render_bit_depth_format(Uph_RenderBitDepth depth)
{
    switch (depth)
    {
        case UPH_RENDER_BIT_DEPTH_16: return ma_format_s16;
        case UPH_RENDER_BIT_DEPTH_24: return ma_format_s24;
        default: return ma_format_f32;
    }
}

#define UPH_RENDER_JOB_RENDER_WEIGHT 0.9
#define UPH_RENDER_JOB_OUTPUT_CHUNK 4096

typedef uint8_t Uph_RenderJobPhase;
enum
{
    UPH_RENDER_JOB_IDLE,
    UPH_RENDER_JOB_RENDERING,
    UPH_RENDER_JOB_WRITING
};

typedef struct
{
    Uph_RenderSettings settings;
    char filepath[1024];
    Uph_RenderJobPhase phase;

    uint32_t engine_rate;
    uint32_t output_rate;
    uint32_t output_channels;
    ma_format output_format;
    double start_seconds;

    float *rendered;
    uint64_t total_frames;
    uint64_t buffer_frames;
    uint64_t rendered_frames;

    ma_data_converter converter;
    ma_encoder encoder;
    void *out_buffer;
    uint64_t target_frames;
    uint64_t consumed_frames;
    uint64_t written_frames;
    bool converter_ready;
    bool encoder_ready;
    bool locked;
}
Uph_RenderJob;
static Uph_RenderJob uph_render_job;

static void uph_render_job_release(bool delete_file)
{
    Uph_RenderJob *job = &uph_render_job;

    if (job->locked)
    {
        uph_audio_engine_stop_all_notes();
        uph_audio_engine_unlock();
    }

    if (job->encoder_ready)
        ma_encoder_uninit(&job->encoder);

    if (job->converter_ready)
        ma_data_converter_uninit(&job->converter, NULL);

    free(job->out_buffer);
    free(job->rendered);

    if (delete_file && job->encoder_ready)
        remove(job->filepath);

    *job = (Uph_RenderJob){0};
}

bool uph_audio_engine_render_begin(const char *filepath, const Uph_RenderSettings *settings)
{
    Uph_RenderJob *job = &uph_render_job;
    if (job->phase != UPH_RENDER_JOB_IDLE)
        return false;

    if (settings->end_beat <= settings->start_beat)
    {
        fprintf(stderr, "uph_audio_engine_render_begin: invalid beat range\n");
        return false;
    }

    if (uph_state.project.bpm <= 0.0f)
    {
        fprintf(stderr, "uph_audio_engine_render_begin: invalid bpm\n");
        return false;
    }

    *job = (Uph_RenderJob){0};
    job->settings = *settings;
    snprintf(job->filepath, sizeof(job->filepath), "%s", filepath);

    job->engine_rate = uph_audio_engine_data.device.sampleRate;
    job->output_rate = settings->sample_rate ? settings->sample_rate : job->engine_rate;
    job->output_channels = settings->channels == 1 ? 1 : 2;
    job->output_format = uph_render_bit_depth_format(settings->bit_depth);

    job->start_seconds = uph_tempo_beat_to_seconds(settings->start_beat);
    const double duration_seconds = uph_tempo_beat_to_seconds(settings->end_beat) - job->start_seconds + NAUI_MAX(0.0f, settings->tail_seconds);
    job->total_frames = (uint64_t)(duration_seconds * (double)job->engine_rate);
    if (job->total_frames == 0)
        return false;

    ma_data_converter_config converter_config = ma_data_converter_config_init(ma_format_f32, job->output_format, 2, job->output_channels, job->engine_rate, job->output_rate);
    converter_config.ditherMode = settings->dither && job->output_format != ma_format_f32 ? ma_dither_mode_triangle : ma_dither_mode_none;
    converter_config.resampling.algorithm = ma_resample_algorithm_linear;
    converter_config.resampling.linear.lpfOrder = MA_MAX_FILTER_ORDER;

    if (ma_data_converter_init(&converter_config, NULL, &job->converter) != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_render_begin: failed to init converter\n");
        uph_render_job_release(false);
        return false;
    }
    job->converter_ready = true;

    job->buffer_frames = job->total_frames + ma_data_converter_get_input_latency(&job->converter);
    job->rendered = (float*)calloc((size_t)job->buffer_frames * 2, sizeof(float));
    job->out_buffer = malloc((size_t)UPH_RENDER_JOB_OUTPUT_CHUNK * job->output_channels * ma_get_bytes_per_sample(job->output_format));
    if (!job->rendered || !job->out_buffer)
    {
        fprintf(stderr, "uph_audio_engine_render_begin: out of memory\n");
        uph_render_job_release(false);
        return false;
    }

    job->target_frames = (uint64_t)((double)job->total_frames * (double)job->output_rate / (double)job->engine_rate);

    uph_audio_engine_lock();
    job->locked = true;
    uph_audio_engine_stop_all_notes();

    job->phase = UPH_RENDER_JOB_RENDERING;
    return true;
}

static void uph_render_job_normalize(Uph_RenderJob *job)
{
    float peak = 0.0f;
    for (uint64_t i = 0; i < job->total_frames * 2; i++)
        peak = NAUI_MAX(peak, fabsf(job->rendered[i]));

    if (peak <= 1e-6f)
        return;

    const float gain = 0.98855309f / peak;
    for (uint64_t i = 0; i < job->total_frames * 2; i++)
        job->rendered[i] *= gain;
}

static bool uph_render_job_start_writing(Uph_RenderJob *job)
{
    uph_audio_engine_stop_all_notes();
    uph_audio_engine_unlock();
    job->locked = false;

    if (job->settings.normalize)
        uph_render_job_normalize(job);

    ma_encoder_config encoder_config = ma_encoder_config_init(ma_encoding_format_wav, job->output_format, job->output_channels, job->output_rate);
    if (ma_encoder_init_file(job->filepath, &encoder_config, &job->encoder) != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_render: failed to init WAV encoder for '%s'\n", job->filepath);
        return false;
    }

    job->encoder_ready = true;
    job->phase = UPH_RENDER_JOB_WRITING;
    return true;
}

static bool uph_render_job_write_chunk(Uph_RenderJob *job)
{
    ma_uint64 frames_in = job->buffer_frames - job->consumed_frames;
    ma_uint64 frames_out = NAUI_MIN((uint64_t)UPH_RENDER_JOB_OUTPUT_CHUNK, job->target_frames - job->written_frames);
    if (ma_data_converter_process_pcm_frames(&job->converter, job->rendered + job->consumed_frames * 2, &frames_in, job->out_buffer, &frames_out) != MA_SUCCESS)
        return false;

    job->consumed_frames += frames_in;
    if (frames_out == 0)
    {
        job->written_frames = job->target_frames;
        return true;
    }

    ma_uint64 frames_written = 0;
    if (ma_encoder_write_pcm_frames(&job->encoder, job->out_buffer, frames_out, &frames_written) != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_render: failed writing frames to '%s'\n", job->filepath);
        return false;
    }

    job->written_frames += frames_written;
    return true;
}

Uph_RenderStatus uph_audio_engine_render_step(double budget_seconds)
{
    Uph_RenderJob *job = &uph_render_job;
    if (job->phase == UPH_RENDER_JOB_IDLE)
        return UPH_RENDER_STATUS_IDLE;

    const ma_uint32 chunk_frames = NAUI_MAX(64u, uph_state.settings.audio.buffer_size);
    const double deadline = naui_time() + budget_seconds;

    do
    {
        if (job->phase == UPH_RENDER_JOB_RENDERING)
        {
            const ma_uint32 frames = (ma_uint32)NAUI_MIN((uint64_t)chunk_frames, job->total_frames - job->rendered_frames);
            const double playhead_beat = uph_tempo_seconds_to_beat(job->start_seconds + (double)job->rendered_frames / (double)job->engine_rate);
            uph_render_audio(playhead_beat, job->engine_rate, job->rendered + job->rendered_frames * 2, frames, true, NULL);
            job->rendered_frames += frames;

            if (job->rendered_frames >= job->total_frames && !uph_render_job_start_writing(job))
            {
                uph_render_job_release(true);
                return UPH_RENDER_STATUS_FAILED;
            }
        }
        else
        {
            if (!uph_render_job_write_chunk(job))
            {
                uph_render_job_release(true);
                return UPH_RENDER_STATUS_FAILED;
            }

            if (job->written_frames >= job->target_frames)
            {
                fprintf(stdout, "uph_audio_engine_render: rendered %llu frames to '%s'\n", (unsigned long long)job->written_frames, job->filepath);
                uph_render_job_release(false);
                return UPH_RENDER_STATUS_DONE;
            }
        }
    }
    while (budget_seconds <= 0.0 || naui_time() < deadline);

    return UPH_RENDER_STATUS_RUNNING;
}

float uph_audio_engine_render_progress(void)
{
    const Uph_RenderJob *job = &uph_render_job;
    if (job->phase == UPH_RENDER_JOB_IDLE || job->total_frames == 0)
        return 0.0f;

    const double rendered = (double)job->rendered_frames / (double)job->total_frames;
    const double written = job->target_frames ? (double)job->written_frames / (double)job->target_frames : 0.0;
    return (float)(rendered * UPH_RENDER_JOB_RENDER_WEIGHT + written * (1.0 - UPH_RENDER_JOB_RENDER_WEIGHT));
}

bool uph_audio_engine_render_active(void)
{
    return uph_render_job.phase != UPH_RENDER_JOB_IDLE;
}

void uph_audio_engine_render_cancel(void)
{
    if (uph_render_job.phase != UPH_RENDER_JOB_IDLE)
        uph_render_job_release(true);
}

bool uph_audio_engine_render_to_wav(const char *filepath, const Uph_RenderSettings *settings)
{
    if (!uph_audio_engine_render_begin(filepath, settings))
        return false;

    Uph_RenderStatus status;
    while ((status = uph_audio_engine_render_step(0.0)) == UPH_RENDER_STATUS_RUNNING) {}
    return status == UPH_RENDER_STATUS_DONE;
}

bool uph_audio_engine_export_to_wav(const char *filepath, double start_beat, double end_beat)
{
    const Uph_RenderSettings settings = {
        .start_beat = start_beat,
        .end_beat = end_beat,
        .channels = 2,
        .bit_depth = UPH_RENDER_BIT_DEPTH_32_FLOAT
    };
    return uph_audio_engine_render_to_wav(filepath, &settings);
}

void uph_audio_engine_unload_sample_data(Uph_SampleData *sample_data)
{
    if (!sample_data)
        return;
    free(sample_data->frames);
    naui_list_free(sample_data->waveform_peaks);
    naui_list_free(sample_data->channel_peaks);
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