#pragma once
#include <string>
#include <cstdint>

struct GeneralSettings
{
	std::string theme;
	std::string languageCode = "en";
	std::string regionCode = "US";
	float uiScale = 1.0f;
	float fontSize = 16.0f;

	bool openLastProjectOnStartup = true;
	std::string lastProjectPath;

	bool autosaveEnabled = true;
	uint32_t undoHistoryLimit = 256;
	
	bool confirmOnExit = true;
	bool confirmOnDelete = true;
	bool showTooltips = true;

	std::string lastOpenDirectory;
	std::string lastSaveDirectory;

	// UI Settings
	float horizontalScrollSensitivity = 20.0f;
	float verticalScrollSensitivity = 2.0f;
	float horizontalZoomSensitivity = 0.1f;
	float verticalZoomSensitivity = 0.1f;

	bool followPlayhead = false;
	float playheadLockPosition = 0.0f;

	int defaultSnap = 16;
	float defaultHorizontalZoom = 1.0f;
	float defaultVerticalZoom = 1.0f;
	float defaultBeatWidth = 50.0f;
};