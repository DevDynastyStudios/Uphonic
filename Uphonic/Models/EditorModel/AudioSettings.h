#pragma once
#include <string>
#include <cstdint>

struct AudioSettings
{
	std::string outputDevice;
	std::string inputDevice;

	uint32_t sampleRate = 44100;
	uint32_t bufferSize = 512;
	uint32_t channels = 2;
	bool exclusiveMode = false;

	bool softwareMonitoring = false;
	float monitoringGain = 1.0f;

	bool autoRestartOnDeviceChange = true;
	bool underrunProtection = true;

	float vuMeterHoldTime = 0.5f;
	float vuMeterDecayRate = 1.5f;
};