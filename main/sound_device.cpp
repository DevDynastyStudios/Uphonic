#include "sound_device.h"

#include <miniaudio.h>

#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cmath>

constexpr float PI = 3.14159265358979323846f;
constexpr float HALF_PI = PI * 0.5f;

struct UphSoundDevice
{
    ma_device device;
    uint32_t block_size = 512;

    struct UphSoundDeviceIO
    {
        float inputs[64][512];
        float outputs[64][512];
    }
    *io;
};

static UphSoundDevice sound_device;

static void uph_midi_pattern_process_playback_for_block(
    AEffect *effect, UphMidiPattern *pattern,
    float sec_per_beat, float prev_beat, float new_beat,
    float sample_rate, float frame_count,
    float start_time = 0.0f,
    float start_offset = 0.0f,
    float length = INT32_MAX
)
{
    constexpr int maxEvents = 256;
    VstMidiEvent midiEvents[maxEvents]{};
    uint32_t midiEventCount = 0;

    int microOffset = 0;
    int lastSampleOffset = -1;

    int noteOnMicroOffset = 0;
    int noteOffMicroOffset = 0;
    int lastNoteOnSample = -1;
    int lastNoteOffSample = -1;

    auto queue_event = [&](int status, int pitch, int velocity, float event_beat)
    {
        if (midiEventCount >= maxEvents) return;

        float noteTimeSec = event_beat * sec_per_beat;
        float prevTimeSec = prev_beat * sec_per_beat;
        int sample_offset = int((noteTimeSec - prevTimeSec) * sample_rate);
        sample_offset = std::clamp(sample_offset, 0, (int)frame_count - 1);

        if (status == 0x90) // Note on
        {
            if (sample_offset == lastNoteOnSample)
            {
                noteOnMicroOffset++;
                sample_offset = std::clamp(sample_offset + noteOnMicroOffset, 0, (int)frame_count - 1);
            } else {
                noteOnMicroOffset = 0;
                lastNoteOnSample = sample_offset;
            }
        }
        else if (status == 0x80) // Note off
        {
            if (sample_offset == lastNoteOffSample)
            {
                noteOffMicroOffset++;
                sample_offset = std::clamp(sample_offset + noteOffMicroOffset, 0, (int)frame_count - 1);
            } else {
                noteOffMicroOffset = 0;
                lastNoteOffSample = sample_offset;
            }
        }

        VstMidiEvent &ev = midiEvents[midiEventCount++];
        ev.type = kVstMidiType;
        ev.byteSize = sizeof(ev);
        ev.midiData[0] = (unsigned char)status;
        ev.midiData[1] = (unsigned char)pitch;
        ev.midiData[2] = (unsigned char)velocity;
        ev.deltaFrames = sample_offset;
    };

    for (auto &note : pattern->notes)
    {
        const float original_start = note.start + start_time;
        const float original_end   = original_start + note.length - 0.001f;

        const float note_start = std::clamp<float>(
            original_start - start_offset,
            start_time,
            start_time + length
        );

        const float note_end = std::clamp<float>(
            original_end - start_offset,
            0.0f,
            start_time + length
        );

        if (note_end <= note_start)
            continue;

        if (note_end >= prev_beat && note_end < new_beat)
            queue_event(0x80, note.key, 0, note_end);

        if (note_start >= prev_beat && note_start < new_beat)
            queue_event(0x90, note.key, note.velocity, note_start);
    }

    if (midiEventCount > 0)
    {
        VstEvents events{};
        events.numEvents = midiEventCount;
        for (int i = 0; i < midiEventCount; ++i)
            events.events[i] = (VstEvent*)&midiEvents[i];
        effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);
    }
}

