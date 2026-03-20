#include "PianoKeyRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

bool PianoKeyRenderer::IsBlackKey(int s)
{
	return s == 1 || s == 3 || s == 6 || s == 8 || s == 10;
}

int PianoKeyRenderer::WhiteIndex(int s)
{
							// C  C# D  D# E  F  F# G  G# A  A# B
	static const int t[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
	return t[s];
}

void PianoKeyRenderer::BlackKeyPlacement(int semitone, int& lw, float& ratio) const
{
	switch (semitone)
	{
		case 1: lw = 0; ratio = layout.cSharp; break;	// C# : C(0)→D(1)
		case 3: lw = 1; ratio = layout.dSharp; break;	// D# : D(1)→E(2)
		case 6: lw = 3; ratio = layout.fSharp; break;	// F# : F(3)→G(4)
		case 8: lw = 4; ratio = layout.gSharp; break;	// G# : G(4)→A(5)
		case 10: lw = 5; ratio = layout.aSharp; break;	// A# : A(5)→B(6)
		default: lw = 0; ratio = 0.5f; break;
	}
}

void PianoKeyRenderer::VisibleMidiRange(float noteHeight, float scrollY, ImVec2 canvasSize, int totalKeys, int& highMidi, int& lowMidi)
{
	const int firstRow = std::max<int>(0, (int)scrollY - 2);
	const int lastRow = std::min<int>(totalKeys - 1, (int)(scrollY + canvasSize.y / noteHeight) + 2);
	highMidi = totalKeys - 1 - firstRow;
	lowMidi = std::max<int>(0, totalKeys - 1 - lastRow);
}

void PianoKeyRenderer::PressKey(int n)
{
	m_pressedKeys.insert(n);
}

void PianoKeyRenderer::ReleaseKey(int n)
{
	m_pressedKeys.erase(n);
}

void PianoKeyRenderer::ReleaseAllKeys()
{
	m_pressedKeys.clear();
}

bool PianoKeyRenderer::IsKeyPressed(int n) const 
{
	return m_pressedKeys.count(n) != 0;
}

void PianoKeyRenderer::Render(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config)
{
	const ImVec2 pianoTopLeft(canvasPos.x, canvasPos.y + config.rulerHeight);
	const ImVec2 pianoMax(canvasPos.x + layout.keyLength, canvasPos.y + canvasSize.y);
	draw->PushClipRect(pianoTopLeft, pianoMax, true);
	draw->AddRectFilled(pianoTopLeft, pianoMax, IM_COL32(18, 18, 20, 255));

	RenderWhiteKeys(draw, canvasPos, canvasSize, noteHeight, scrollY, config);
	RenderBlackKeys(draw, canvasPos, canvasSize, noteHeight, scrollY, config);
	RenderLabels(draw, canvasPos, canvasSize, noteHeight, scrollY, config);

	draw->AddLine(ImVec2(canvasPos.x + layout.keyLength, canvasPos.y + config.rulerHeight), ImVec2(canvasPos.x + layout.keyLength, canvasPos.y + canvasSize.y), IM_COL32(45, 45, 54, 255), 2.0f);
	draw->PopClipRect();
}

int PianoKeyRenderer::HandleInput(ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config)
{
	const ImVec2 mouse = ImGui::GetMousePos();
	const float pianoEndX = canvasPos.x + layout.keyLength;
	const float bodyTopY = canvasPos.y + config.rulerHeight;

	const bool inPiano = mouse.x >= canvasPos.x && mouse.x < pianoEndX && mouse.y >= bodyTopY && mouse.y < canvasPos.y + canvasSize.y;

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && m_lastHeldNote != -1)
	{
		ReleaseKey(m_lastHeldNote);
		m_lastHeldNote = -1;
	}

	if (!inPiano || !ImGui::IsWindowHovered())
		return -1;

	if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		const int hit = HitTest(mouse, canvasPos, noteHeight, scrollY, config);
		if (hit != -1 && hit != m_lastHeldNote)
		{
			if (m_lastHeldNote != -1) ReleaseKey(m_lastHeldNote);
			PressKey(hit);
			m_lastHeldNote = hit;
			return hit;
		}
	}
	return -1;
}

