#include "AudioEngine.h"
#include "../Core/ProjectState.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

AudioEngineData AudioEngine::s_data = {};
AudioConfig AudioEngine::s_config;

static constexpr float PI_HALF = 1.5707963267948966f;
static constexpr float SECONDS_PER_MINUTE = 60.0f;
static constexpr int MAX_PLUGIN_CHANNELS = 64;

float AudioEngine::CubicInterpolate(float y0, float y1, float y2, float y3, float mu)
{
    const float muSquared = mu * mu;
    const float a0 = y3 - y2 - y0 + y1;
    const float a1 = y0 - y1 - a0;
    const float a2 = y2 - y0;
    const float a3 = y1;
    
    return a0 * mu * muSquared + a1 * muSquared + a2 * mu + a3;
}

void AudioEngine::CalculatePanGains(float pan, float& leftGain, float& rightGain)
{
    pan = std::max(0.0f, std::min(1.0f, pan));
    const float panRadians = pan * PI_HALF;
    leftGain = cosf(panRadians);
    rightGain = sinf(panRadians);
}

void AudioEngine::UpdatePeaks(float& peakLeft, float& peakRight, float newLeft, float newRight)
{
    newLeft = std::abs(newLeft);
    newRight = std::abs(newRight);
    
    peakLeft = std::max(newLeft, peakLeft * s_config.peakDecayRate);
    peakRight = std::max(newRight, peakRight * s_config.peakDecayRate);
}

bool AudioEngine::Initialize(const AudioConfig& config)
{
    s_config = config;
    
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = s_config.channels;
    deviceConfig.sampleRate = s_config.sampleRate;
    deviceConfig.dataCallback = AudioCallback;
    
    if (ma_device_init(nullptr, &deviceConfig, &s_data.device) != MA_SUCCESS)
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
    
    s_data.lastProcessedBeat = -1.0;
    s_data.needsNoteRetrigger = true;
    
    return true;
}

void AudioEngine::Shutdown()
{
    ma_device_stop(&s_data.device);
    ma_device_uninit(&s_data.device);
}

void AudioEngine::StopAllNotes()
{
    ProjectState& state = ProjectState::GetInstance();
    for (AudioTrack& track : state.tracks)
    {
        if (track.instrument.plugin)
        {
            track.instrument.plugin->StopAllNotes();
        }
    }
}

void AudioEngine::AddSample(const char* filepath)
{
	ProjectState &state = ProjectState::GetInstance();
    if (AudioSample sample; AudioEngine::LoadSample(filepath, sample))
        state.samples.push_back(sample);
}

bool AudioEngine::LoadSample(const char* filepath, AudioSample &sample)
{    
    ma_decoder decoder;
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
    
    if (ma_decoder_init_file(filepath, &decoderConfig, &decoder) != MA_SUCCESS)
    {
        Naui::Debug::Error("Failed to initialize decoder for: %s", filepath);
        return false;
    }
    
    ma_uint64 frameCount;
    ma_result result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (result != MA_SUCCESS)
    {
        Naui::Debug::Error("Failed to get frame count for: %s", filepath);
        ma_decoder_uninit(&decoder);
        return false;
    }
    
    const ma_uint32 channels = decoder.outputChannels;
    const ma_uint32 sampleRate = decoder.outputSampleRate;
    const ma_uint64 totalSamples = frameCount * channels;
    
    float* frames = new float[totalSamples];
    
    ma_uint64 framesRead = 0;
    result = ma_decoder_read_pcm_frames(&decoder, frames, frameCount, &framesRead);
    
    ma_decoder_uninit(&decoder);
    
    if (result != MA_SUCCESS || framesRead == 0)
    {
        Naui::Debug::Error("Failed to decode audio from: %s", filepath);
        delete[] frames;
        return false;
    }
    
    const char* filename = filepath;
    const char* lastSlash = strrchr(filepath, '/');
    const char* lastBackslash = strrchr(filepath, '\\');
    if (lastSlash || lastBackslash)
    {
        filename = (lastSlash > lastBackslash) ? lastSlash + 1 : lastBackslash + 1;
    }
    
    sample.name = filename;
	sample.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    sample.filePath = filepath;
    sample.channelType = (channels == 2) ? SampleChannelType::Stereo : SampleChannelType::Mono;
    sample.frameData = frames;
    sample.frameCount = framesRead;
    sample.sampleRate = sampleRate;
    
    std::cout << "Loaded sample: " << sample.name 
              << " (" << framesRead << " frames, " 
              << channels << " channels, " 
              << sampleRate << " Hz)\n";
    
    return true;
}

