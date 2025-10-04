#include "panel_manager.h"

struct UphSettings
{
    
};

static UphSettings settings_data {};

static void uph_settings_init(UphPanel* panel)
{
	panel->panel_flags |= UphPanelFlags_HiddenFromMenu;
	panel->window_flags = ImGuiWindowFlags_NoSavedSettings;
}

static void uph_settings_render(UphPanel* panel)
{

}

UPH_REGISTER_PANEL("Settings", UphPanelFlags_Popup, uph_settings_render, uph_settings_init);