#include "panel_manager.h"

struct UphSettings
{
    
};

static UphSettings settings_data {};

static void uph_settings_render(UphPanel* panel)
{

}

UPH_REGISTER_PANEL("Settings", &settings_data, ImGuiWindowFlags_None, uph_settings_render);