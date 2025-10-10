#include "panel_manager.h"
#include "../types.h"

static void uph_rhythm_settings_init(UphPanel* panel)
{

}

static void uph_rhythm_settings_render(UphPanel* panel)
{
    // --- BPM control ---
    ImGui::Text("BPM:");
    ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::InputFloat("##bpm", &app->project.bpm, 0.1f, 1.0f, "%.1f");
	app->project.bpm = std::max<float>(1.0f, app->project.bpm);

    ImGui::SameLine(0, 20); // spacing before next group

    // --- Time signature dropdown ---
    static const char* time_sig_items[] = {
        "2/4", "3/4", "4/4", "5/4", "6/8", "7/8", "9/8", "12/8"
    };

	ImGui::SetNextItemWidth(50);
    static int current_sig = 2; // default to "4/4"
    if (ImGui::BeginCombo("", time_sig_items[current_sig])) {
        for (int n = 0; n < IM_ARRAYSIZE(time_sig_items); n++) {
            bool is_selected = (current_sig == n);
            if (ImGui::Selectable(time_sig_items[n], is_selected)) {
                current_sig = n;

                // Parse numerator/denominator from string
                int num = 0, den = 0;
                sscanf_s(time_sig_items[n], "%d/%d", &num, &den);
                app->project.time_sig_numerator   = num;
                app->project.time_sig_denominator = den;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine(0, 20);

    // --- Grid resolution ---
    ImGui::Text("Grid:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70);
    ImGui::InputInt("##steps", &app->project.steps_per_beat);
    app->project.steps_per_beat = std::max<int>(1, app->project.steps_per_beat);
}

UPH_REGISTER_PANEL("Rhythm Settings", UphPanelFlags_Panel, uph_rhythm_settings_render, uph_rhythm_settings_init);