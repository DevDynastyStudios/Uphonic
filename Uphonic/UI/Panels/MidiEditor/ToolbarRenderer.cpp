#include "ToolbarRenderer.h"
#include "Core/ProjectState.h"

struct TimeSigPreset { int num; int den; const char* label; };
static const TimeSigPreset kPresets[] =
{
 	{ 2, 4, "2/4" },
 	{ 3, 4, "3/4" },
 	{ 4, 4, "4/4" },
 	{ 5, 4, "5/4" },
 	{ 6, 4, "6/4" },
 	{ 7, 4, "7/4" },
 	{ 3, 8, "3/8" },
 	{ 5, 8, "5/8" },
 	{ 6, 8, "6/8" },
 	{ 7, 8, "7/8" },
 	{ 9, 8, "9/8" },
 	{ 12, 8, "12/8" },
	{ 15, 16, "15/16" },
};
static const int kPresetCount = (int)(sizeof(kPresets) / sizeof(kPresets[0]));

const char* ToolbarRenderer::SnapName(int snap)
{
 	switch (snap)
 	{
 	 	case 4: return "1/4";
 	 	case 8: return "1/8";
 	 	case 16: return "1/16";
 	 	case 32: return "1/32";
		case 64: return "1/64";
 	 	default: return "?";
 	}
}

void ToolbarRenderer::Render(int* currentTool, float* zoom, int* snap, bool hasSelection, TimeSignature* timeSig, const MidiEditorConfig& config, const std::function<void()>& onDeleteSelected, const std::function<void()>& onSelectAll, const std::function<void()>& onToolChanged)
{
 	if (ImGui::RadioButton("Select", *currentTool == 0))
 	{
 	 	*currentTool = 0;
 	 	if (onToolChanged)
			onToolChanged();
 	}

 	ImGui::SameLine();
 	if (ImGui::RadioButton("Draw", *currentTool == 1))
 	{
 	 	*currentTool = 1;
 	 	if (onToolChanged)
			onToolChanged();
 	}

 	ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();

 	const char* preview = nullptr;
 	for (int i = 0; i < kPresetCount; ++i)
 	{
 	 	if (kPresets[i].num == timeSig->numerator && kPresets[i].den == timeSig->denominator)
 	 	{
 	 	 	preview = kPresets[i].label;
 	 	 	break;
 	 	}
 	}

 	char customBuf[16];
 	if (!preview)
 	{
 	 	snprintf(customBuf, sizeof(customBuf), "%d/%d", timeSig->numerator, timeSig->denominator);
 	 	preview = customBuf;
 	}

 	ImGui::Text("Time:");
 	ImGui::SameLine();
 	ImGui::SetNextItemWidth(60.0f);
 	if (ImGui::BeginCombo("##TimeSig", preview, ImGuiComboFlags_NoArrowButton))
 	{
 	 	for (int i = 0; i < kPresetCount; ++i)
 	 	{
 	 	 	const bool sel = (timeSig->numerator == kPresets[i].num && timeSig->denominator == kPresets[i].den);
 	 	 	if (ImGui::Selectable(kPresets[i].label, sel))
 	 	 	{
 	 	 	 	timeSig->numerator = kPresets[i].num;
 	 	 	 	timeSig->denominator = kPresets[i].den;
 	 	 	}

 	 	 	if (sel)
				ImGui::SetItemDefaultFocus();
 	 	}
		
 	 	ImGui::EndCombo();
 	}

 	ImGui::SameLine();
	ImGui::Text("|");
	ImGui::SameLine();

 	ImGui::Text("Snap:");
 	ImGui::SameLine();
 	ImGui::SetNextItemWidth(70.0f);
 	if (ImGui::BeginCombo("##Snap", SnapName(*snap), ImGuiComboFlags_NoArrowButton))
 	{
 	 	if (ImGui::Selectable("1/4", *snap == 4)) *snap = 4;
 	 	if (ImGui::Selectable("1/8", *snap == 8)) *snap = 8;
 	 	if (ImGui::Selectable("1/16", *snap == 16)) *snap = 16;
 	 	if (ImGui::Selectable("1/32", *snap == 32)) *snap = 32;
		if (ImGui::Selectable("1/64", *snap == 64)) *snap = 64;
 	 	ImGui::EndCombo();
 	}

 	ImGui::SameLine();
	ImGui::Text("|");
	ImGui::SameLine();

 	ImGui::Text("Zoom:");
 	ImGui::SameLine();
 	ImGui::SetNextItemWidth(130.0f);
 	ImGui::SliderFloat("##Zoom", zoom, config.minZoom, config.maxZoom, "%.2fx");

 	ImGui::SameLine();
	ImGui::Text("|");
	ImGui::SameLine();

 	if (hasSelection && *currentTool == 0)
 	{
 	 	if (ImGui::Button("Delete Selected") && onDeleteSelected)
 	 	 	onDeleteSelected();

 	 	ImGui::SameLine();
 	}

 	if (ImGui::Button("Select All") && onSelectAll)
 	 	onSelectAll();
}