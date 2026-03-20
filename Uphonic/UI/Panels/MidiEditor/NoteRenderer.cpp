#include "NoteRenderer.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

static const char* kNoteNames[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
static constexpr float kUnsetAlphaThreshold = 0.01f;

void NoteRenderer::Render(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight, float scrollX, float scrollY, float pianoWidth, const MidiPattern& pattern, const std::vector<int>& selectedIndices, const MidiEditorConfig& config)
{
 	const float gridStartX = canvasPos.x + pianoWidth;
 	const float gridEndX = canvasPos.x + canvasSize.x;
 	const float bodyTopY = canvasPos.y + config.rulerHeight;
	const bool hasPatternColor = (pattern.color.w > kUnsetAlphaThreshold);
	ImU32 noteCol, noteSelCol, borderCol;

	if(hasPatternColor)
	{
 		const Color4& pc = pattern.color;
 		noteCol = ImGui::ColorConvertFloat4ToU32({pc.x, pc.y, pc.z, pc.w});
 		noteSelCol = ImGui::ColorConvertFloat4ToU32(ImVec4(std::min<float>(1.0f, pc.x * 1.4f), std::min<float>(1.0f, pc.y * 1.4f), std::min<float>(1.0f, pc.z * 1.4f), pc.w));
 		borderCol = ImGui::ColorConvertFloat4ToU32(ImVec4(std::min<float>(1.0f, pc.x * 1.7f), std::min<float>(1.0f, pc.y * 1.7f), std::min<float>(1.0f, pc.z * 1.7f), pc.w));
	}
	else
	{
		noteCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Button));
 	 	noteSelCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
 	 	borderCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	}

 	const ImU32 textCol = IM_COL32(230, 230, 240, 220);
 	for (int i = 0; i < (int)pattern.notes.size(); ++i)
 	{
 	 	const MidiNote& note = pattern.notes[i];
 	 	const bool selected = std::find(selectedIndices.begin(), selectedIndices.end(), i) != selectedIndices.end();

 	 	const float noteX = gridStartX + (float)note.startBeat * beatWidth - scrollX;
 	 	const float noteW = (float)note.lengthBeats * beatWidth;

 	 	const float noteY = bodyTopY + ((float)((config.totalKeys - 1) - note.keyNumber) - scrollY) * noteHeight;

 	 	if (noteX + noteW < gridStartX || noteX > gridEndX)
			continue;

 	 	if (noteY + noteHeight < bodyTopY || noteY > canvasPos.y + canvasSize.y)
			continue;

 	 	const float drawX = std::max<float>(noteX, gridStartX);
 	 	const float drawW = std::min<float>(noteX + noteW, gridEndX) - drawX;
 	 	if (drawW <= 0.0f)
			continue;

 	 	const ImVec2 p1(drawX, noteY + 1.0f);
 	 	const ImVec2 p2(drawX + drawW, noteY + noteHeight - 1.0f);
 	 	draw->AddRectFilled(p1, p2, selected ? noteSelCol : noteCol, 2.0f);
 	 	draw->AddRect(p1, p2, borderCol, 2.0f, 0, selected ? 2.0f : 1.2f);

 	 	if (noteW > 25.0f && noteHeight >= 12.0f)
 	 	{
 	 	 	char buf[8];
 	 	 	snprintf(buf, sizeof(buf), "%s%d", kNoteNames[note.keyNumber % 12], (note.keyNumber / 12) - 1);

 	 	 	const ImVec2 sz = ImGui::CalcTextSize(buf);
 	 	 	const float tx = std::max<float>(drawX + 4.0f, noteX + 4.0f);
 	 	 	const float ty = noteY + (noteHeight - sz.y) * 0.5f;

 	 	 	if (tx + sz.x < drawX + drawW)
 	 	 	 	draw->AddText(ImVec2(tx, ty), textCol, buf);
 	 	}

 	 	if (selected && noteX + noteW >= gridStartX && noteHeight >= 8.0f)
 	 	{
 	 	 	const float hx = std::max(noteX + noteW - 4.0f, gridStartX);
 	 	 	draw->AddRectFilled(ImVec2(hx, noteY + 3.0f), ImVec2(noteX + noteW - 1, noteY + noteHeight - 3.0f), borderCol);
 	 	}
 	}
}