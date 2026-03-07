#pragma once
#include <imgui.h>
#include <filesystem>

enum class SampleChannelType
{
	Mono,
	Stereo
};

struct AudioSample
{
	std::string name;
	std::string filename;	// Points to the audio sample in workspace
	SampleChannelType channelType;
	ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f};	// Possibly optimize to not use ImGui's Vec4
	float* frameData;
	uint64_t frameCount;
	uint32_t originalSampleRate;

	AudioSample() : channelType(SampleChannelType::Mono), frameData(nullptr), frameCount(0), originalSampleRate(44100) {}
	bool IsValid() const { return frameCount > 0; }
};