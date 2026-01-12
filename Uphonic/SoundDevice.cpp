#include "SoundDevice.h"
#include <iostream>
#include <algorithm>
#include <cmath>

struct SoundDeviceData
{
    ma_device device;
    double lastProcessedBeat;
    bool needsNoteRetrigger;
    std::vector<std::pair<Track*, bool>> trackHadActiveBlock;
    
    static constexpr float PEAK_DECAY_RATE = 0.9995f; // Decay per sample
};

static SoundDeviceData s_data = { {}, -1.0, true, {} };

inline float CubicInterpolate(float y0, float y1, float y2, float y3, float mu)
{
    const float mu2 = mu * mu;
    const float a0 = y3 - y2 - y0 + y1;
    const float a1 = y0 - y1 - a0;
    const float a2 = y2 - y0;
    const float a3 = y1;
    
    return a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3;
}

// Calculate stereo gains from pan (0.0 = left, 0.5 = center, 1.0 = right)
inline void CalculatePanGains(float pan, float& leftGain, float& rightGain)
{
    pan = std::max(0.0f, std::min(1.0f, pan));
    const float pan_radians = pan * 1.5707963267948966f; // pan * (PI/2)
    leftGain = cosf(pan_radians);
    rightGain = sinf(pan_radians);
}

// Update peak values with decay
inline void UpdatePeaks(float& peakLeft, float& peakRight, float newLeft, float newRight)
{
    // Take absolute values
    newLeft = std::abs(newLeft);
    newRight = std::abs(newRight);
    
    // Update peaks if new values are higher, otherwise decay
    peakLeft = std::max(newLeft, peakLeft * SoundDeviceData::PEAK_DECAY_RATE);
    peakRight = std::max(newRight, peakRight * SoundDeviceData::PEAK_DECAY_RATE);
}

bool SoundDevice::Initialize(void)
{
    ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = ma_format_f32;
    device_config.playback.channels = 2;
    device_config.sampleRate        = Core::settings.sampleRate;
    device_config.dataCallback      = AudioCallback;
    
    if (ma_device_init(NULL, &device_config, &s_data.device) != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize audio device\n";
        return false;
    }
    
    if (ma_device_start(&s_data.device) != MA_SUCCESS)
    {
        std::cerr << "Failed to start audio device\n";
        ma_device_uninit(&s_data.device);
        return false;
    }
    
    return true;
}

void SoundDevice::Shutdown(void)
{
    ma_device_stop(&s_data.device);
    ma_device_uninit(&s_data.device);
}

