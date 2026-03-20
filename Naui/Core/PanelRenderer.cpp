#include "PanelRenderer.h"
#include "Panel.h"
#include "Shortcut/Shortcut.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <unordered_set>
#include <vector>

namespace Naui {

bool PanelImGuiImpl::IsFocused() const
{
	ImGuiWindow* win = ImGui::FindWindowByName(GetTitle().c_str());
	if (!win)
		return false;
 
	ImGuiWindow* nav = ImGui::GetCurrentContext()->NavWindow;
	for (ImGuiWindow* w = nav; w; w = w->ParentWindow)
	{
		if (w == win)
			return true;
	}
 
	return false;
}

// Tracks which panels were focused last frame to detect enter/leave transitions
static std::unordered_set<uint64_t> s_focusedLastFrame;

void PanelRenderer::Render()
{
	auto& panels = GetAllPanels();
	for (auto& [uid, panel_ptr] : panels)
	{
		Panel& panel = *panel_ptr;
		if (!panel.m_shortcutsRegistered)
		{
			panel.OnRegisterShortcuts(panel.GetShortcuts());
			panel.m_shortcutsRegistered = true;
		}
	}

	{
		std::vector<ShortcutTable*> tables;
		for (auto& [uid, panel_ptr] : panels)
		{
			Panel& panel = *panel_ptr;
			if (!panel.m_open)
				continue;

			if (panel.IsFocused())
				panel.GetShortcuts().SetAllHardWeights(ShortcutTable::FOCUS_WEIGHT);
			else
				panel.GetShortcuts().ClearAllHardWeights();

			tables.push_back(&panel.GetShortcuts());
		}

		Shortcut::Poll(std::span<ShortcutTable*>(tables));
	}

	{
		std::unordered_set<uint64_t> focusedThisFrame;

		for (auto& [uid, panel_ptr] : panels)
		{
			if (panel_ptr->m_open && panel_ptr->IsFocused())
				focusedThisFrame.insert(uid);
		}

		for (auto& [uid, panel_ptr] : panels)
		{
			const bool wasFocused = s_focusedLastFrame.count(uid) > 0;
			const bool nowFocused = focusedThisFrame.count(uid) > 0;

			if (!wasFocused && nowFocused) panel_ptr->OnFocus();
			if (wasFocused && !nowFocused) panel_ptr->OnUnfocus();
		}

		s_focusedLastFrame = std::move(focusedThisFrame);
	}

	for (auto& [uid, panel_ptr] : panels)
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