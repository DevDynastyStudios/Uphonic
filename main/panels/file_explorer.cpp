#include "panel_manager.h"

struct UphFileExplorer
{
    
};

static UphFileExplorer explorer_data {};

static void uph_file_explorer_render(UphPanel* panel)
{
    ImGui::Text("File Explorer");
}

UPH_REGISTER_PANEL("File Explorer", ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, uph_file_explorer_render);