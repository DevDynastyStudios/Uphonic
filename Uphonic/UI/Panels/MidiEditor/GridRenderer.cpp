#include "GridRenderer.h"
#include "Models/DataModel/Music.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

static bool SemitoneIsBlack(int s)
{
 	return s == 1 || s == 3 || s == 6 || s == 8 || s == 10;
}

void GridRenderer::Render(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float noteHeight, float scrollX, float scrollY, float pianoWidth, int snap, const TimeSignature& timeSig, const MidiEditorConfig& config, int lowestNote, int highestNote)
{
 	RenderRowBackgrounds(draw, canvasPos, canvasSize, noteHeight, scrollY, pianoWidth, config, lowestNote, highestNote);
 	RenderRuler(draw, canvasPos, canvasSize, beatWidth, scrollX, pianoWidth, snap, timeSig, config);
 	RenderBeatLines(draw, canvasPos, canvasSize, beatWidth, scrollX, pianoWidth, snap, timeSig, config);
}

void GridRenderer::RenderRowBackgrounds(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float noteHeight, float scrollY, float pianoWidth, const MidiEditorConfig& config, int lowestNote, int highestNote)
{
 	const float gridStartX = canvasPos.x + pianoWidth;
 	const float gridEndX = canvasPos.x + canvasSize.x;
 	const float bodyTopY = canvasPos.y + config.rulerHeight;
 	const float bodyHeight = canvasSize.y - config.rulerHeight;

 	const int pianoTopRow = (config.totalKeys - 1) - highestNote;
 	const int pianoBottomRow = (config.totalKeys - 1) - lowestNote;

 	const int firstRow = std::max<int>(pianoTopRow, std::max<int>(0, (int)scrollY));
 	const int lastRow = std::min<int>(pianoBottomRow + 1, std::min<int>(config.totalKeys, (int)(scrollY + bodyHeight / noteHeight) + 1));

 	for (int row = firstRow; row < lastRow; ++row)
 	{
 	 	const int midiNote = (config.totalKeys - 1) - row;
 	 	const bool isBlack = SemitoneIsBlack(midiNote % 12);

 	 	const float y = bodyTopY + ((float)row - scrollY) * noteHeight;
 	 	const ImU32 fill = isBlack ? IM_COL32(24, 24, 28, 255) : IM_COL32(30, 30, 36, 255);
 	 	const ImU32 line = isBlack ? IM_COL32(30, 30, 34, 255) : IM_COL32(36, 36, 42, 255);

 	 	draw->AddRectFilled(ImVec2(gridStartX, y), ImVec2(gridEndX, y + noteHeight), fill);
 	 	draw->AddLine(ImVec2(gridStartX, y), ImVec2(gridEndX, y), line);
 	}
}

void GridRenderer::RenderRuler(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float scrollX, float pianoWidth, int snap, const TimeSignature& timeSig, const MidiEditorConfig& config)
{
 	const float gridStartX = canvasPos.x + pianoWidth;
 	const float gridEndX = canvasPos.x + canvasSize.x;
 	const float rulerTop = canvasPos.y;
 	const float rulerBot = canvasPos.y + config.rulerHeight;

 	draw->AddRectFilled(ImVec2(gridStartX, rulerTop), ImVec2(gridEndX, rulerBot), IM_COL32(20, 20, 24, 255));
 	draw->AddLine(ImVec2(gridStartX, rulerBot), ImVec2(gridEndX, rulerBot), IM_COL32(52, 52, 62, 255), 1.0f);

 	const float beatsPerMeasure = timeSig.BeatsPerMeasure();
 	const float measureWidth = beatsPerMeasure * beatWidth;
 	const int startMeasure = std::max<int>(0, (int)(scrollX / measureWidth));
 	const int endMeasure = (int)((scrollX + (gridEndX - gridStartX)) / measureWidth) + 2;

 	for (int m = startMeasure; m <= endMeasure; ++m)
 	{
 	 	const float x = gridStartX + (float)m * measureWidth - scrollX;
 	 	if (x < gridStartX || x >= gridEndX)
			continue;

 	 	draw->AddLine(ImVec2(x, rulerBot - 5.0f), ImVec2(x, rulerBot), IM_COL32(78, 78, 95, 255), 1.0f);

 	 	if (measureWidth >= 20.0f)
 	 	{
 	 	 	char buf[16];
 	 	 	snprintf(buf, sizeof(buf), "%d", m + 1);
 	 	 	draw->AddText(ImVec2(x + 3.0f, rulerTop + 3.0f), IM_COL32(130, 130, 150, 220), buf);
 	 	}

 	 	const float beatStep = beatWidth * 4.0f / (float)timeSig.denominator;
 	 	if (beatStep >= 6.0f)
 	 	{
 	 	 	for (int b = 1; b < timeSig.numerator; ++b)
 	 	 	{
 	 	 	 	const float bx = x + (float)b * beatStep;
 	 	 	 	if (bx >= gridEndX)
					break;

 	 	 	 	draw->AddLine(ImVec2(bx, rulerBot - 3.0f), ImVec2(bx, rulerBot), IM_COL32(50, 50, 65, 255), 1.0f);
 	 	 	}
 	 	}
 	}
}

void GridRenderer::RenderBeatLines(ImDrawList* draw, ImVec2 canvasPos, ImVec2 canvasSize, float beatWidth, float scrollX, float pianoWidth, int snap, const TimeSignature& timeSig, const MidiEditorConfig& config)
{
 	static constexpr float kMinSubPx = 4.0f;

 	const float gridStartX = canvasPos.x + pianoWidth;
 	const float gridEndX = canvasPos.x + canvasSize.x;
 	const float lineTop = canvasPos.y + config.rulerHeight;
 	const float lineBot = canvasPos.y + canvasSize.y;

 	const int subsPerBeat = snap / 4;
 	const float subWidth = beatWidth / (float)subsPerBeat;
 	const bool drawSubs = (subWidth >= kMinSubPx) && (subsPerBeat > 1);
 	const int beatsPerMeasure = (int)std::round(timeSig.BeatsPerMeasure());

 	const int startBeat = std::max(0, (int)(scrollX / beatWidth));
 	const int endBeat = (int)((scrollX + (gridEndX - gridStartX)) / beatWidth) + 2;

 	for (int beat = startBeat; beat <= endBeat; ++beat)
 	{
 	 	const float x = gridStartX + (float)beat * beatWidth - scrollX;
 	 	if (x < gridStartX || x > gridEndX)
			continue;

 	 	if (beat % beatsPerMeasure == 0)
 	 	 	draw->AddLine(ImVec2(x, lineTop), ImVec2(x, lineBot), IM_COL32(65, 65, 80, 255), 1.5f);
 	 	else
 	 	 	draw->AddLine(ImVec2(x, lineTop), ImVec2(x, lineBot), IM_COL32(42, 42, 54, 255), 1.0f);
 	}

 	if (!drawSubs)
		return;

 	const int startSub = std::max(0, (int)(scrollX / subWidth));
 	const int endSub = (int)((scrollX + (gridEndX - gridStartX)) / subWidth) + 2;

 	for (int sub = startSub; sub <= endSub; ++sub)
 	{
 	 	if (sub % subsPerBeat == 0)
			continue;

 	 	const float x = gridStartX + (float)sub * subWidth - scrollX;
 	 	if (x < gridStartX || x > gridEndX)
			continue;

 	 	draw->AddLine(ImVec2(x, lineTop), ImVec2(x, lineBot), IM_COL32(34, 34, 44, 255), 0.5f);
 	}
}