#include "panel_manager.h"

struct UphSettings
{
    
};

static UphSettings settings_data {};

static void uph_settings_render(UphPanel* panel)
{

}

UPH_REGISTER_PANEL("Settings", ImGuiWindowFlags_NoSavedSettings, uph_settings_render, false);