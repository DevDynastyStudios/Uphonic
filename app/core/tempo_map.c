#define UPH_TEMPO_MAP_STEPS_PER_BEAT 32
#define UPH_TEMPO_MAP_MAX_STEPS (1u << 21)

typedef struct
{
    double *seconds;
    float *bpm;
    uint32_t count;
    double step;
    float tail_bpm;
    float base_bpm;
}
Uph_TempoMap;

typedef struct
{
    _Atomic(Uph_TempoMap*) current;
    Uph_TempoMap *retired;
    uint64_t retired_after;
    atomic_uint_fast64_t callbacks_started;
    atomic_uint_fast64_t callbacks_finished;
    _Atomic(float) current_bpm;
    uint64_t hash;
    bool hash_valid;
}
Uph_TempoMapData;

static Uph_TempoMapData uph_tempo_map_data = { 0 };

float uph_tempo_from_automation(float value)
{
    const float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    return UPH_TEMPO_MIN_BPM + clamped * (UPH_TEMPO_MAX_BPM - UPH_TEMPO_MIN_BPM);
}

Uph_Track *uph_tempo_find_lane(void)
{
    Uph_Track *master = uph_resources_master_track();
    if (!master)
        return NULL;

    const uint32_t count = (uint32_t)naui_list_len(master->subtracks);
    for (uint32_t i = 0; i < count; i++)
    {
        Uph_Track *track = &master->subtracks[i];
        if (track->type == UPH_RESOURCE_AUTOMATION && track->automation_target == UPH_AUTOMATION_TARGET_TEMPO)
            return track;
    }
    return NULL;
}

bool uph_tempo_lane_active(void)
{
    return atomic_load(&uph_tempo_map_data.current) != NULL;
}

static float uph_tempo_base_bpm(void)
{
    const float bpm = uph_state.project.bpm;
    return bpm > 0.0f ? bpm : 120.0f;
}

static bool uph_tempo_block_value(const Uph_TimelineBlock *block, double beat, float *out_bpm)
{
    if (block->resource_index >= naui_list_len(uph_state.project.automations))
        return false;

    float value;
    if (!uph_automation_value_at_beat(&uph_state.project.automations[block->resource_index], (beat - block->start_beat) + block->start_offset_beats, &value))
        return false;

    *out_bpm = uph_tempo_from_automation(value);
    return true;
}

static float uph_tempo_lane_bpm_at(const Uph_Track *lane, double beat, float base)
{
    double held_end = -INFINITY;
    float held = base;

    const uint64_t block_count = naui_list_len(lane->blocks);
    for (uint64_t b = 0; b < block_count; b++)
    {
        const Uph_TimelineBlock *block = &lane->blocks[b];
        const double end = block->start_beat + block->length_beats;
        float bpm;

        if (beat >= block->start_beat && beat < end)
        {
            if (uph_tempo_block_value(block, beat, &bpm))
                return bpm;
            continue;
        }

        if (end <= beat && end > held_end && uph_tempo_block_value(block, end - 1e-9, &bpm))
        {
            held_end = end;
            held = bpm;
        }
    }

    return held;
}