void SoundDevice::AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    float* output = (float*)pOutput;
    memset(pOutput, 0, frameCount * 2 * sizeof(float));

    float _inputs[64][512] = { 0 };
    float _outputs[64][512] = { 0 };
    
    static float *inputs[64] = { 0 };
    static float *outputs[64] = { 0 };
    for (int i = 0; i < 64; ++i)
    {
        inputs[i] = _inputs[i];
        outputs[i] = _outputs[i];
    }
    
    // Temporary buffers for track processing
    float trackBuffer[512 * 2] = { 0 };
    
    // Reset all track peaks at the start of the callback
    for (Track &track : Core::tracks)
    {
        track.peakLeft = 0.0f;
        track.peakRight = 0.0f;
    }
    Core::masterTrack.peakLeft = 0.0f;
    Core::masterTrack.peakRight = 0.0f;
    
    if (Core::isPlayingTimeline && !Core::isDraggingHandle)
    {
        const double prev_beat = Core::timelinePosition;
        const float sec_per_beat = 60.0f / Core::bpm;
        const double beats_per_frame = 1.0 / (pDevice->sampleRate * sec_per_beat);
        const double new_beat = prev_beat + frameCount * beats_per_frame;
        
        // Detect if playback jumped (seek, play button pressed, or loop)
        const bool playback_jumped = (s_data.lastProcessedBeat < 0 || 
                                      prev_beat < s_data.lastProcessedBeat || 
                                      prev_beat > s_data.lastProcessedBeat + beats_per_frame * frameCount * 2);
        
        if (playback_jumped)
        {
            s_data.needsNoteRetrigger = true;
        }
        
        // Detect note starts and ends on each midi track
        for (Track &track : Core::tracks)
        {
            if (track.muted)
                continue;

            if (track.type == TrackType_Midi && track.instrument.plugin)
            {
                bool track_has_active_block = false;
                
                for (const TimelineBlock &block : track.blocks)
                {
                    // Check if block is active in this time range
                    const double block_end = block.start + block.length - 0.005;
                    if (block_end < prev_beat || block.start > new_beat)
                        continue;
                    
                    track_has_active_block = true;
                    
                    // Get the pattern for this block
                    if (block.patternIndex >= Core::patterns.size())
                        continue;
                    
                    const MidiPattern &pattern = Core::patterns[block.patternIndex];
                    
                    // Process each note in the pattern
                    for (const MidiNote &note : pattern.notes)
                    {
                        // Calculate absolute note position in timeline
                        const double note_start = block.start + note.start - block.startOffset;
                        const double note_end = note_start + note.length - 0.005;
                        
                        // Note end is limited by block end (whichever comes first)
                        const double effective_note_end = std::min(note_end, block_end);
                        
                        // Check if note was cut off by startOffset but should still be playing
                        const bool note_cut_by_offset = note.start < block.startOffset;
                        const bool note_active_at_block_start = note_cut_by_offset && 
                                                                (note.start + note.length) > block.startOffset;
                        
                        // If playback just jumped and we're in the middle of a note, trigger it
                        if (s_data.needsNoteRetrigger && note_start < prev_beat && effective_note_end > prev_beat)
                        {
                            int32_t sample_offset = (127 - note.key);
                            const int32_t clamped_offset = std::max(0, std::min((int32_t)frameCount - 1, sample_offset));
                            track.instrument.plugin->PlayNote(note.key, note.velocity, clamped_offset);
                        }
                        // Special case: if block just started and note was cut by offset, trigger it
                        else if (note_active_at_block_start && block.start >= prev_beat && block.start < new_beat)
                        {
                            const double beats_from_buffer_start = block.start - prev_beat;
                            int32_t sample_offset = (int32_t)(beats_from_buffer_start / beats_per_frame);
                            sample_offset += (127 - note.key);
                            const int32_t clamped_offset = std::max(0, std::min((int32_t)frameCount - 1, sample_offset));
                            track.instrument.plugin->PlayNote(note.key, note.velocity, clamped_offset);
                        }
                        // Check if note starts within this buffer (only if not cut by offset)
                        else if (!note_cut_by_offset && note_start >= prev_beat && note_start < new_beat)
                        {
                            const double beats_from_buffer_start = note_start - prev_beat;
                            int32_t sample_offset = (int32_t)(beats_from_buffer_start / beats_per_frame);
                            sample_offset += (127 - note.key);
                            const int32_t clamped_offset = std::max(0, std::min((int32_t)frameCount - 1, sample_offset));
                            track.instrument.plugin->PlayNote(note.key, note.velocity, clamped_offset);
                        }
                        
                        // Check if note ends within this buffer (using effective end time)
                        if (effective_note_end >= prev_beat && effective_note_end < new_beat)
                        {
                            const double beats_from_buffer_start = effective_note_end - prev_beat;
                            int32_t sample_offset = (int32_t)(beats_from_buffer_start / beats_per_frame);
                            sample_offset += (127 - note.key);
                            const int32_t clamped_offset = std::max(0, std::min((int32_t)frameCount - 1, sample_offset));
                            track.instrument.plugin->StopNote(note.key, clamped_offset);
                        }
                    }
                }
                
                // Check if this track had an active block in the previous frame but not anymore
                auto it = std::find_if(s_data.trackHadActiveBlock.begin(), s_data.trackHadActiveBlock.end(),
                    [&track](const std::pair<Track*, bool>& p) { return p.first == &track; });
                
                if (it != s_data.trackHadActiveBlock.end())
                {
                    if (it->second && !track_has_active_block)
                    {
                        for (int key = 0; key < 128; ++key)
                        {
                            track.instrument.plugin->StopNote(key, 0);
                        }
                    }
                    it->second = track_has_active_block;
                }
                else
                {
                    s_data.trackHadActiveBlock.push_back({&track, track_has_active_block});
                }
            }
            else if (track.type == TrackType_Sample)
            {
                // Clear track buffer for this track
                memset(trackBuffer, 0, frameCount * 2 * sizeof(float));
                
                // Calculate pan gains for this track
                float leftGain, rightGain;
                CalculatePanGains(track.pan, leftGain, rightGain);
                
                // Process sample blocks into track buffer
                for (const TimelineBlock &block : track.blocks)
                {
                    const double block_end = block.start + block.length - 0.005;
                    if (block_end <= prev_beat || block.start >= new_beat)
                        continue;
                    
                    if (block.sampleIndex >= Core::samples.size())
                        continue;
                    
                    const Sample &sample = Core::samples[block.sampleIndex];

                    if (!sample.frames || sample.frameCount == 0)
                        continue;

                    const float sec_per_beat = 60.0f / Core::bpm;
                    
                    for (ma_uint32 i = 0; i < frameCount; ++i)
                    {
                        const double current_beat = prev_beat + i * beats_per_frame;
                        
                        if (current_beat < block.start || current_beat >= block_end)
                            continue;
                        
                        const double beat_in_block = current_beat - block.start + block.startOffset;
                        const double sec_in_block = beat_in_block * sec_per_beat;
                        const double sample_time = sec_in_block / block.stretchScale;
                        const double sample_pos = sample_time * sample.sampleRate;
                        
                        if (sample_pos < 0 || sample_pos >= sample.frameCount)
                            continue;
                        
                        const uint64_t sample_index = (uint64_t)sample_pos;
                        const float frac = sample_pos - sample_index;
                        
                        if (sample.type == SampleType::Stereo)
                        {
                            const uint64_t frame_offset = sample_index * 2;
                            
                            if (sample_index + 1 < sample.frameCount)
                            {
                                float left, right;
                                
                                if (sample_index > 0 && sample_index + 2 < sample.frameCount)
                                {
                                    const float* s = &sample.frames[frame_offset];
                                    left = CubicInterpolate(s[-2], s[0], s[2], s[4], frac);
                                    right = CubicInterpolate(s[-1], s[1], s[3], s[5], frac);
                                }
                                else
                                {
                                    left = sample.frames[frame_offset] + 
                                        (sample.frames[frame_offset + 2] - sample.frames[frame_offset]) * frac;
                                    right = sample.frames[frame_offset + 1] + 
                                            (sample.frames[frame_offset + 3] - sample.frames[frame_offset + 1]) * frac;
                                }
                                
                                // Write to track buffer (without volume/pan yet)
                                trackBuffer[i * 2]     += left;
                                trackBuffer[i * 2 + 1] += right;
                            }
                        }
                        else // SampleType::Mono
                        {
                            float mono_sample;
                            
                            if (sample_index + 1 < sample.frameCount)
                            {
                                if (sample_index > 0 && sample_index + 2 < sample.frameCount)
                                {
                                    mono_sample = CubicInterpolate(
                                        sample.frames[sample_index - 1],
                                        sample.frames[sample_index],
                                        sample.frames[sample_index + 1],
                                        sample.frames[sample_index + 2],
                                        frac
                                    );
                                }
                                else
                                {
                                    mono_sample = sample.frames[sample_index] + 
                                                (sample.frames[sample_index + 1] - sample.frames[sample_index]) * frac;
                                }
                            }
                            else
                            {
                                mono_sample = sample.frames[sample_index];
                            }
                            
                            // Write to track buffer (mono to stereo, without volume/pan yet)
                            trackBuffer[i * 2]     += mono_sample;
                            trackBuffer[i * 2 + 1] += mono_sample;
                        }
                    }
                }
                
                // Process track effects chain
                if (!track.effects.empty())
                {
                    // Deinterleave for VST processing
                    for (ma_uint32 i = 0; i < frameCount; ++i)
                    {
                        outputs[0][i] = trackBuffer[i * 2];
                        outputs[1][i] = trackBuffer[i * 2 + 1];
                    }
                    
                    // Process each effect in the chain
                    for (Effect &effect : track.effects)
                    {
                        if (effect.plugin)
                        {
                            // Copy outputs to inputs for the next effect
                            memcpy(inputs[0], outputs[0], frameCount * sizeof(float));
                            memcpy(inputs[1], outputs[1], frameCount * sizeof(float));
                            
                            effect.plugin->Process((float**)inputs, (float**)outputs, frameCount);
                        }
                    }
                    
                    // Interleave back to track buffer
                    for (ma_uint32 i = 0; i < frameCount; ++i)
                    {
                        trackBuffer[i * 2]     = outputs[0][i];
                        trackBuffer[i * 2 + 1] = outputs[1][i];
                    }
                }
                
                // Apply volume and pan, then add to output
                for (ma_uint32 i = 0; i < frameCount; ++i)
                {
                    const float leftOut = trackBuffer[i * 2] * track.volume * leftGain;
                    const float rightOut = trackBuffer[i * 2 + 1] * track.volume * rightGain;
                    
                    output[i * 2]     += leftOut;
                    output[i * 2 + 1] += rightOut;
                    
                    UpdatePeaks(track.peakLeft, track.peakRight, leftOut, rightOut);
                }
            }
        }
        
        s_data.needsNoteRetrigger = false;
        s_data.lastProcessedBeat = new_beat;
        Core::timelinePosition = new_beat;
    }
    else
    {
        s_data.lastProcessedBeat = -1.0;
        s_data.needsNoteRetrigger = true;
    }
    
    // Process all MIDI tracks (VST/instrument processing)
    for (Track &track : Core::tracks)
    {
        if (track.instrument.plugin)
        {
            // Clear track buffer
            memset(trackBuffer, 0, frameCount * 2 * sizeof(float));
            
            // Process instrument
            track.instrument.plugin->Process((float**)inputs, (float**)outputs, frameCount);
            
            // Copy to track buffer
            for (ma_uint32 i = 0; i < frameCount; i++)
            {
                trackBuffer[i * 2]     = outputs[0][i];
                trackBuffer[i * 2 + 1] = outputs[1][i];
            }
            
            // Process track effects chain
            if (!track.effects.empty())
            {
                // Deinterleave
                for (ma_uint32 i = 0; i < frameCount; ++i)
                {
                    outputs[0][i] = trackBuffer[i * 2];
                    outputs[1][i] = trackBuffer[i * 2 + 1];
                }
                
                // Process each effect in the chain
                for (Effect &effect : track.effects)
                {
                    if (effect.plugin)
                    {
                        memcpy(inputs[0], outputs[0], frameCount * sizeof(float));
                        memcpy(inputs[1], outputs[1], frameCount * sizeof(float));
                        
                        effect.plugin->Process((float**)inputs, (float**)outputs, frameCount);
                    }
                }
                
                // Interleave back
                for (ma_uint32 i = 0; i < frameCount; ++i)
                {
                    trackBuffer[i * 2]     = outputs[0][i];
                    trackBuffer[i * 2 + 1] = outputs[1][i];
                }
            }
            
            // Calculate pan gains and apply volume
            float leftGain, rightGain;
            CalculatePanGains(track.pan, leftGain, rightGain);
            
            for (ma_uint32 i = 0; i < frameCount; i++)
            {
                const float leftOut = trackBuffer[i * 2] * track.volume * leftGain;
                const float rightOut = trackBuffer[i * 2 + 1] * track.volume * rightGain;
                
                output[i * 2]     += leftOut;
                output[i * 2 + 1] += rightOut;
                
                UpdatePeaks(track.peakLeft, track.peakRight, leftOut, rightOut);
            }
        }
    }
    
    // Process master track effects
    if (!Core::masterTrack.effects.empty())
    {
        // Deinterleave master output
        for (ma_uint32 i = 0; i < frameCount; ++i)
        {
            outputs[0][i] = output[i * 2];
            outputs[1][i] = output[i * 2 + 1];
        }
        
        // Process each master effect in the chain
        for (Effect &effect : Core::masterTrack.effects)
        {
            if (effect.plugin)
            {
                memcpy(inputs[0], outputs[0], frameCount * sizeof(float));
                memcpy(inputs[1], outputs[1], frameCount * sizeof(float));
                
                effect.plugin->Process((float**)inputs, (float**)outputs, frameCount);
            }
        }
        
        // Interleave back to output
        for (ma_uint32 i = 0; i < frameCount; ++i)
        {
            output[i * 2]     = outputs[0][i];
            output[i * 2 + 1] = outputs[1][i];
        }
    }
    
    // Apply master volume and pan
    {
        float leftGain, rightGain;
        CalculatePanGains(Core::masterTrack.pan, leftGain, rightGain);
        const float masterVolume = Core::masterTrack.volume;
        leftGain *= masterVolume / cosf(0.7853982f);
        rightGain *= masterVolume / sinf(0.7853982f);
        
        for (ma_uint32 i = 0; i < frameCount; i++)
        {
            const float leftOut = output[i * 2] * leftGain;
            const float rightOut = output[i * 2 + 1] * rightGain;

            output[i * 2]     = leftOut;
            output[i * 2 + 1] = rightOut;

            UpdatePeaks(Core::masterTrack.peakLeft, Core::masterTrack.peakRight, leftOut, rightOut);
        }
    }
}

