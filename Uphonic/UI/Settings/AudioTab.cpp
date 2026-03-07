#include "AudioTab.h"
#include "Models/EditorModel/ApplicationSettings.h"
#include <imgui.h>

void AudioTab::Draw(ApplicationSettings& draft)
{
	AudioSettings& a = draft.audioSettings;
	ImGui::SeparatorText("Device");
	{
		static const uint32_t sampleRates[] = { 44100, 48000, 88200, 96000, 176400, 192000 };
		static const char* sampleRateLabels[] = { "44100 Hz", "48000 Hz", "88200 Hz", "96000 Hz", "176400 Hz", "192000 Hz" };

		int currentRate = 0;
		for (int i = 0; i < IM_ARRAYSIZE(sampleRates); ++i)
			if (sampleRates[i] == a.sampleRate) { currentRate = i; break; }

		if (ImGui::Combo("Sample Rate", &currentRate, sampleRateLabels, IM_ARRAYSIZE(sampleRateLabels)))
			a.sampleRate = sampleRates[currentRate];

		static const uint32_t bufferSizes[] = { 64, 128, 256, 512, 1024, 2048 };
		static const char* bufferLabels[] = { "64", "128", "256", "512", "1024", "2048" };

		int currentBuffer = 0;
		for (int i = 0; i < IM_ARRAYSIZE(bufferSizes); ++i)
			if (bufferSizes[i] == a.bufferSize) { currentBuffer = i; break; }

		if (ImGui::Combo("Buffer Size", &currentBuffer, bufferLabels, IM_ARRAYSIZE(bufferLabels)))
			a.bufferSize = bufferSizes[currentBuffer];

		static const char* channelOptions[] = { "Mono", "Stereo" };
		int currentChannels = (int)a.channels - 1;
		if (ImGui::Combo("Channels", &currentChannels, channelOptions, IM_ARRAYSIZE(channelOptions)))
			a.channels = (uint32_t)(currentChannels + 1);

		ImGui::Checkbox("Exclusive Mode", &a.exclusiveMode);
	}

	ImGui::SeparatorText("Monitoring");
	{
		ImGui::Checkbox("Software Monitoring", &a.softwareMonitoring);
		ImGui::DragFloat("Monitoring Gain", &a.monitoringGain, 0.01f, 0.0f, 2.0f, "%.2f");
	}

	ImGui::SeparatorText("Stability");
	{
		ImGui::Checkbox("Auto-restart on Device Change", &a.autoRestartOnDeviceChange);
		ImGui::Checkbox("Underrun Protection", &a.underrunProtection);
	}

	ImGui::SeparatorText("VU Meter");
	{
		ImGui::DragFloat("Hold Time (s)", &a.vuMeterHoldTime, 0.01f, 0.0f, 5.0f, "%.2f");
		ImGui::DragFloat("Decay Rate", &a.vuMeterDecayRate, 0.01f, 0.1f, 10.0f, "%.2f");
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Audio device restart required for sample rate / buffer changes.");
}