int PianoKeyRenderer::HitTest(ImVec2 mouse, ImVec2 canvasPos, float noteHeight, float scrollY, const MidiEditorConfig& config) const
{
	const float bodyTopY = canvasPos.y + config.rulerHeight;
	const float blackEndX = canvasPos.x + layout.keyLength * layout.blackKeyLenRatio;
	const float relY = mouse.y - bodyTopY;
	if (relY < 0.0f)
		return -1;

	const int row = (int)((relY + scrollY * noteHeight) / noteHeight);
	const int midiNote = config.totalKeys - 1 - row;

	if (midiNote < layout.lowestNote || midiNote > layout.highestNote)
		return -1;

	if (midiNote < 0 || midiNote > 127)
		return -1;

	if (mouse.x > blackEndX && IsBlackKey(midiNote % 12))
	{
		const float whiteH = 12.0f * noteHeight / 7.0f;
		const int octBase = (midiNote / 12) * 12;

		for (int oOff = -12; oOff <= 12; oOff += 12)
		{
			const int oct = octBase + oOff;
			if (oct < 0)
				continue;

			const float cBotY = bodyTopY + ((float)(config.totalKeys - oct) - scrollY) * noteHeight;
			for (int s = 0; s < 12; ++s)
			{
				if (IsBlackKey(s))
					continue;

				const int candidate = oct + s;
				if (candidate < layout.lowestNote || candidate > layout.highestNote)
					continue;

				const int wi = WhiteIndex(s);
				const float yBottom = cBotY - (float)wi * whiteH;
				const float yTop = yBottom - whiteH;
				if (mouse.y >= yTop && mouse.y < yBottom)
					return candidate;
			}
		}

		return -1;
	}

	return midiNote;
}

void PianoKeyRenderer::RenderWhiteKeys(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config)
{
	const float whiteH = 12.0f * noteHeight / 7.0f;
	const float keyW = layout.keyLength;
	const float bodyTopY = canvasPos.y + config.rulerHeight;
	const float bodyH = canvasSize.y - config.rulerHeight;

	int highMidi, lowMidi;
	VisibleMidiRange(noteHeight, scrollY, {canvasSize.x, bodyH}, config.totalKeys, highMidi, lowMidi);
	const int startOctave = (std::max(lowMidi, layout.lowestNote) / 12) * 12;

	for (int octaveBase = startOctave; octaveBase <= highMidi + 11; octaveBase += 12)
	{
		const float cBottomY =
			bodyTopY + ((float)(config.totalKeys - octaveBase) - scrollY) * noteHeight;

		for (int semitone = 0; semitone < 12; ++semitone)
		{
			if (IsBlackKey(semitone))
				continue;

			const int midiNote = octaveBase + semitone;
			if (midiNote < layout.lowestNote || midiNote > layout.highestNote)
				continue;

			const int w = WhiteIndex(semitone);
			const float yBottom = cBottomY - (float)w * whiteH;
			const float yTop = yBottom - whiteH;

			if (yTop >= bodyTopY + bodyH)
				continue;

			if (yBottom <= bodyTopY)
				continue;

			const bool pressed = IsKeyPressed(midiNote);
			const ImU32 fill = pressed ? IM_COL32(145, 175, 255, 255) : IM_COL32(218, 218, 226, 255);
			draw->AddRectFilled(ImVec2(canvasPos.x, yTop), ImVec2(canvasPos.x + keyW, yBottom), fill);

			const bool isCNote = (semitone == 0);
			draw->AddLine(
				ImVec2(canvasPos.x, yBottom),
				ImVec2(canvasPos.x + keyW, yBottom),
				IM_COL32(75, 75, 92, isCNote ? (int)layout.octaveBorderAlpha : (int)layout.whiteKeyBorderAlpha),
				isCNote ? layout.octaveBorderThick : layout.whiteKeyBorderThick
			);
		}
	}
}