bool SoundDevice::ExportToWav(const char* filepath, double startBeat, double endBeat)
{
    if (endBeat <= startBeat)
    {
        std::cerr << "Invalid beat range for export\n";
        return false;
    }
    
    // Calculate total duration
    const float sec_per_beat = 60.0f / Core::bpm;
    const double duration_beats = endBeat - startBeat;
    const double duration_sec = duration_beats * sec_per_beat;
    
    const ma_uint32 sampleRate = Core::settings.sampleRate;
    const ma_uint32 channels = 2;
    const ma_uint64 totalFrames = (ma_uint64)(duration_sec * sampleRate);
    
    std::cout << "Exporting " << duration_beats << " beats (" 
              << duration_sec << " seconds, " 
              << totalFrames << " frames) to " << filepath << "\n";
    
    // Initialize WAV encoder
    ma_encoder_config encoder_config = ma_encoder_config_init(
        ma_encoding_format_wav,
        ma_format_f32,
        channels,
        sampleRate
    );

    ma_encoder encoder;
    if (ma_encoder_init_file(filepath, &encoder_config, &encoder) != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize WAV encoder\n";
        return false;
    }
    
    // Save current playback state
    const double original_position = Core::timelinePosition;
    const bool was_playing = Core::isPlayingTimeline;
    const bool was_dragging = Core::isDraggingHandle;
    
    // Set up for export
    Core::timelinePosition = startBeat;
    Core::isPlayingTimeline = true;
    Core::isDraggingHandle = false;
    
    // Reset audio state
    s_data.lastProcessedBeat = -1.0;
    s_data.needsNoteRetrigger = true;
    s_data.trackHadActiveBlock.clear();
    
    // Process in chunks (same size as typical audio callback)
    const ma_uint32 chunkSize = 512;
    float* buffer = new float[chunkSize * channels];
    
    ma_uint64 framesProcessed = 0;
    
    // Create a fake device structure for the callback
    ma_device fake_device = s_data.device;
    fake_device.sampleRate = sampleRate;
    
    ma_device_stop(&s_data.device);

    while (framesProcessed < totalFrames)
    {
        const ma_uint32 framesToProcess = std::min(chunkSize, (ma_uint32)(totalFrames - framesProcessed));
        
        // Zero the buffer
        memset(buffer, 0, framesToProcess * channels * sizeof(float));
        
        // Call the audio callback to generate audio
        AudioCallback(&fake_device, buffer, nullptr, framesToProcess);
        
        // Write to file
        ma_uint64 framesWritten;
        if (ma_encoder_write_pcm_frames(&encoder, buffer, framesToProcess, &framesWritten) != MA_SUCCESS)
        {
            std::cerr << "Failed to write frames to WAV file\n";
            delete[] buffer;
            ma_encoder_uninit(&encoder);
            
            // Restore state
            Core::timelinePosition = original_position;
            Core::isPlayingTimeline = was_playing;
            Core::isDraggingHandle = was_dragging;
            s_data.lastProcessedBeat = -1.0;
            s_data.needsNoteRetrigger = true;
            
            return false;
        }
        
        framesProcessed += framesWritten;
        
        // Progress indicator
        if (framesProcessed % (sampleRate * 2) == 0) // Every 2 seconds
        {
            const float progress = (float)framesProcessed / totalFrames * 100.0f;
            std::cout << "Export progress: " << (int)progress << "%\n";
        }
    }
    
    delete[] buffer;
    StopAllNotes();
    ma_encoder_uninit(&encoder);
    
    // Restore original playback state
    Core::timelinePosition = original_position;
    Core::isPlayingTimeline = was_playing;
    Core::isDraggingHandle = was_dragging;
    s_data.lastProcessedBeat = -1.0;
    s_data.needsNoteRetrigger = true;
    s_data.trackHadActiveBlock.clear();
    ma_device_start(&s_data.device);

    std::cout << "Export complete: " << filepath << "\n";
    return true;
}