static uint64_t uph_tempo_hash_bytes(uint64_t hash, const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t*)data;
    for (size_t i = 0; i < size; i++)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint64_t uph_tempo_lane_hash(const Uph_Track *lane, float base)
{
    uint64_t hash = 14695981039346656037ull;
    hash = uph_tempo_hash_bytes(hash, &base, sizeof(base));

    const uint64_t block_count = naui_list_len(lane->blocks);
    for (uint64_t b = 0; b < block_count; b++)
    {
        const Uph_TimelineBlock *block = &lane->blocks[b];
        hash = uph_tempo_hash_bytes(hash, &block->start_beat, sizeof(block->start_beat));
        hash = uph_tempo_hash_bytes(hash, &block->start_offset_beats, sizeof(block->start_offset_beats));
        hash = uph_tempo_hash_bytes(hash, &block->length_beats, sizeof(block->length_beats));
        hash = uph_tempo_hash_bytes(hash, &block->resource_index, sizeof(block->resource_index));

        if (block->resource_index >= naui_list_len(uph_state.project.automations))
            continue;

        const Uph_Automation *automation = &uph_state.project.automations[block->resource_index];
        const uint64_t point_count = naui_list_len(automation->points);
        for (uint64_t p = 0; p < point_count; p++)
        {
            const Uph_AutomationPoint *point = &automation->points[p];
            hash = uph_tempo_hash_bytes(hash, &point->beat, sizeof(point->beat));
            hash = uph_tempo_hash_bytes(hash, &point->value, sizeof(point->value));
            hash = uph_tempo_hash_bytes(hash, &point->tension, sizeof(point->tension));
            hash = uph_tempo_hash_bytes(hash, &point->curve, sizeof(point->curve));
        }
    }
    return hash;
}

static void uph_tempo_map_free(Uph_TempoMap *map)
{
    if (!map)
        return;
    free(map->seconds);
    free(map->bpm);
    free(map);
}

static Uph_TempoMap *uph_tempo_map_build(const Uph_Track *lane, float base)
{
    double end_beat = 0.0;
    const uint64_t block_count = naui_list_len(lane->blocks);
    for (uint64_t b = 0; b < block_count; b++)
    {
        const double end = lane->blocks[b].start_beat + lane->blocks[b].length_beats;
        if (end > end_beat)
            end_beat = end;
    }

    if (end_beat <= 0.0)
        return NULL;

    const double step = 1.0 / (double)UPH_TEMPO_MAP_STEPS_PER_BEAT;
    double wanted = ceil(end_beat / step);
    uint32_t count = wanted > (double)UPH_TEMPO_MAP_MAX_STEPS ? UPH_TEMPO_MAP_MAX_STEPS : (uint32_t)wanted;
    if (count == 0)
        count = 1;

    Uph_TempoMap *map = (Uph_TempoMap*)calloc(1, sizeof(Uph_TempoMap));
    if (!map)
        return NULL;

    map->seconds = (double*)malloc(sizeof(double) * ((size_t)count + 1));
    map->bpm = (float*)malloc(sizeof(float) * (size_t)count);
    if (!map->seconds || !map->bpm)
    {
        uph_tempo_map_free(map);
        return NULL;
    }

    map->count = count;
    map->step = step;
    map->base_bpm = base;
    map->seconds[0] = 0.0;
    for (uint32_t i = 0; i < count; i++)
    {
        const float bpm = uph_tempo_lane_bpm_at(lane, ((double)i + 0.5) * step, base);
        map->bpm[i] = bpm;
        map->seconds[i + 1] = map->seconds[i] + step * 60.0 / (double)bpm;
    }
    map->tail_bpm = uph_tempo_lane_bpm_at(lane, (double)count * step, base);

    return map;
}

static bool uph_tempo_map_try_release_retired(void)
{
    Uph_TempoMapData *data = &uph_tempo_map_data;
    if (!data->retired)
        return true;

    const uint64_t started = atomic_load(&data->callbacks_started);
    const uint64_t finished = atomic_load(&data->callbacks_finished);
    if (started != finished && finished < data->retired_after)
        return false;

    uph_tempo_map_free(data->retired);
    data->retired = NULL;
    return true;
}

static void uph_tempo_map_publish(Uph_TempoMap *map)
{
    Uph_TempoMapData *data = &uph_tempo_map_data;
    Uph_TempoMap *old = atomic_exchange(&data->current, map);
    data->retired = old;
    data->retired_after = atomic_load(&data->callbacks_started);
    uph_tempo_map_try_release_retired();
}

