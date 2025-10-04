#include "panel_manager.h"
#include "../types.h"
#include <imgui.h>
#include <vector>
#include <cstdio> // snprintf

struct UphPatternRack 
{
	int renaming_index = -1;
};

static UphPatternRack pattern_data { };

// Forward-declared global application (from your data model)
extern UphApplication* app;

static void uph_sample_rack_render(UphPanel* panel)
{
    // Create a new empty pattern
    if (ImGui::Button("+ New Sample"))
    {
        UphSample pattern{};
        snprintf(pattern.name, sizeof(pattern.name), "Sample %zu", app->project.samples.size() + 1);
        app->project.samples.push_back(pattern);
        // Optionally start renaming immediately:
        // pattern_data.renaming_index = (int)app->project.samples.size() - 1;
    }

    ImGui::Separator();

    // Consistent fixed block height
    const ImVec2 block_size = ImVec2(-1.0f, 28.0f); // full width, fixed height
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    for (size_t i = 0; i < app->project.samples.size(); ++i)
    {
        UphSample& pat = app->project.samples[i];
        ImGui::PushID((int)i);

        const bool is_renaming = (pattern_data.renaming_index == (int)i);

        if (!is_renaming)
        {
            // Render as a “physical” block button
            if (ImGui::Button(pat.name, block_size))
            {
                // Click: open in MIDI editor or select
            }

            // --- Rename triggers ---
            bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_None);

            // Double‑click to rename
            if (hovered && ImGui::IsMouseDoubleClicked(0))
                pattern_data.renaming_index = (int)i;

            // F2 works if item is focused OR just hovered
            if ((ImGui::IsItemFocused() || hovered) && ImGui::IsKeyPressed(ImGuiKey_F2, false))
                pattern_data.renaming_index = (int)i;

            // Drag source (for timeline placement or rack reorder)
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("SAMPLE", &i, sizeof(size_t));
                ImGui::Text("Dragging %s", pat.name);
                ImGui::EndDragDropSource();
            }

            // Drag target (reorder within rack)
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAMPLE"))
                {
                    size_t src_index = *(const size_t*)payload->Data;
                    if (src_index != i)
                    {
                        UphSample moved = app->project.samples[src_index];
                        app->project.samples.erase(app->project.samples.begin() + src_index);
                        app->project.samples.insert(app->project.samples.begin() + i, moved);
                        // Adjust rename index if needed
                        if (pattern_data.renaming_index == (int)src_index) pattern_data.renaming_index = (int)i;
                        else if (pattern_data.renaming_index > (int)src_index && pattern_data.renaming_index <= (int)i) pattern_data.renaming_index--;
                        else if (pattern_data.renaming_index < (int)src_index && pattern_data.renaming_index >= (int)i) pattern_data.renaming_index++;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Context menu: Rename / Duplicate / Delete
            if (ImGui::BeginPopupContextItem("SampleContext"))
            {
                if (ImGui::MenuItem("Rename", "F2"))
                    pattern_data.renaming_index = (int)i;

                if (ImGui::MenuItem("Duplicate"))
                {
                    UphSample copy = pat;											// <---------------------- BOX THIS IS YOUR SHIT!!!!
                    // Append " Copy" but avoid overflow
                    const char* suffix = " Copy";
                    size_t len = strlen(copy.name);
                    size_t suf_len = strlen(suffix);
                    if (len + suf_len < sizeof(copy.name))
                        memcpy(copy.name + len, suffix, suf_len + 1);
                    app->project.samples.insert(app->project.samples.begin() + i + 1, copy);
                }

                if (ImGui::MenuItem("Delete"))
                {
                    app->project.samples.erase(app->project.samples.begin() + i);
                    if (pattern_data.renaming_index == (int)i) pattern_data.renaming_index = -1;
                    else if (pattern_data.renaming_index > (int)i) pattern_data.renaming_index--;
                    ImGui::EndPopup();
                    ImGui::PopID();
                    break; // vector changed; restart loop next frame
                }

                ImGui::EndPopup();
            }
        }
        else
        {
            // Inline rename field (same footprint as the button)
            ImGui::PushItemWidth(block_size.x);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));

            bool commit = ImGui::InputText("##rename", pat.name, sizeof(pat.name),
                                           ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_AutoSelectAll);

            ImGui::PopStyleVar();
            ImGui::PopItemWidth();
            ImGui::SetKeyboardFocusHere(-1); // Focus input on start

            // --- Exit conditions ---
            // 1. Enter commits
            if (commit)
                pattern_data.renaming_index = -1;

            // 2. Escape cancels
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                pattern_data.renaming_index = -1;

            // 3. Click outside cancels
            if (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
                pattern_data.renaming_index = -1;
        }

        ImGui::PopID();
    }

    ImGui::PopStyleVar(2);
}

UPH_REGISTER_PANEL("Sample Rack", UphPanelFlags::Panel, uph_sample_rack_render, nullptr);