void AudioEngine::UnloadSample(AudioSample& sample)
{
    delete[] sample.frameData;
    sample.frameData = nullptr;
}

void AudioEngine::ProcessMidiTrack(AudioTrack& track, double prevBeat, double newBeat,
                                   double beatsPerFrame, ma_uint32 frameCount,
                                   float** inputs, float** outputs)
{
    ProjectState& state = ProjectState::GetInstance();
    bool trackHasActiveBlock = false;
    
    for (const TimelineBlock& block : track.blocks)
    {
        const double blockEnd = block.midiBlock.startBeat + block.midiBlock.lengthBeats - s_config.blockEndEpsilon;
        if (blockEnd < prevBeat || block.midiBlock.startBeat > newBeat)
            continue;
        
        trackHasActiveBlock = true;
        
        if (block.midiBlock.patternIndex >= state.patterns.size())
            continue;
        
        const MidiPattern& pattern = state.patterns[block.midiBlock.patternIndex];
        
        for (const MidiNote& note : pattern.notes)
        {
            const double noteStart = block.midiBlock.startBeat + note.startBeat - block.midiBlock.startOffsetBeats;
            const double noteEnd = noteStart + note.lengthBeats - s_config.blockEndEpsilon;
            const double effectiveNoteEnd = std::min(noteEnd, blockEnd);
            
            const bool noteCutByOffset = note.startBeat < block.midiBlock.startOffsetBeats;
            const bool noteActiveAtBlockStart = noteCutByOffset && 
                                                (note.startBeat + note.lengthBeats) > block.midiBlock.startOffsetBeats;
            
            if (s_data.needsNoteRetrigger && noteStart < prevBeat && effectiveNoteEnd > prevBeat)
            {
                int32_t sampleOffset = s_config.midiKeyOffset - note.keyNumber;
                const int32_t clampedOffset = std::max(0, std::min((int32_t)frameCount - 1, sampleOffset));
                track.instrument.plugin->PlayNote(note.keyNumber, note.velocity, clampedOffset);
            }
            else if (noteActiveAtBlockStart && block.midiBlock.startBeat >= prevBeat && block.midiBlock.startBeat < newBeat)
            {
                const double beatsFromBufferStart = block.midiBlock.startBeat - prevBeat;
                int32_t sampleOffset = (int32_t)(beatsFromBufferStart / beatsPerFrame);
                sampleOffset += s_config.midiKeyOffset - note.keyNumber;
                const int32_t clampedOffset = std::max(0, std::min((int32_t)frameCount - 1, sampleOffset));
                track.instrument.plugin->PlayNote(note.keyNumber, note.velocity, clampedOffset);
            }
            else if (!noteCutByOffset && noteStart >= prevBeat && noteStart < newBeat)
            {
                const double beatsFromBufferStart = noteStart - prevBeat;
                int32_t sampleOffset = (int32_t)(beatsFromBufferStart / beatsPerFrame);
                sampleOffset += s_config.midiKeyOffset - note.keyNumber;
                const int32_t clampedOffset = std::max(0, std::min((int32_t)frameCount - 1, sampleOffset));
                track.instrument.plugin->PlayNote(note.keyNumber, note.velocity, clampedOffset);
            }
            
            if (effectiveNoteEnd >= prevBeat && effectiveNoteEnd < newBeat)
            {
                const double beatsFromBufferStart = effectiveNoteEnd - prevBeat;
                int32_t sampleOffset = (int32_t)(beatsFromBufferStart / beatsPerFrame);
                sampleOffset += s_config.midiKeyOffset - note.keyNumber;
                const int32_t clampedOffset = std::max(0, std::min((int32_t)frameCount - 1, sampleOffset));
                track.instrument.plugin->StopNote(note.keyNumber, clampedOffset);
            }
        }
    }
    
    auto it = std::find_if(s_data.trackActiveBlockState.begin(), s_data.trackActiveBlockState.end(),
        [&track](const std::pair<AudioTrack*, bool>& p) { return p.first == &track; });
    
    if (it != s_data.trackActiveBlockState.end())
    {
        if (it->second && !trackHasActiveBlock)
        {
            for (int key = 0; key < 128; ++key)
            {
                track.instrument.plugin->StopNote(key, 0);
            }
        }
        it->second = trackHasActiveBlock;
    }
    else
    {
        s_data.trackActiveBlockState.push_back({&track, trackHasActiveBlock});
    }
}

