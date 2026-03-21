#include "Modal.h"
#include <imgui.h>

namespace Naui {

static std::unordered_map<uint64_t, Modal*> s_modals;
static std::unordered_map<std::string, ModalFactory> s_modalFactories;
static Vec4 s_overlayColor = Vec4(0.0f, 0.0f, 0.0f, 0.6f);

std::unordered_map<uint64_t, Modal*>& GetAllModals()
{
	return s_modals;
}

std::unordered_map<std::string, ModalFactory>& GetModalFactories()
{
	return s_modalFactories;
}

void FocusModalWindow(const char* title)
{
	ImGui::SetWindowFocus(title);
}

void RegisterModalFactory(const std::string& typeName, ModalFactory factory)
{
	s_modalFactories[typeName] = std::move(factory);
}

void DestroyModal(uint64_t uid)
{
	auto it = s_modals.find(uid);
	if (it == s_modals.end())
		return;

	it->second->OnClose();
	delete it->second;
	s_modals.erase(it);
}

void DestroyAllModals()
{
	for (auto it = s_modals.begin(); it != s_modals.end();)
	{
		it->second->OnClose();
		delete it->second;
		it = s_modals.erase(it);
	}
}

void SetModalOverlayColor(const Vec4& color)
{
	s_overlayColor = color;
}

Vec4 GetModalOverlayColor()
{
	return s_overlayColor;
}

void SetModalOverlayAlpha(float alpha)
{
	s_overlayColor.w = alpha;
}

}