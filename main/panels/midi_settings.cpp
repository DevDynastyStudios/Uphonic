#include "panel_manager.h"

static void uph_midi_settings_init(UphPanel* panel)
{
	panel->window_flags = ImGuiWindowFlags_NoSavedSettings;
}

UPH_REGISTER_PANEL("Midi Settings", UphPanelFlags::Panel, nullptr, uph_midi_settings_init);