void AudioEngine::ProcessSampleTrack(AudioTrack& track, double prevBeat, double newBeat,
                                     double beatsPerFrame, ma_uint32 frameCount,
                                     float* trackBuffer, float** inputs, float** outputs)
{
    ProjectState& state = ProjectState::GetInstance();
    memset(trackBuffer, 0, frameCount * s_config.channels * sizeof(float));
    
    float leftGain, rightGain;
    CalculatePanGains(track.pan, leftGain, rightGain);
    
    for (const TimelineBlock& block : track.blocks)
    {
        const double blockEnd = block.sampleBlock.startBeat + block.sampleBlock.lengthBeats - s_config.blockEndEpsilon;
        if (blockEnd <= prevBeat || block.sampleBlock.startBeat >= newBeat)
            continue;
        
        if (block.sampleBlock.sampleIndex >= state.samples.size())
            continue;
        
        const AudioSample& sample = state.samples[block.sampleBlock.sampleIndex];
        
        if (!sample.frameData || sample.frameCount == 0)
            continue;
        
        const float secondsPerBeat = SECONDS_PER_MINUTE / state.beatsPerMinute;
        
        for (ma_uint32 i = 0; i < frameCount; ++i)
        {
            const double currentBeat = prevBeat + i * beatsPerFrame;
            
            if (currentBeat < block.sampleBlock.startBeat || currentBeat >= blockEnd)
                continue;
            
            const double beatInBlock = currentBeat - block.sampleBlock.startBeat + block.sampleBlock.startOffsetBeats;
            const double secondsInBlock = beatInBlock * secondsPerBeat;
            const double sampleTime = secondsInBlock / block.sampleBlock.stretchScale;
            const double samplePos = sampleTime * sample.sampleRate;
            
            if (samplePos < 0 || samplePos >= sample.frameCount)
                continue;
            
            const uint64_t sampleIndex = (uint64_t)samplePos;
            const float fraction = samplePos - sampleIndex;
            
            if (sample.channelType == SampleChannelType::Stereo)
            {
                const uint64_t frameOffset = sampleIndex * 2;
                
                if (sampleIndex + 1 < sample.frameCount)
                {
                    float left, right;
                    
                    if (sampleIndex > 0 && sampleIndex + 2 < sample.frameCount)
                    {
                        const float* s = &sample.frameData[frameOffset];
                        left = CubicInterpolate(s[-2], s[0], s[2], s[4], fraction);
                        right = CubicInterpolate(s[-1], s[1], s[3], s[5], fraction);
                    }
                    else
                    {
                        left = sample.frameData[frameOffset] + 
                            (sample.frameData[frameOffset + 2] - sample.frameData[frameOffset]) * fraction;
                        right = sample.frameData[frameOffset + 1] + 
                                (sample.frameData[frameOffset + 3] - sample.frameData[frameOffset + 1]) * fraction;
                    }
                    
                    trackBuffer[i * 2] += left;
                    trackBuffer[i * 2 + 1] += right;
                }
            }
            else
            {
                float monoSample;
                
                if (sampleIndex + 1 < sample.frameCount)
                {
                    if (sampleIndex > 0 && sampleIndex + 2 < sample.frameCount)
                    {
                        monoSample = CubicInterpolate(
                            sample.frameData[sampleIndex - 1],
                            sample.frameData[sampleIndex],
                            sample.frameData[sampleIndex + 1],
                            sample.frameData[sampleIndex + 2],
                            fraction
                        );
                    }
                    else
                    {
                        monoSample = sample.frameData[sampleIndex] + 
                                    (sample.frameData[sampleIndex + 1] - sample.frameData[sampleIndex]) * fraction;
                    }
                }
                else
                {
                    monoSample = sample.frameData[sampleIndex];
                }
                
                trackBuffer[i * 2] += monoSample;
                trackBuffer[i * 2 + 1] += monoSample;
            }
        }
    }
    
    ProcessTrackEffects(track.effects, trackBuffer, frameCount, inputs, outputs);
    ApplyTrackVolumeAndPan(track, trackBuffer, nullptr, frameCount, leftGain, rightGain);
}

