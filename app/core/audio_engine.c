#ifndef UPH_INVALID_TIMELINE_BLOCK
#define UPH_INVALID_TIMELINE_BLOCK NULL
#endif

typedef struct
{
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

    uph_process_plugin(
        &track->instrument,
        (float**)inputs,
        (float**)outputs,
        frame_count,
        playhead_beat,
        is_playing
    );
}

static void uph_apply_track_effects(
    Uph_Track *track,
    float *left,
    float *right,
    ma_uint32 frame_count,
    double playhead_beat,
    bool is_playing
)
{
    uint64_t effect_count = naui_list_len(track->effects);
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
        Uph_EffectPlugin *effect = &track->effects[e];
        if (!effect->enabled || !effect->plugin.loaded)
            continue;

        memset(dst[0], 0, bytes);
        memset(dst[1], 0, bytes);

        uph_process_plugin(
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

static bool uph_automation_value_at_beat(Uph_Automation *automation, double beat, float *out_value)
{
    uint64_t count = naui_list_len(automation->points);
    if (count == 0)
        return false;

    if (beat <= automation->points[0].beat)
    {
        *out_value = automation->points[0].value;
        return true;
    }
    if (beat >= automation->points[count - 1].beat)
    {
        *out_value = automation->points[count - 1].value;
        return true;
    }

    for (uint64_t i = 0; i + 1 < count; i++)
    {
        Uph_AutomationPoint *a = &automation->points[i];
        Uph_AutomationPoint *b = &automation->points[i + 1];

        if (beat >= a->beat && beat <= b->beat)
        {
            double span = b->beat - a->beat;
            double t = (span > 0.0) ? (beat - a->beat) / span : 0.0;
            *out_value = (float)(a->value + (b->value - a->value) * t);
            return true;
        }
    }

    return false;
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
    float *track_l = alloca(max_frame_count * sizeof(float));
    float *track_r = alloca(max_frame_count * sizeof(float));

    uint64_t track_count = naui_list_len(project->tracks);
    for (uint64_t t = 0; t < track_count; t++)
    {
        Uph_Track *track = &project->tracks[t];

        track->peak_right = 0.0f;
        track->peak_left = 0.0f;

        if (!uph_track_is_audible(project, track))
            continue;

        uint64_t block_count = naui_list_len(track->blocks);

        float volume = (float)fabs(track->volume);
        float pan = NAUI_CLAMP(track->pan, -1.0f, 1.0f);

        float pan_angle = (pan + 1.0f) * 0.25f * (float)NAUI_PI;
        float gain_left  = volume * cosf(pan_angle) * NAUI_SQRT2;
        float gain_right = volume * sinf(pan_angle) * NAUI_SQRT2;

        memset(track_l, 0, max_frame_count * sizeof(float));
        memset(track_r, 0, max_frame_count * sizeof(float));

        if (timeline_playing)
        {
            uph_queue_track_automation(
                track,
                buffer_start_beat, buffer_end_beat,
                engine_sample_rate, bpm
            );
        }

        if (track->type == UPH_RESOURCE_SAMPLE)
        {
            if (timeline_playing)
            {
                for (uint64_t b = 0; b < block_count; b++)
                {
                    Uph_TimelineBlock *block = &track->blocks[b];

                    if (block == track->armed_block)
                        continue;

                    if (block->resource_index >= naui_list_len(project->samples))
                        continue;

                    Uph_Sample *sample = &project->samples[block->resource_index];
                    Uph_SampleData *sample_data = &project->sample_data[sample->data_index];

                    double time_scale = (sample->time_scale > 0.0) ? sample->time_scale : 1.0;

                    for (uint32_t f = 0; f < frame_count; f++)
                    {
                        double frame_beat = playhead_start_beat + uph_seconds_to_beats((double)f / (double)engine_sample_rate, bpm);

                        double beats_into_block = frame_beat - block->start_beat;
                        if (beats_into_block < 0.0 || beats_into_block >= block->length_beats)
                            continue;

                        double source_beats = beats_into_block / time_scale + block->start_offset_beats;
                        double source_seconds = uph_beats_to_seconds(source_beats, bpm);
                        double source_frame_pos = source_seconds * (double)engine_sample_rate;

                        float left, right;
                        uph_read_sample_frame(sample_data, source_frame_pos, &left, &right);

                        track_l[f] += left;
                        track_r[f] += right;
                    }
                }
            }
        }
        else if (track->type == UPH_RESOURCE_PATTERN)
        {
            if (track->instrument.loaded)
            {
                if (timeline_playing)
                {
                    bool should_be_held[128] = { false };

                    for (uint64_t b = 0; b < block_count; b++)
                    {
                        Uph_TimelineBlock *block = &track->blocks[b];
                        if (block->type != UPH_RESOURCE_PATTERN)
                            continue;

                        uph_queue_pattern_block_notes(
                            track, block,
                            buffer_start_beat, buffer_end_beat,
                            engine_sample_rate, bpm
                        );

                        uph_mark_block_held_notes(
                            track, block,
                            buffer_end_beat,
                            should_be_held
                        );
                    }

                    for (int key = 0; key < 128; key++)
                    {
                        if (uph_plugin_note_active(&track->instrument, (uint8_t)key) && !should_be_held[key])
                            uph_plugin_queue_note_event(&track->instrument, false, (uint8_t)key, 0, 0, 0);
                    }
                }

                uph_render_track_instrument(
                    track, frame_count,
                    track_l, track_r,
                    playhead_start_beat, timeline_playing
                );
            }
        }

        if (input
            && track->monitored
            && (track->type == UPH_RESOURCE_SAMPLE || track->type == UPH_RESOURCE_NONE))
        {
            for (ma_uint32 f = 0; f < frame_count; f++)
            {
                track_l[f] += input[f * 2 + 0];
                track_r[f] += input[f * 2 + 1];
            }
        }

        uph_apply_track_effects(
            track,
            track_l, track_r,
            frame_count,
            playhead_start_beat, timeline_playing
        );

        float track_peak_left = 0.0f;
        float track_peak_right = 0.0f;

        for (ma_uint32 f = 0; f < frame_count; f++)
        {
            float out_left  = track_l[f] * gain_left;
            float out_right = track_r[f] * gain_right;

            out[f * 2 + 0] += out_left;
            out[f * 2 + 1] += out_right;

            float abs_left  = fabsf(out_left);
            float abs_right = fabsf(out_right);
            if (abs_left  > track_peak_left)  track_peak_left  = abs_left;
            if (abs_right > track_peak_right) track_peak_right = abs_right;
        }

        track->peak_left  = track_peak_left;
        track->peak_right = track_peak_right;
    }
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

    const uint32_t max_frame_count = uph_state.settings.audio.buffer_size;

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

    const uint32_t max_frame_count = uph_state.settings.audio.buffer_size;
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

static void uph_finish_takes_all(Naui_List(Uph_Track) tracks)
{
    for (uint64_t i = 0; i < naui_list_len(tracks); i++)
    {
        uph_finish_takes_all(tracks[i].subtracks);
        uph_finish_take(&tracks[i]);
    }
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

    if (input_f && (input_channel == 0 || input_channel == 1))
    {
        float *routed = alloca(frame_count * 2 * sizeof(float));
        for (ma_uint32 f = 0; f < frame_count; f++)
        {
            float s = input_f[f * 2 + input_channel];
            routed[f * 2 + 0] = s;
            routed[f * 2 + 1] = s;
        }
        input_f = routed;
    }

    uph_render_audio(playhead_start_beat, engine_sample_rate, out, frame_count, is_playing, input_f);

    if (is_playing && input_f && bpm > 0.0f)
        uph_record_input(uph_state.project.tracks, input_f, frame_count, engine_sample_rate, bpm);

    if (was_playing && !is_playing)
    {
        uph_audio_engine_stop_all_notes();
        uph_finish_takes_all(uph_state.project.tracks);
    }

    was_playing = is_playing;
    if (!is_playing)
        return;

    if (bpm > 0.0f)
    {
        double buffer_beats = uph_seconds_to_beats((double)frame_count / (double)engine_sample_rate, bpm);
        double new_beat = playhead_start_beat + buffer_beats;
        if (new_beat >= uph_audio_engine_get_song_length() && !uph_has_active_take(uph_state.project.tracks))
            new_beat = 0.0;
        uph_state.shared.song_timeline_playhead_position = new_beat;
    }
}

void uph_audio_engine_init(void)
{
    Uph_AudioSettings settings = uph_state.settings.audio;

    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.capture.format     = ma_format_f32;
    config.capture.channels   = settings.channels;
    //config.playback.pDeviceID = &settings.input_device.id;
    config.playback.format    = ma_format_f32;
    config.playback.channels  = settings.channels;
    //config.playback.pDeviceID = &settings.output_device.id;
    config.sampleRate         = settings.sample_rate;
    config.dataCallback       = uph_audio_engine_data_callback;

    config.performanceProfile  = ma_performance_profile_low_latency;
    config.playback.shareMode  = settings.exclusive_mode ? ma_share_mode_exclusive : ma_share_mode_shared;
    config.capture.shareMode   = settings.exclusive_mode ? ma_share_mode_exclusive : ma_share_mode_shared;
    config.periodSizeInFrames = settings.buffer_size;
    //config.periods            = 2;

    ma_result result = ma_device_init(NULL, &config, &uph_audio_engine_data.device);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "uph_audio_engine_init: failed to init duplex device (%s), falling back to playback only\n", ma_result_description(result));

        config.deviceType = ma_device_type_playback;
        result = ma_device_init(NULL, &config, &uph_audio_engine_data.device);
        if (result != MA_SUCCESS)
        {
            fprintf(stderr, "uph_audio_engine_init: failed to init playback device (%s)\n", ma_result_description(result));
            return;
        }
    }

    ma_device_start(&uph_audio_engine_data.device);
}

void uph_audio_engine_shutdown(void)
{
    ma_device_uninit(&uph_audio_engine_data.device);
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

double uph_audio_engine_get_song_length_beats(void)
{
    Uph_Project *project = &uph_state.project;
    double max_end_beat = 0.0;
    uint64_t track_count = naui_list_len(project->tracks);

    for (uint64_t t = 0; t < track_count; t++)
    {
        Uph_Track *track = &project->tracks[t];
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

void uph_audio_engine_stop_all_notes(void)
{
    Uph_Project *project = &uph_state.project;
    uint64_t track_count = naui_list_len(project->tracks);

    for (uint64_t t = 0; t < track_count; t++)
    {
        Uph_Track *track = &project->tracks[t];

        if (track->type != UPH_RESOURCE_PATTERN || !track->instrument.loaded)
            continue;

        uph_plugin_queue_stop_all(&track->instrument, 0);
    }
}