void uph_tempo_map_update(void)
{
    Uph_TempoMapData *data = &uph_tempo_map_data;
    if (!uph_tempo_map_try_release_retired())
        return;

    const Uph_Track *lane = uph_tempo_find_lane();
    const bool usable = lane && !(lane->state & UPH_TRACK_MUTED) && naui_list_len(lane->blocks) > 0;
    const float base = uph_tempo_base_bpm();

    if (!usable)
    {
        if (atomic_load(&data->current))
            uph_tempo_map_publish(NULL);
        data->hash_valid = false;
        return;
    }

    const uint64_t hash = uph_tempo_lane_hash(lane, base);
    if (data->hash_valid && data->hash == hash && atomic_load(&data->current))
        return;

    Uph_TempoMap *map = uph_tempo_map_build(lane, base);
    data->hash = hash;
    data->hash_valid = true;
    uph_tempo_map_publish(map);
}

void uph_tempo_map_shutdown(void)
{
    Uph_TempoMapData *data = &uph_tempo_map_data;
    uph_tempo_map_free(atomic_exchange(&data->current, NULL));
    uph_tempo_map_free(data->retired);
    data->retired = NULL;
    data->hash_valid = false;
}

void uph_tempo_map_audio_begin(void)
{
    atomic_fetch_add(&uph_tempo_map_data.callbacks_started, 1);
}

void uph_tempo_map_audio_end(void)
{
    atomic_fetch_add(&uph_tempo_map_data.callbacks_finished, 1);
}

float uph_tempo_bpm_at(double beat)
{
    const Uph_TempoMap *map = atomic_load(&uph_tempo_map_data.current);
    if (!map)
        return uph_tempo_base_bpm();

    if (beat < 0.0)
        return map->bpm[0];

    const double index = beat / map->step;
    if (index >= (double)map->count)
        return map->tail_bpm;

    return map->bpm[(uint32_t)index];
}

double uph_tempo_beat_to_seconds(double beat)
{
    const Uph_TempoMap *map = atomic_load(&uph_tempo_map_data.current);
    if (!map)
        return beat * 60.0 / (double)uph_tempo_base_bpm();

    if (beat < 0.0)
        return beat * 60.0 / (double)map->bpm[0];

    const double index = beat / map->step;
    if (index >= (double)map->count)
        return map->seconds[map->count] + (beat - (double)map->count * map->step) * 60.0 / (double)map->tail_bpm;

    const uint32_t i = (uint32_t)index;
    return map->seconds[i] + (beat - (double)i * map->step) * 60.0 / (double)map->bpm[i];
}

double uph_tempo_seconds_to_beat(double seconds)
{
    const Uph_TempoMap *map = atomic_load(&uph_tempo_map_data.current);
    if (!map)
        return seconds * (double)uph_tempo_base_bpm() / 60.0;

    if (seconds < 0.0)
        return seconds * (double)map->bpm[0] / 60.0;

    if (seconds >= map->seconds[map->count])
        return (double)map->count * map->step + (seconds - map->seconds[map->count]) * (double)map->tail_bpm / 60.0;

    uint32_t low = 0;
    uint32_t high = map->count;
    while (high - low > 1)
    {
        const uint32_t mid = low + (high - low) / 2;
        if (map->seconds[mid] <= seconds)
            low = mid;
        else
            high = mid;
    }

    return (double)low * map->step + (seconds - map->seconds[low]) * (double)map->bpm[low] / 60.0;
}

double uph_tempo_beat_after_seconds(double beat, double seconds)
{
    return uph_tempo_seconds_to_beat(uph_tempo_beat_to_seconds(beat) + seconds);
}

void uph_tempo_set_current_bpm(float bpm)
{
    atomic_store(&uph_tempo_map_data.current_bpm, bpm);
}

float uph_tempo_current_bpm(void)
{
    const float bpm = atomic_load(&uph_tempo_map_data.current_bpm);
    return bpm > 0.0f ? bpm : uph_tempo_base_bpm();
}