void AudioEngine::ProcessTrackEffects(std::vector<PluginEffect>& effects, float* trackBuffer,
                                     ma_uint32 frameCount, float** inputs, float** outputs)
{
    if (effects.empty())
        return;
    
    for (ma_uint32 i = 0; i < frameCount; ++i)
    {
        outputs[0][i] = trackBuffer[i * 2];
        outputs[1][i] = trackBuffer[i * 2 + 1];
    }
    
    for (PluginEffect& effect : effects)
    {
        if (effect.plugin)
        {
            memcpy(inputs[0], outputs[0], frameCount * sizeof(float));
            memcpy(inputs[1], outputs[1], frameCount * sizeof(float));
            
            effect.plugin->Process((float**)inputs, (float**)outputs, frameCount);
        }
    }
    
    for (ma_uint32 i = 0; i < frameCount; ++i)
    {
        trackBuffer[i * 2] = outputs[0][i];
        trackBuffer[i * 2 + 1] = outputs[1][i];
    }
}

void AudioEngine::ApplyTrackVolumeAndPan(AudioTrack& track, float* trackBuffer, float* output,
                                        ma_uint32 frameCount, float leftGain, float rightGain)
{
    for (ma_uint32 i = 0; i < frameCount; i++)
    {
        const float leftOut = trackBuffer[i * 2] * track.volume * leftGain;
        const float rightOut = trackBuffer[i * 2 + 1] * track.volume * rightGain;
        
        if (output)
        {
            output[i * 2] += leftOut;
            output[i * 2 + 1] += rightOut;
        }
        
        UpdatePeaks(track.peakLeft, track.peakRight, leftOut, rightOut);
    }
}