void PianoKeyRenderer::RenderBlackKeys(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config)
{
	const float whiteH = 12.0f * noteHeight / 7.0f;
	const float blackH = whiteH * layout.blackKeyHgtRatio;
	const float blackW = layout.keyLength * layout.blackKeyLenRatio;
	const float bodyTopY = canvasPos.y + config.rulerHeight;
	const float bodyH = canvasSize.y - config.rulerHeight;

	int highMidi, lowMidi;
	VisibleMidiRange(noteHeight, scrollY, {canvasSize.x, bodyH}, config.totalKeys, highMidi, lowMidi);
	const int startOctave = (std::max(lowMidi, layout.lowestNote) / 12) * 12;
	static const int kBlack[] = { 1, 3, 6, 8, 10 };

	for (int octaveBase = startOctave; octaveBase <= highMidi + 11; octaveBase += 12)
	{
		const float cBottomY = bodyTopY + ((float)(config.totalKeys - octaveBase) - scrollY) * noteHeight;

		for (int semitone : kBlack)
		{
			const int midiNote = octaveBase + semitone;
			if (midiNote < layout.lowestNote || midiNote > layout.highestNote)
				continue;

			int lw; float ratio;
			BlackKeyPlacement(semitone, lw, ratio);
			const float yCenter = cBottomY - ((float)lw + ratio) * whiteH;
			const float yTop = yCenter - blackH * 0.5f;
			const float yBottom = yCenter + blackH * 0.5f;

			if (yTop >= bodyTopY + bodyH)
				continue;

			if (yBottom <= bodyTopY)
				continue;

			const bool pressed = IsKeyPressed(midiNote);
			const ImU32 fill = pressed ? IM_COL32(80, 110, 220, 255) : IM_COL32(28, 28, 34, 255);
			const ImU32 border = IM_COL32(10, 10, 14, (int)layout.blackKeyBorderAlpha);

			draw->AddRectFilled(ImVec2(canvasPos.x, yTop), ImVec2(canvasPos.x + blackW, yBottom), fill, 2.0f);
			draw->AddRect(ImVec2(canvasPos.x, yTop), ImVec2(canvasPos.x + blackW, yBottom), border, 2.0f, 0, layout.blackKeyBorderThick);

			if (!pressed && (yBottom - yTop) > 7.0f)
				draw->AddRectFilled(ImVec2(canvasPos.x + 1.5f, yTop + 1.5f), ImVec2(canvasPos.x + 3.5f, yBottom - 1.5f), IM_COL32(50, 50, 60, 255), 1.0f);
		}
	}
}

void PianoKeyRenderer::RenderLabels(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, const MidiEditorConfig& config)
{
	const float whiteH = 12.0f * noteHeight / 7.0f;
	const float labelX = canvasPos.x + layout.keyLength * layout.blackKeyLenRatio + 4.0f;
	const float bodyTopY = canvasPos.y + config.rulerHeight;
	const float bodyH = canvasSize.y - config.rulerHeight;

	if (whiteH < ImGui::GetTextLineHeight() + 3.0f)
		return;

	int highMidi, lowMidi;
	VisibleMidiRange(noteHeight, scrollY, {canvasSize.x, bodyH}, config.totalKeys, highMidi, lowMidi);
	const int startOctave = (std::max(lowMidi, layout.lowestNote) / 12) * 12;

	for (int octaveBase = startOctave; octaveBase <= highMidi + 11; octaveBase += 12)
	{
		const int midiNote = octaveBase;
		if (midiNote < layout.lowestNote || midiNote > layout.highestNote)
			continue;

		const float cBottomY = bodyTopY + ((float)(config.totalKeys - octaveBase) - scrollY) * noteHeight;
		const float cTopY = cBottomY - whiteH;
		const float labelY = cTopY + (whiteH - ImGui::GetTextLineHeight()) * 0.5f;

		if (labelY < bodyTopY || labelY >= bodyTopY + bodyH)
			continue;

		char buf[8];
		snprintf(buf, sizeof(buf), "C%d", (octaveBase / 12) - 1);
		draw->AddText(ImVec2(labelX, labelY), IM_COL32(105, 105, 120, 215), buf);
	}
}

