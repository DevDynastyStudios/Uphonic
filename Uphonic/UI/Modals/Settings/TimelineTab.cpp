#include "TimelineTab.h"
#include "Models/EditorModel/ApplicationSettings.h"
#include <imgui.h>

void TimelineTab::Draw(ApplicationSettings& draft)
{
	TimelineSettings& t = draft.timelineSettings;

	ImGui::SeparatorText("Layout");
	{
		ImGui::DragFloat("Default Track Height", &t.defaultTrackHeight, 1.0f, 20.0f, 500.0f, "%.1f");
		ImGui::DragFloat("Track Header Width", &t.trackHeaderWidth, 1.0f, 80.0f, 400.0f, "%.1f");
		ImGui::DragFloat("Default Beat Width", &t.defaultBeatWidth, 1.0f, 10.0f, 200.0f, "%.1f");
	}

	ImGui::SeparatorText("Blocks");
	{
		ImGui::DragFloat("Default Instance Length (beats)", &t.defaultInstanceLength, 0.25f, 0.25f, 64.0f, "%.2f");
	}
}