void AudioEngine::AudioCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
{
    ProjectState& state = ProjectState::GetInstance();
    float* outputBuffer = (float*)output;
    memset(output, 0, frameCount * s_config.channels * sizeof(float));
    
    float pluginInputs[MAX_PLUGIN_CHANNELS][512] = {0};
    float pluginOutputs[MAX_PLUGIN_CHANNELS][512] = {0};
    
    static float* inputs[MAX_PLUGIN_CHANNELS] = {nullptr};
    static float* outputs[MAX_PLUGIN_CHANNELS] = {nullptr};
    for (int i = 0; i < MAX_PLUGIN_CHANNELS; ++i)
    {
        inputs[i] = pluginInputs[i];
        outputs[i] = pluginOutputs[i];
    }
    
    float trackBuffer[512 * 2] = {0};
    
    for (AudioTrack& track : state.tracks)
    {
        track.peakLeft = 0.0f;
        track.peakRight = 0.0f;
    }
    state.masterTrack.peakLeft = 0.0f;
    state.masterTrack.peakRight = 0.0f;
    
    if (state.isPlaying && !state.isDraggingPlayhead)
    {
        const double prevBeat = state.timelinePositionBeats;
        const float secondsPerBeat = SECONDS_PER_MINUTE / state.beatsPerMinute;
        const double beatsPerFrame = 1.0 / (device->sampleRate * secondsPerBeat);
        const double newBeat = prevBeat + frameCount * beatsPerFrame;
        
        const bool playbackJumped = (s_data.lastProcessedBeat < 0 || 
                                    prevBeat < s_data.lastProcessedBeat || 
                                    prevBeat > s_data.lastProcessedBeat + beatsPerFrame * frameCount * 2);
        
        if (playbackJumped)
        {
            s_data.needsNoteRetrigger = true;
        }
        
        for (AudioTrack& track : state.tracks)
        {
            if (track.muted)
                continue;
            
            if (track.type == TrackType::Midi && track.instrument.plugin)
            {
                ProcessMidiTrack(track, prevBeat, newBeat, beatsPerFrame, frameCount, inputs, outputs);
            }
            else if (track.type == TrackType::Audio)
            {
                ProcessSampleTrack(track, prevBeat, newBeat, beatsPerFrame, frameCount, trackBuffer, inputs, outputs);
                
                float leftGain, rightGain;
                CalculatePanGains(track.pan, leftGain, rightGain);
                
                for (ma_uint32 i = 0; i < frameCount; i++)
                {
                    const float leftOut = trackBuffer[i * 2] * track.volume * leftGain;
                    const float rightOut = trackBuffer[i * 2 + 1] * track.volume * rightGain;
                    
                    outputBuffer[i * 2] += leftOut;
                    outputBuffer[i * 2 + 1] += rightOut;
                    
                    UpdatePeaks(track.peakLeft, track.peakRight, leftOut, rightOut);
                }
            }
        }
        
        s_data.needsNoteRetrigger = false;
        s_data.lastProcessedBeat = newBeat;
        state.timelinePositionBeats = newBeat;
    }
    else
    {
        s_data.lastProcessedBeat = -1.0;
        s_data.needsNoteRetrigger = true;
    }
    
    for (AudioTrack& track : state.tracks)
    {
        if (track.instrument.plugin)
        {
            memset(trackBuffer, 0, frameCount * s_config.channels * sizeof(float));
            
            track.instrument.plugin->Process((float**)inputs, (float**)outputs, frameCount);
            
            for (ma_uint32 i = 0; i < frameCount; i++)
            {
                trackBuffer[i * 2] = outputs[0][i];
                trackBuffer[i * 2 + 1] = outputs[1][i];
            }
            
            ProcessTrackEffects(track.effects, trackBuffer, frameCount, inputs, outputs);
            
            float leftGain, rightGain;
            CalculatePanGains(track.pan, leftGain, rightGain);
            
            for (ma_uint32 i = 0; i < frameCount; i++)
            {
                const float leftOut = trackBuffer[i * 2] * track.volume * leftGain;
                const float rightOut = trackBuffer[i * 2 + 1] * track.volume * rightGain;
                
                outputBuffer[i * 2] += leftOut;
                outputBuffer[i * 2 + 1] += rightOut;
                
                UpdatePeaks(track.peakLeft, track.peakRight, leftOut, rightOut);
            }
        }
    }
    
    if (!state.masterTrack.effects.empty())
    {
        for (ma_uint32 i = 0; i < frameCount; ++i)
        {
            outputs[0][i] = outputBuffer[i * 2];
            outputs[1][i] = outputBuffer[i * 2 + 1];
        }
        
        for (PluginEffect& effect : state.masterTrack.effects)
        {
            if (effect.plugin)
            {
                memcpy(inputs[0], outputs[0], frameCount * sizeof(float));
                memcpy(inputs[1], outputs[1], frameCount * sizeof(float));
                
                effect.plugin->Process((float**)inputs, (float**)outputs, frameCount);
            }
        }
        
        for (ma_uint32 i = 0; i < frameCount; ++i)
        {
            outputBuffer[i * 2] = outputs[0][i];
            outputBuffer[i * 2 + 1] = outputs[1][i];
        }
    }
    
    if (state.masterTrack.muted)
    {
        memset(outputBuffer, 0, frameCount * s_config.channels * sizeof(float));
        return;
    }

    float leftGain, rightGain;
    CalculatePanGains(state.masterTrack.pan, leftGain, rightGain);
    const float masterVolume = state.masterTrack.volume;
    leftGain *= masterVolume / s_config.masterVolumeNormalization;
    rightGain *= masterVolume / s_config.masterVolumeNormalization;
    
    for (ma_uint32 i = 0; i < frameCount; i++)
    {
        const float leftOut = outputBuffer[i * 2] * leftGain;
        const float rightOut = outputBuffer[i * 2 + 1] * rightGain;
        
        outputBuffer[i * 2] = leftOut;
        outputBuffer[i * 2 + 1] = rightOut;
        
        UpdatePeaks(state.masterTrack.peakLeft, state.masterTrack.peakRight, leftOut, rightOut);
    }
}