static void uph_midi_editor_process_playback_for_block(float sample_rate, float frame_count)
{
    AEffect *effect = app->project.tracks[app->current_track_index].instrument.effect;
    if (!effect)
        return;

    const float sec_per_beat = 60.0f / app->project.bpm;
    float prev_beat = app->midi_editor_song_position;
    float new_beat = prev_beat + frame_count / sample_rate / sec_per_beat;

    uph_midi_pattern_process_playback_for_block(
        effect,
        &app->project.patterns[app->current_pattern_index],
        sec_per_beat, prev_beat, new_beat,
        sample_rate, frame_count
    );

    app->midi_editor_song_position = new_beat;
}

static void uph_song_timeline_process_playback_for_block(float sample_rate, float frame_count)
{
    const float prev_beat = app->song_timeline_song_position;

    const float sec_per_beat = 60.0f / app->project.bpm;
    float new_beat = prev_beat + frame_count / sample_rate / sec_per_beat;

    const std::vector<UphTrack> &tracks = app->project.tracks;
    for (auto &track : tracks)
    {
        if (track.track_type == UphTrackType_Midi)
        {
            AEffect *effect = track.instrument.effect;
            if (!effect)
                continue;
            for (auto &pattern_instance : track.timeline_blocks)
            {
                UphMidiPattern &pattern = app->project.patterns[pattern_instance.pattern_index];
                uph_midi_pattern_process_playback_for_block(
                    effect,
                    &pattern,
                    sec_per_beat, prev_beat, new_beat,
                    sample_rate, frame_count,
                    pattern_instance.start_time,
                    pattern_instance.start_offset,
                    pattern_instance.length
                );
            }
        }
    }

    app->song_timeline_song_position = new_beat;
}

static inline void uph_audio_stop_all_notes(const std::vector<UphTrack> &tracks)
{
    for (auto &track : tracks)
    {
        if (track.track_type == UphTrackType_Midi)
        {
            AEffect *effect = track.instrument.effect;
            if (!effect)
                continue;

            VstEvents events{};
            VstMidiEvent midiEvents[16]{};
            
            events.numEvents = 16;
            events.reserved = 0;
            
            for (int channel = 0; channel < 16; channel++)
            {
                midiEvents[channel].type = kVstMidiType;
                midiEvents[channel].byteSize = sizeof(VstMidiEvent);
                midiEvents[channel].midiData[0] = 0xB0 | channel;
                midiEvents[channel].midiData[1] = 123;
                midiEvents[channel].midiData[2] = 0;

                events.events[channel] = (VstEvent*)&midiEvents[channel];
            }
            
            effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);
        }
    }
}

static inline float db_to_lin(float db) { return pow(10.0f, db / 20.0f); }
static inline float lin_to_db(float lin) { return 20.0f * log10(lin); }

