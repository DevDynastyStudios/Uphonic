#include "PanelRenderer.h"
#include "Panel.h"
#include <imgui.h>

namespace Naui {

void PanelRenderer::Render()
{
    auto& panels = GetAllPanels();
    for (const auto& [id, panel_ptr] : panels)
    {
        Panel& panel = *panel_ptr;

        if (!panel.m_open)
            continue;

        ImGui::SetNextWindowSizeConstraints(panel.m_minSize, panel.m_maxSize);
        ImGui::Begin(panel.GetTitle().c_str(), panel.m_closable ? &panel.m_open : nullptr, panel.m_imguiFlags);
        panel.OnRender();
        ImGui::End();
    }
}

}