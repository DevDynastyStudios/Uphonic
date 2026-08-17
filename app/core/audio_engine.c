typedef struct
{
    ma_device device;
}
Uph_AudioEngineData;

static Uph_AudioEngineData data;

static double uph_beats_to_seconds(double beats, float bpm)
{
    return (beats / (double)bpm) * 60.0;
}

static double uph_seconds_to_beats(double seconds, float bpm)
{
    return (seconds / 60.0) * (double)bpm;
}

static void uph_read_sample_frame(const Uph_Sample *sample, double pos, float *out_left, float *out_right)
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

static void uph_data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
    (void)input;

    float *out = (float*)output;
    memset(out, 0, sizeof(float) * 2 * frame_count);

    if (!uph_state.interact.song_timeline_playing)
        return;

    Uph_Project *project = &uph_state.project;
    float bpm = project->bpm;
    if (bpm <= 0.0f)
        return;

    uint32_t engine_sample_rate = device->sampleRate;
    double playhead_start_beat = uph_state.interact.song_timeline_playhead_position;

    uint64_t track_count = naui_list_len(project->tracks);
    for (uint64_t t = 0; t < track_count; t++)
    {
        Uph_Track *track = &project->tracks[t];
        if (track->state & UPH_TRACK_STATE_MUTE)
        {
            track->peak_right = 0.0f;
            track->peak_left = 0.0f;
            continue;
        }

        uint64_t block_count = naui_list_len(track->blocks);

        float volume = naui_clamp01(track->volume);
        float pan = naui_clamp(track->pan, -1.0f, 1.0f);

        float pan_angle = (pan + 1.0f) * 0.25f * (float)NAUI_PI;
        float gain_left  = volume * cosf(pan_angle) * NAUI_SQRT2;
        float gain_right = volume * sinf(pan_angle) * NAUI_SQRT2;

        float track_peak_left = 0.0f;
        float track_peak_right = 0.0f;

        for (uint64_t b = 0; b < block_count; b++)
        {
            Uph_TimelineBlock *block = &track->blocks[b];

            if (block->type != UPH_TIMELINE_BLOCK_SAMPLE)
                continue;

            if (block->resource_index >= naui_list_len(project->samples))
                continue;

            Uph_Sample *sample = &project->samples[block->resource_index];
            double stretch_scale = (block->stretch_scale > 0.0) ? block->stretch_scale : 1.0;

            for (uint32_t f = 0; f < frame_count; f++)
            {
                double frame_beat = playhead_start_beat + uph_seconds_to_beats((double)f / (double)engine_sample_rate, bpm);

                double beats_into_block = frame_beat - block->start_beat;
                if (beats_into_block < 0.0 || beats_into_block >= block->length_beats)
                    continue;

                double source_beats = beats_into_block / stretch_scale + block->start_offset_beats;
                double source_seconds = uph_beats_to_seconds(source_beats, bpm);
                double source_frame_pos = source_seconds * (double)uph_state.settings.audio.sample_rate;

                float left, right;
                uph_read_sample_frame(sample, source_frame_pos, &left, &right);

                float out_left  = left  * gain_left;
                float out_right = right * gain_right;

                out[f * 2 + 0] += out_left;
                out[f * 2 + 1] += out_right;

                float abs_left  = fabsf(out_left);
                float abs_right = fabsf(out_right);
                if (abs_left  > track_peak_left)  track_peak_left  = abs_left;
                if (abs_right > track_peak_right) track_peak_right = abs_right;
            }
        }

        track->peak_left  = track_peak_left;
        track->peak_right = track_peak_right;
    }

    double buffer_beats = uph_seconds_to_beats((double)frame_count / (double)engine_sample_rate, bpm);
    uph_state.interact.song_timeline_playhead_position = playhead_start_beat + buffer_beats;
}

