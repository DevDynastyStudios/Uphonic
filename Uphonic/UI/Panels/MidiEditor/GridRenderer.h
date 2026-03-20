#pragma once
#include "Naui.h"
#include "Config/EditorConfig.h"

struct TimeSignature;

class GridRenderer
{
public:
 	void Render(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight, float scrollX, float scrollY, float pianoWidth, int snap, const TimeSignature& timeSig, const MidiEditorConfig& config, int lowestNote, int highestNote);

private:
 	void RenderRowBackgrounds(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, float pianoWidth, const MidiEditorConfig& config, int lowestNote, int highestNote);
 	void RenderRuler(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float scrollX, float pianoWidth, int snap, const TimeSignature& timeSig, const MidiEditorConfig& config);
 	void RenderBeatLines(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float scrollX, float pianoWidth, int snap, const TimeSignature& timeSig, const MidiEditorConfig& config);
};