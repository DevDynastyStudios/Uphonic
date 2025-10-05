#include "panel_manager.h"
#include "../settings/layout_manager.h"
#include <deque>
#include <chrono>

struct UphTempoTapper
{
	std::deque<double> tap_times;
	float current_bpm = 0.0f;
};

static UphTempoTapper tapper_data {};

static void uph_tempo_tapper_init(UphPanel* panel)
{
	panel->category = UPH_CATEGORY_EDITOR;
}

static void uph_tempo_tapper_render(UphPanel* panel)
{
    ImGui::Text("Tap tempo to set BPM");

    // Make a square button
    float size = 80.0f;
    if (ImGui::Button("Tap", ImVec2(size, size)))
    {
        using clock = std::chrono::steady_clock;
        static auto last_tap = clock::now();
        auto now = clock::now();

        double seconds = std::chrono::duration<double>(now - last_tap).count();
        last_tap = now;

        // Ignore very long gaps (reset)
        if (seconds > 2.0)
            tapper_data.tap_times.clear();
        else
            tapper_data.tap_times.push_back(seconds);

        // Keep only the last N intervals
        const size_t max_samples = 8;
        if (tapper_data.tap_times.size() > max_samples)
            tapper_data.tap_times.pop_front();

        // Compute BPM from average interval
        if (!tapper_data.tap_times.empty())
        {
            double avg = 0.0;
            for (double s : tapper_data.tap_times) avg += s;
            avg /= tapper_data.tap_times.size();
            tapper_data.current_bpm = float(60.0 / avg);
        }
    }

    if (tapper_data.current_bpm > 0.0f)
        ImGui::Text("BPM: %.1f", tapper_data.current_bpm);
    else
        ImGui::Text("BPM: --");
}

UPH_REGISTER_PANEL("Tempo Tapper", UphPanelFlags_Panel, uph_tempo_tapper_render, uph_tempo_tapper_init);