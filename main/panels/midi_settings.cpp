#include "panel_manager.h"

static void uph_midi_settings_init(UphPanel* panel)
{
	panel->panel_flags |= UphPanelFlags_HiddenFromMenu;
	panel->window_flags = ImGuiWindowFlags_NoSavedSettings;
}

UPH_REGISTER_PANEL("Midi Settings", UphPanelFlags_Panel, nullptr, uph_midi_settings_init);