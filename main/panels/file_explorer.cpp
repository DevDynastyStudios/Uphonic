#include "panel_manager.h"

struct UphFileExplorer
{
    
};

static UphFileExplorer explorer_data {};

static void uph_file_explorer_render(UphPanel* panel)
{
    ImGui::Text("File Explorer");
}

UPH_REGISTER_PANEL("File Explorer", &explorer_data, ImGuiWindowFlags_None, uph_file_explorer_render);