void uph_audio_engine_init(void)
{
    Uph_AudioSettings settings = uph_state.settings.audio;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = settings.sample_rate;
    config.dataCallback      = uph_data_callback;

    ma_result result = ma_device_init(NULL, &config, &data.device);
    if (result != MA_SUCCESS)
    {
        // TODO: route through your logging system instead
        fprintf(stderr, "uph_audio_engine_init: failed to init playback device (%s)\n", ma_result_description(result));
        return;
    }

    ma_device_start(&data.device);
}

void uph_audio_engine_shutdown(void)
{
    ma_device_uninit(&data.device);
}

static void uph_build_waveform_peaks(Uph_Sample *resource)
{
    if (!resource->frames || resource->frame_count == 0)
        return;

    const float *samples = (const float*)resource->frames;
    const int channel_count = (resource->channel_type == UPH_SAMPLE_STEREO) ? 2 : 1;

    const uint64_t bin_count = (resource->frame_count + UPH_SAMPLE_FRAME_COUNT - 1) / UPH_SAMPLE_FRAME_COUNT;

    naui_list_reserve(resource->waveform_peaks, bin_count);

    for (uint64_t bin = 0; bin < bin_count; bin++)
    {
        uint64_t frame_start = bin * UPH_SAMPLE_FRAME_COUNT;
        uint64_t frame_end = frame_start + UPH_SAMPLE_FRAME_COUNT;
        if (frame_end > resource->frame_count)
            frame_end = resource->frame_count;

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

        Uph_WaveformPeak peak = { .min = min_v, .max = max_v };
        naui_list_push(resource->waveform_peaks, peak);
    }
}

Uph_Sample uph_audio_engine_load_sample(Naui_Path path)
{
    Uph_Sample sample = {0};

    ma_decoder temp_decoder;
    ma_decoder_config temp_config = ma_decoder_config_init(ma_format_f32, 0, 0);

    size_t file_size;
    void *file_data = naui_file_read_all(path, &file_size);
    if (!file_data)
    {
        fprintf(stderr, "uph_audio_engine_load_sample: failed to read file '%s'\n", path.data);
        return sample;
    }

    uint32_t original_sample_rate = data.device.sampleRate;
    if (ma_decoder_init_memory(file_data, file_size, &temp_config, &temp_decoder) == MA_SUCCESS)
    {
        original_sample_rate = temp_decoder.outputSampleRate;
        ma_decoder_uninit(&temp_decoder);
    }

    ma_decoder decoder;
    ma_decoder_config decoder_config = ma_decoder_config_init(
        ma_format_f32,
        0,
        data.device.sampleRate
    );

    ma_result result = ma_decoder_init_memory(file_data, file_size, &decoder_config, &decoder);
    if (result != MA_SUCCESS)
    {
        // TODO: route through your logging/error system instead
        fprintf(stderr, "uph_audio_engine_load_sample: failed to open '%s' (%s)\n", path.data, ma_result_description(result));
        return sample;
    }

    ma_uint64 frame_count = 0;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);
    if (result != MA_SUCCESS || frame_count == 0)
    {
        fprintf(stderr, "uph_audio_engine_load_sample: failed to get length of '%s'\n", path.data);
        ma_decoder_uninit(&decoder);
        return sample;
    }

    ma_uint32 channels = decoder.outputChannels;
    ma_uint32 sample_rate = decoder.outputSampleRate;

    size_t frame_size = sizeof(float) * channels;
    void *frames = malloc((size_t)frame_count * frame_size);
    if (!frames)
    {
        fprintf(stderr, "uph_audio_engine_load_sample: out of memory loading '%s'\n", path.data);
        ma_decoder_uninit(&decoder);
        return sample;
    }

    ma_uint64 frames_read = 0;
    result = ma_decoder_read_pcm_frames(&decoder, frames, frame_count, &frames_read);
    ma_decoder_uninit(&decoder);

    if (result != MA_SUCCESS || frames_read != frame_count)
    {
        fprintf(stderr, "uph_audio_engine_load_sample: failed to fully decode '%s'\n", path.data);
        free(frames);
        return sample;
    }

    naui_string_copy_view(&sample.name, naui_file_stem(&path));
    sample.file_path = path;
    sample.frames = frames;
    sample.frame_count = frames_read;
    sample.original_sample_rate = original_sample_rate;
    sample.channel_type = (channels == 1) ? UPH_SAMPLE_MONO : UPH_SAMPLE_STEREO;

    if (original_sample_rate != sample_rate)
        fprintf(stdout, "uph_audio_engine_load_sample: loaded '%s' (%llu frames, %u channels, %u Hz -> %u Hz [resampled])\n", path.data, (unsigned long long)frames_read, channels, original_sample_rate, sample_rate);
    else
        fprintf(stdout, "uph_audio_engine_load_sample: loaded '%s' (%llu frames, %u channels, %u Hz)\n", path.data, (unsigned long long)frames_read, channels, sample_rate);

    uph_build_waveform_peaks(&sample);

    return sample;
}