bool AudioEngine::ExportToWav(const char* filepath, double startBeat, double endBeat)
{
    if (endBeat <= startBeat)
    {
        std::cerr << "Invalid beat range for export\n";
        return false;
    }
    
    ProjectState& state = ProjectState::GetInstance();
    const float secondsPerBeat = SECONDS_PER_MINUTE / state.beatsPerMinute;
    const double durationBeats = endBeat - startBeat;
    const double durationSeconds = durationBeats * secondsPerBeat;
    
    const ma_uint32 sampleRate = s_config.sampleRate;
    const ma_uint32 channels = s_config.channels;
    const ma_uint64 totalFrames = (ma_uint64)(durationSeconds * sampleRate);
    
    std::cout << "Exporting " << durationBeats << " beats (" 
              << durationSeconds << " seconds, " 
              << totalFrames << " frames) to " << filepath << "\n";
    
    ma_encoder_config encoderConfig = ma_encoder_config_init(
        ma_encoding_format_wav,
        ma_format_f32,
        channels,
        sampleRate
    );
    
    ma_encoder encoder;
    if (ma_encoder_init_file(filepath, &encoderConfig, &encoder) != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize WAV encoder\n";
        return false;
    }
    
    const double originalPosition = state.timelinePositionBeats;
    const bool wasPlaying = state.isPlaying;
    const bool wasDragging = state.isDraggingPlayhead;
    
    state.timelinePositionBeats = startBeat;
    state.isPlaying = true;
    state.isDraggingPlayhead = false;
    
    s_data.lastProcessedBeat = -1.0;
    s_data.needsNoteRetrigger = true;
    s_data.trackActiveBlockState.clear();
    
    const ma_uint32 chunkSize = s_config.bufferSize;
    float* buffer = new float[chunkSize * channels];
    
    ma_uint64 framesProcessed = 0;
    
    ma_device fakeDevice = s_data.device;
    fakeDevice.sampleRate = sampleRate;
    
    ma_device_stop(&s_data.device);
    
    while (framesProcessed < totalFrames)
    {
        const ma_uint32 framesToProcess = std::min(chunkSize, (ma_uint32)(totalFrames - framesProcessed));
        
        memset(buffer, 0, framesToProcess * channels * sizeof(float));
        
        AudioCallback(&fakeDevice, buffer, nullptr, framesToProcess);
        
        ma_uint64 framesWritten;
        if (ma_encoder_write_pcm_frames(&encoder, buffer, framesToProcess, &framesWritten) != MA_SUCCESS)
        {
            std::cerr << "Failed to write frames to WAV file\n";
            delete[] buffer;
            ma_encoder_uninit(&encoder);
            
            state.timelinePositionBeats = originalPosition;
            state.isPlaying = wasPlaying;
            state.isDraggingPlayhead = wasDragging;
            s_data.lastProcessedBeat = -1.0;
            s_data.needsNoteRetrigger = true;
            
            return false;
        }
        
        framesProcessed += framesWritten;
        
        if (framesProcessed % (sampleRate * 2) == 0)
        {
            const float progress = (float)framesProcessed / totalFrames * 100.0f;
            std::cout << "Export progress: " << (int)progress << "%\n";
        }
    }
    
    delete[] buffer;
    StopAllNotes();
    ma_encoder_uninit(&encoder);
    
    state.timelinePositionBeats = originalPosition;
    state.isPlaying = wasPlaying;
    state.isDraggingPlayhead = wasDragging;
    s_data.lastProcessedBeat = -1.0;
    s_data.needsNoteRetrigger = true;
    s_data.trackActiveBlockState.clear();
    ma_device_start(&s_data.device);
    
    std::cout << "Export complete: " << filepath << "\n";
    return true;
}