void SoundDevice::StopAllNotes(void)
{
    for (Track &track : Core::tracks)
        if (track.instrument.plugin)
            track.instrument.plugin->StopAllNotes();
}

Sample SoundDevice::LoadSample(const char* filepath)
{
    Sample outSample = {};
    
    ma_decoder decoder;
    ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 0, 0);
    
    if (ma_decoder_init_file(filepath, &decoder_config, &decoder) != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize decoder for: " << filepath << "\n";
        return outSample;
    }
    
    // Get the total frame count
    ma_uint64 frameCount;
    ma_result result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (result != MA_SUCCESS)
    {
        std::cerr << "Failed to get frame count for: " << filepath << "\n";
        ma_decoder_uninit(&decoder);
        return outSample;
    }
    
    // Allocate buffer for the decoded audio
    const ma_uint32 channels = decoder.outputChannels;
    const ma_uint32 sampleRate = decoder.outputSampleRate;
    const ma_uint64 totalSamples = frameCount * channels;
    
    float* frames = new float[totalSamples];
    
    // Decode the entire file
    ma_uint64 framesRead = 0;
    result = ma_decoder_read_pcm_frames(&decoder, frames, frameCount, &framesRead);
    
    ma_decoder_uninit(&decoder);
    
    if (result != MA_SUCCESS || framesRead == 0)
    {
        std::cerr << "Failed to decode audio from: " << filepath << "\n";
        delete[] frames;
        return outSample;
    }
    
    // Fill out the Sample struct
    const char* filename = filepath;
    const char* lastSlash = strrchr(filepath, '/');
    const char* lastBackslash = strrchr(filepath, '\\');
    if (lastSlash || lastBackslash)
    {
        filename = (lastSlash > lastBackslash) ? lastSlash + 1 : lastBackslash + 1;
    }
    
    outSample.name = filename;
    
    outSample.type = (channels == 2) ? SampleType::Stereo : SampleType::Mono;
    outSample.frames = frames;
    outSample.frameCount = framesRead;
    outSample.sampleRate = sampleRate;
    
    std::cout << "Loaded sample: " << outSample.name 
              << " (" << framesRead << " frames, " 
              << channels << " channels, " 
              << sampleRate << " Hz)\n";
    
    return outSample;
}

void SoundDevice::UnloadSample(Sample &sample)
{
    delete[] sample.frames;
}