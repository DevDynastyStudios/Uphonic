#include "panel_manager.h"

struct UphSettings
{
    
};

static UphSettings settings_data {};

static void uph_settings_init(UphPanel* panel)
{
	
	panel->window_flags = ImGuiWindowFlags_NoSavedSettings;
}

static void uph_settings_render(UphPanel* panel)
{
    ImGui::TextWrapped("An unsaved project was found. Would you like to restore or discard it?");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Restore", ImVec2(120, 0))) 
	{
        panel->is_visible = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Discard", ImVec2(120, 0))) 
	{
        panel->is_visible = false;
    }

}

UPH_REGISTER_PANEL("Settings", UphPanelFlags_Popup, uph_settings_render, uph_settings_init);