static void uph_audio_callback(ma_device* p_device, void* p_output, const void* p_input, ma_uint32 frame_count)
{
    float* output = (float*)p_output;
    std::vector<UphTrack> &tracks = app->project.tracks;

    static float *inputs[64] = { 0 };
    static float *outputs[64] = { 0 };

    for (int i = 0; i < 64; ++i)
    {
        inputs[i] = sound_device.io->inputs[i];
        outputs[i] = sound_device.io->outputs[i];
    }

    if (app->should_stop_all_notes.load())
    {
        uph_audio_stop_all_notes(tracks);
        app->should_stop_all_notes.store(false);
    }
    else if (app->is_midi_editor_playing)
        uph_midi_editor_process_playback_for_block(p_device->sampleRate, frame_count);
    else if (app->is_song_timeline_playing)
        uph_song_timeline_process_playback_for_block(p_device->sampleRate, frame_count);

    for (auto &track : tracks)
    {
        if (track.muted)
        {
            track.peak_left = 0.0f;
            track.peak_right = 0.0f;
            continue;
        }

        memset(sound_device.io->inputs, 0, sizeof(float) * 512 * 64);
        memset(sound_device.io->outputs, 0, sizeof(float) * 512 * 64);

        if (track.track_type == UphTrackType_Midi)
        {
            AEffect *effect = track.instrument.effect;
            if (!effect)
                continue;

            if (effect->flags & effFlagsCanReplacing)
                effect->processReplacing(effect, inputs, outputs, frame_count);
        }
        else if (app->is_song_timeline_playing && track.track_type == UphTrackType_Sample)
        {
            for (auto &sample_instance : track.timeline_blocks)
            {
                if (sample_instance.sample_index < 0 || sample_instance.sample_index >= (int)app->project.samples.size())
                    continue;

                const UphSample &sample = app->project.samples[sample_instance.sample_index];
                if (!sample.frames || sample.frame_count == 0)
                    continue;

                const float sec_per_beat = 60.0f / app->project.bpm;

                float sample_rate_ratio = (float)sample.sample_rate / (float)p_device->sampleRate;
                float playback_speed = sample_rate_ratio / sample_instance.stretch_scale;
                // --- NEW: playback speed multiplier ---
                float playback_rate = (playback_speed > 0.0f) ? playback_speed : 1.0f;

                float instance_start_beat = sample_instance.start_time;

                // FIX: adjust length to account for playback speed
                float instance_length_beats = (sample_instance.length > 0.0f)
                    ? sample_instance.length
                    : (sample.frame_count / (float)p_device->sampleRate / sec_per_beat) / playback_rate;

                float instance_end_beat = instance_start_beat + instance_length_beats;

                const float prev_beat = app->song_timeline_song_position;
                if (instance_end_beat <= prev_beat || instance_start_beat >= app->song_timeline_song_position)
                    continue;

                int block_start_sample = int(std::round((instance_start_beat - prev_beat) * sec_per_beat * p_device->sampleRate));
                int sample_read_start  = int(std::round(sample_instance.start_offset * sec_per_beat * p_device->sampleRate * playback_rate));
                if (sample_read_start < 0) sample_read_start = 0;

                int write_i = std::max<int>(0, block_start_sample);

                // read_index is now floating-point so we can step at playback_rate
                double read_index = (double)sample_read_start + (double)(write_i - block_start_sample) * playback_rate;

                ma_uint64 available_in_sample = (sample.frame_count > (ma_uint64)read_index) ? sample.frame_count - (ma_uint64)read_index : 0;
                int available_in_block = (int)frame_count - write_i;
                int frames_to_copy = (int)std::min<ma_uint64>(available_in_sample, (ma_uint64)available_in_block);
                if (frames_to_copy <= 0) continue;

                const int sample_channels = (sample.type == UphSampleType_Mono) ? 1 : 2;
                const float *src = sample.frames;
                const float track_volume = track.volume;

                for (int i = 0; i < frames_to_copy; ++i)
                {
                    // float index to handle speed change
                    double sample_frame_idx_f = read_index + (double)i * playback_rate;
                    ma_uint64 base = (ma_uint64)sample_frame_idx_f;

                    float left_sample = 0.0f;
                    float right_sample = 0.0f;

                    // linear interpolation to reduce aliasing
                    float frac = (float)(sample_frame_idx_f - (double)base);

                    if (sample_channels == 1)
                    {
                        float s1 = src[base];
                        float s2 = (base + 1 < sample.frame_count) ? src[base + 1] : 0.0f;
                        float sample_val = s1 + (s2 - s1) * frac;

                        left_sample = sample_val;
                        right_sample = sample_val;
                    }
                    else
                    {
                        ma_uint64 base2 = base * 2;
                        float l1 = src[base2];
                        float l2 = (base2 + 2 < sample.frame_count * 2) ? src[base2 + 2] : 0.0f;
                        float r1 = src[base2 + 1];
                        float r2 = (base2 + 3 < sample.frame_count * 2) ? src[base2 + 3] : 0.0f;

                        left_sample  = l1 + (l2 - l1) * frac;
                        right_sample = r1 + (r2 - r1) * frac;
                    }

                    int out_idx = write_i + i;
                    
                    sound_device.io->outputs[0][out_idx] += left_sample;
                    sound_device.io->outputs[1][out_idx] += right_sample;
                }
            }
        }

        float peakL = 0.0f;
        float peakR = 0.0f;

        float panNorm = (track.pan + 1.0f) * 0.5f;
        float gainL = cosf(panNorm * HALF_PI);
        float gainR = sinf(panNorm * HALF_PI);
        
        const float final_volume = app->project.volume;

        for (ma_uint32 i = 0; i < frame_count; i++)
        {
            const float l = sound_device.io->outputs[0][i] * track.volume * gainL;
            const float r = sound_device.io->outputs[1][i] * track.volume * gainR;

            peakL = std::max<float>(peakL, fabsf(l));
            peakR = std::max<float>(peakR, fabsf(r));

            output[i * 2]     += l * final_volume;
            output[i * 2 + 1] += r * final_volume;
        }

        track.peak_left = peakL;
        track.peak_right = peakR;
    }
}

