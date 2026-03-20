#pragma once
#include "Naui.h"
#include "Config/EditorConfig.h"
#include "GridRenderer.h"
#include <functional>

struct TimeSignature;

class ToolbarRenderer
{
public:
 	void Render(int* currentTool, float* zoom, int* snap, bool hasSelection, TimeSignature* timeSig, const MidiEditorConfig& config, const std::function<void()>& onDeleteSelected, const std::function<void()>& onSelectAll, const std::function<void()>& onToolChanged);

private:
 	static const char* SnapName(int snap);
};