void PianoKeyRenderer::RenderDebugPanel()
{
	if (!showDebugPanel)
		return;

	ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Once);
	ImGui::SetNextWindowPos (ImVec2(20, 20), ImGuiCond_Once);
	if (!ImGui::Begin("Piano Key Layout Tuner", &showDebugPanel, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::End();
		return;
	}

	ImGui::TextDisabled("Ctrl+click a slider to type an exact value.");
	ImGui::Spacing();

	auto Row = [](const char* label, float* v, float lo, float hi, float def)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 1));
		if (ImGui::SmallButton("R"))
			*v = def;

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Reset to %.4f", def);

		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);
		ImGui::SliderFloat(label, v, lo, hi, "%.4f");
		ImGui::PopID();
	};

	if (ImGui::CollapsingHeader("Dimensions", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Row("Key length (px)",	&layout.keyLength,		40.0f, 200.0f, 96.0f);
		Row("Black key length",  &layout.blackKeyLenRatio, 0.30f, 0.90f, 0.60f);
		Row("Black key height",  &layout.blackKeyHgtRatio, 0.30f, 0.95f, 0.62f);
	}

	if (ImGui::CollapsingHeader("2-key group ( C# · D# )", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Row("C# (C - D)", &layout.cSharp, 0.0f, 1.0f, 1.0f);
		Row("D# (D - E)", &layout.dSharp, 0.0f, 1.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("3-key group ( F# · G# · A# )", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Row("F# (F - G)", &layout.fSharp, 0.0f, 1.0f, 1.0f);
		Row("G# (G - A)", &layout.gSharp, 0.0f, 1.0f, 1.0f);
		Row("A# (A - B)", &layout.aSharp, 0.0f, 1.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("MIDI Range"))
	{
		ImGui::TextDisabled("Only skips out-of-range notes — no effect on Y positions.");
		ImGui::SetNextItemWidth(200.0f);
		ImGui::SliderInt("Lowest (A0=21)", &layout.lowestNote,  0, 127);
		ImGui::SetNextItemWidth(200.0f);
		ImGui::SliderInt("Highest (C8=108)", &layout.highestNote, 0, 127);
	}

	ImGui::Separator();
	if (ImGui::CollapsingHeader("Current values", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static char kBuf[512];
		snprintf(kBuf, sizeof(kBuf),
			"layout.keyLength		= %.4ff;\n"
			"layout.blackKeyLenRatio = %.4ff;\n"
			"layout.blackKeyHgtRatio = %.4ff;\n"
			"layout.cSharp			= %.4ff;\n"
			"layout.dSharp			= %.4ff;\n"
			"layout.fSharp			= %.4ff;\n"
			"layout.gSharp			= %.4ff;\n"
			"layout.aSharp			= %.4ff;\n"
			"layout.lowestNote		= %d;\n"
			"layout.highestNote		= %d;\n",
			layout.keyLength, layout.blackKeyLenRatio, layout.blackKeyHgtRatio,
			layout.cSharp, layout.dSharp, layout.fSharp, layout.gSharp, layout.aSharp,
			layout.lowestNote, layout.highestNote);

		ImGui::InputTextMultiline("##vals", kBuf, sizeof(kBuf), ImVec2(-1.0f, 185.0f), ImGuiInputTextFlags_ReadOnly);
		ImGui::TextDisabled("Click inside, Ctrl+A, Ctrl+C to copy.");
	}

	ImGui::End();
}