// TODO: Add Samples
/*
if (track.track_type == UphTrackType_Sample)
{
    for (auto &sample_instance : track.timeline_blocks)
    {
        UphSample &sample = app->project.samples[sample_instance.sample_index];
        if (!sample.frames)
            continue;
        // sample.frames
        // sample.frame_count
        // sample.channel_count
    }
}
*/

void uph_sound_device_initialize(void)
{
    sound_device.io = new UphSoundDevice::UphSoundDeviceIO;

    ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = ma_format_f32;
    device_config.playback.channels = 2;
    device_config.sampleRate        = 44100;
    device_config.dataCallback      = uph_audio_callback;

    if (ma_device_init(NULL, &device_config, &sound_device.device) != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize audio device\n";
        return;
    }

    if (ma_device_start(&sound_device.device) != MA_SUCCESS)
    {
        std::cerr << "Failed to start audio device\n";
        ma_device_uninit(&sound_device.device);
        return;
    }
}

void uph_sound_device_shutdown(void)
{
    ma_device_stop(&sound_device.device);
    ma_device_uninit(&sound_device.device);
    delete sound_device.io;
}

void uph_sound_device_all_notes_off(void)
{
    app->should_stop_all_notes.store(true);
}

float uph_get_song_length_sec(void)
{
    float max_length = 0.0f;
    const std::vector<UphTrack> &tracks = app->project.tracks;
    const float sec_per_beat = 60.0f / app->project.bpm;

    for (auto &track : tracks)
    {
        for (auto &pattern_instance : track.timeline_blocks)
        {
            float pattern_end_beat = pattern_instance.start_time + pattern_instance.length;
            float pattern_end_sec = pattern_end_beat * sec_per_beat;
            if (pattern_end_sec > max_length)
                max_length = pattern_end_sec;
        }
    }

    return max_length;
}


UphSample uph_create_sample_from_file(const char *path)
{
    namespace fs = std::filesystem;
    ma_decoder decoder;
    if (ma_decoder_init_file(path, NULL, &decoder) != MA_SUCCESS)
    {
        printf("Failed to load sample %s\n", path);
        return {};
    }

    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

    float* pFrames = (float*)malloc((size_t)(frameCount * decoder.outputChannels * sizeof(float)));

    ma_uint64 readFrames = 0;
    ma_decoder_read_pcm_frames(&decoder, pFrames, frameCount, &readFrames);

    UphSample sample;
    sample.type = decoder.outputChannels == 1 ?
        UphSampleType_Mono :
        UphSampleType_Stereo;
    sample.frames = pFrames;
    sample.frame_count = frameCount;
    sample.sample_rate = decoder.outputSampleRate;
    strcpy(sample.name, fs::path(path).stem().string().c_str());

    ma_decoder_uninit(&decoder);

    return sample;
}

void uph_destroy_sample(const UphSample *sample)
{
    free(sample->frames);
}