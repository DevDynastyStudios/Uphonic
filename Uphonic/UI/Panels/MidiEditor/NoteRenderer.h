#pragma once
#include "Naui.h"
#include "Models/DataModel/Patterns.h"
#include "Config/EditorConfig.h"

#include <imgui.h>
#include <vector>

class NoteRenderer
{
public:
    void Render(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight, float scrollX, float scrollY, float pianoWidth, const MidiPattern& pattern, const std::vector<int>& selectedIndices, const MidiEditorConfig& config);
};