double uph_audio_engine_get_song_length_beats(void)
{
    Uph_Project *project = &uph_state.project;

    double max_end_beat = 0.0;

    uint64_t track_count = naui_list_len(project->tracks);
    for (uint64_t t = 0; t < track_count; t++)
    {
        Uph_Track *track = &project->tracks[t];
        uint64_t block_count = naui_list_len(track->blocks);

        for (uint64_t b = 0; b < block_count; b++)
        {
            Uph_TimelineBlock *block = &track->blocks[b];

            double block_end = block->start_beat + block->length_beats;
            if (block_end > max_end_beat)
                max_end_beat = block_end;
        }
    }

    return max_end_beat;
}

double uph_audio_engine_get_song_length_seconds(void)
{
    Uph_Project *project = &uph_state.project;
    float bpm = project->bpm;
    if (bpm <= 0.0f)
        return 0.0;

    return uph_beats_to_seconds(uph_audio_engine_get_song_length_beats(), bpm);
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

    uint32_t sample_rate = data.device.sampleRate;
    uint64_t total_frames = (uint64_t)(duration_seconds * (double)sample_rate);

    ma_encoder_config encoder_config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, sample_rate);
    ma_encoder encoder;
    if (ma_encoder_init_file(filepath, &encoder_config, &encoder) != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_export_to_wav: failed to init WAV encoder for '%s'\n", filepath);
        return false;
    }

    double saved_playhead = uph_state.interact.song_timeline_playhead_position;
    bool saved_playing  = uph_state.interact.song_timeline_playing;

    uph_state.interact.song_timeline_playhead_position = start_beat;
    uph_state.interact.song_timeline_playing = true;

    const ma_uint32 chunk_frames = 1024;
    float *buffer = (float*)malloc(sizeof(float) * 2 * chunk_frames);
    if (!buffer)
    {
        fprintf(stderr, "uph_audio_engine_export_to_wav: out of memory\n");
        ma_encoder_uninit(&encoder);
        uph_state.interact.song_timeline_playhead_position = saved_playhead;
        uph_state.interact.song_timeline_playing = saved_playing;
        return false;
    }

    ma_device fake_device = data.device;
    fake_device.sampleRate = sample_rate;

    uint64_t frames_processed = 0;
    bool ok = true;

    while (frames_processed < total_frames)
    {
        ma_uint32 frames_to_process = (ma_uint32)((total_frames - frames_processed) < chunk_frames
            ? (total_frames - frames_processed)
            : chunk_frames);

        uph_data_callback(&fake_device, buffer, NULL, frames_to_process);

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

    uph_state.interact.song_timeline_playhead_position = saved_playhead;
    uph_state.interact.song_timeline_playing = saved_playing;

    if (ok)
        fprintf(stdout, "uph_audio_engine_export_to_wav: exported %llu frames to '%s'\n",
                (unsigned long long)frames_processed, filepath);

    return ok;
}

void uph_audio_engine_unload_sample(Uph_Sample*sample)
{
    if (!sample)
        return;
    free(sample->frames);
    naui_list_free(sample->waveform_peaks);
    sample->frames = NULL;
    sample->frame_count = 0;
}