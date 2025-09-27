#include "panel_manager.h"

struct UphMidiEditor
{
    
};

static UphMidiEditor editor_data {};

static void uph_midi_editor_render(UphPanel* panel)
{
    
}

UPH_REGISTER_PANEL("Midi Editor", ImGuiWindowFlags_None, uph_midi_editor_render);