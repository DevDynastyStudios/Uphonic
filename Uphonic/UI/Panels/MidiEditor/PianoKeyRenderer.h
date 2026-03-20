#pragma once
#include "Naui.h"
#include "Config/EditorConfig.h"
#include <set>

class PianoKeyRenderer
{
public:
	struct Layout
	{
		float keyLength = 96.0f;
		float blackKeyLenRatio = 0.60f;
		float blackKeyHgtRatio = 0.62f;

		float whiteKeyBorderAlpha = 120.0f; // 0-255: separator between white keys
		float whiteKeyBorderThick = 0.8f;	// line thickness in pixels
		float blackKeyBorderAlpha = 255.0f; // outline around each black key
		float blackKeyBorderThick = 1.0f;
		float octaveBorderAlpha	= 220.0f;	// stronger line at every C
		float octaveBorderThick	= 1.2f;

		float cSharp = 1.0f;
		float dSharp = 1.0f;
		float fSharp = 1.0f;
		float gSharp = 1.0f;
		float aSharp = 1.0f;

		int lowestNote = 21;	// A0
		int highestNote = 108;	// C8
	};

	Layout layout;
	bool showDebugPanel = false;

	void RenderDebugPanel();

	void PressKey (int midiNote);
	void ReleaseKey (int midiNote);
	void ReleaseAllKeys();
	bool IsKeyPressed (int midiNote) const;

	void Render(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config);
	int HandleInput(ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config);

private:
	static bool IsBlackKey (int semitone);
	static int WhiteIndex (int semitone); // 0=C … 6=B, -1 for black keys
	void BlackKeyPlacement (int semitone, int& lowerWhiteIdx, float& ratio) const;

	static void VisibleMidiRange(float noteHeight, float scrollY, ImVec2 canvasSize, int totalKeys, int& highMidi, int& lowMidi);

	void RenderWhiteKeys(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config);
	void RenderBlackKeys(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config);
	void RenderLabels (ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config);

	int HitTest(ImVec2 mouse, ImVec2 canvasPos, float noteHeight, float scrollY, const MidiEditorConfig& config) const;

	std::set<int> m_pressedKeys;
	int m_lastHeldNote = -1;
};