#include "ModalRenderer.h"
#include "Modal.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace Naui {

static inline ImVec2 ToImGui(const Vec2& v) { return ImVec2(v.x, v.y); }
static inline ImVec4 ToImGui(const Vec4& v) { return ImVec4(v.x, v.y, v.z, v.w); }

static ModalFocusPolicy ResolveStrongestPolicy()
{
	ModalFocusPolicy strongest = ModalFocusPolicy::Free;
	for (auto& [uid, modal] : GetAllModals())
	{
		if (!modal->IsOpen())
			continue;

		if (modal->GetFocusPolicy() > strongest)
			strongest = modal->GetFocusPolicy();

		if (strongest == ModalFocusPolicy::HardBlock)
			break;
	}

	return strongest;
}

static void ApplyModalPosition(const Modal* modal)
{
	const ModalPosition pos = modal->GetPosition();
	if (pos == ModalPosition::Default)
		return;

	const ImGuiViewport* vp = ImGui::GetMainViewport();
	const ImVec2 vpPos = vp->Pos;
	const ImVec2 vpSize = vp->Size;

	ImVec2 winSize = ToImGui(modal->m_minSize);
	if (const ImGuiWindow* win = ImGui::FindWindowByName(modal->GetTitle().c_str()))
		winSize = win->Size;

	ImVec2 target;
	switch (pos)
	{
		case ModalPosition::Center:
			target = ImVec2(
				vpPos.x + (vpSize.x - winSize.x) * 0.5f,
				vpPos.y + (vpSize.y - winSize.y) * 0.5f);
			break;

		case ModalPosition::TopCenter:
			target = ImVec2(
				vpPos.x + (vpSize.x - winSize.x) * 0.5f,
				vpPos.y + 60.0f);
			break;

		case ModalPosition::Custom:
			target = ToImGui(modal->GetCustomPosition());
			break;

		default:
			return;
	}

	ImGui::SetNextWindowPos(target, ImGuiCond_Appearing);
}

void ModalRenderer::Render()
{
	auto& modals = GetAllModals();
	if (modals.empty())
		return;

	ImGuiViewport* vp = ImGui::GetMainViewport();
	ImVec2 vpPos = vp->Pos;
	ImVec2 vpEnd = ImVec2(vpPos.x + vp->Size.x, vpPos.y + vp->Size.y);

	const ModalFocusPolicy policy = ResolveStrongestPolicy();
	const bool overlayBlocks = (policy >= ModalFocusPolicy::SoftBlock);
	const ImVec4 overlayColor  = ToImGui(GetModalOverlayColor());

	ImGuiWindowFlags overlayFlags =
		ImGuiWindowFlags_NoTitleBar			|
		ImGuiWindowFlags_NoResize			|
		ImGuiWindowFlags_NoScrollbar		|
		ImGuiWindowFlags_NoScrollWithMouse	|
		ImGuiWindowFlags_NoCollapse			|
		ImGuiWindowFlags_NoSavedSettings	|
		ImGuiWindowFlags_NoNav				|
		ImGuiWindowFlags_NoMove				|
		ImGuiWindowFlags_NoFocusOnAppearing;

	if (!overlayBlocks)
		overlayFlags |= ImGuiWindowFlags_NoMouseInputs;

	ImGui::SetNextWindowPos(vpPos);
	ImGui::SetNextWindowSize(vp->Size);
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("##modal_overlay", nullptr, overlayFlags);
	ImGui::PopStyleVar(2);

	if (overlayColor.w > 0.0f)
		ImGui::GetWindowDrawList()->AddRectFilled(vpPos, vpEnd, ImGui::ColorConvertFloat4ToU32(overlayColor));

	if (overlayBlocks)
	{
		ImGui::SetCursorScreenPos(vpPos);
		if (ImGui::InvisibleButton("##overlay_hit", vp->Size))
		{
			for (auto& [uid, modal] : modals)
				if (modal->ClosesOnOverlayClick())
					modal->SetOpen(false);
		}
	}

	ImGui::End();
	for (auto it = modals.begin(); it != modals.end();)
	{
		Modal* modal = it->second;

		if (!modal->IsOpen())
		{
			modal->OnClose();
			delete modal;
			it = modals.erase(it);
			continue;
		}

		ApplyModalPosition(modal);
		ImGui::SetNextWindowSizeConstraints(ToImGui(modal->m_minSize), ToImGui(modal->m_maxSize));

		ImGuiWindowFlags flags = modal->GetWindowFlags();
		flags |= ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;

		bool open = true;
		bool* openPtr = modal->IsClosable() ? &open : nullptr;

		ImGui::Begin(modal->GetTitle().c_str(), openPtr, flags);
		modal->OnRender();
		ImGui::End();

		if (!open)
		{
			modal->OnClose();
			delete modal;
			it = modals.erase(it);
		}
		else
			++it;
	}

	for (auto& [uid, modal] : modals)
	{
		ImGuiWindow* win = ImGui::FindWindowByName(modal->GetTitle().c_str());
		if (win)
			ImGui::BringWindowToDisplayFront(win);
	}

	ImGuiContext* ctx = ImGui::GetCurrentContext();
	for (ImGuiWindow* win : ctx->Windows)
	{
		if (!(win->Flags & ImGuiWindowFlags_Popup) || !win->Active)
			continue;

		for (ImGuiWindow* parent = win->ParentWindow; parent; parent = parent->ParentWindow)
		{
			bool isOurModal = false;
			for (auto& [uid, modal] : modals)
			{
				if (ImGui::FindWindowByName(modal->GetTitle().c_str()) == parent)
				{
					isOurModal = true;
					break;
				}
			}

			if (isOurModal)
			{
				ImGui::BringWindowToDisplayFront(win);
				break;
			}
		}
	}
}

}