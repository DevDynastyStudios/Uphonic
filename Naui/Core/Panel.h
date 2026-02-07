#pragma warning(push)
#pragma warning(disable : 4251)

#pragma once

#include "Base.h"

#include <imgui.h>

#include <cstdint>
#include <string>
#include <cfloat>
#include <unordered_map>
#include <typeinfo>

namespace Naui
{

class NAUI_API PanelImGuiImpl
{
public:
	void SetResizable(bool value) { value ? m_imguiFlags &= ~ImGuiWindowFlags_NoResize : m_imguiFlags |= ImGuiWindowFlags_NoResize; }
	void SetMovable(bool value) { value ? m_imguiFlags &= ~ImGuiWindowFlags_NoMove : m_imguiFlags |= ImGuiWindowFlags_NoMove; }
	void SetMinimizable(bool value) { value ? m_imguiFlags &= ~ImGuiWindowFlags_NoCollapse : m_imguiFlags |= ImGuiWindowFlags_NoCollapse; }
	void SetAutoResize(bool value) { value ? m_imguiFlags |= ImGuiWindowFlags_AlwaysAutoResize : m_imguiFlags &= ~ImGuiWindowFlags_AlwaysAutoResize; }
	void SetNoCollapse(bool value) { value ? m_imguiFlags |= ImGuiWindowFlags_NoCollapse : m_imguiFlags &= ~ImGuiWindowFlags_NoCollapse; }
	void SetDockable(bool value) { value ? m_imguiFlags &= ~ImGuiWindowFlags_NoDocking : m_imguiFlags |= ImGuiWindowFlags_NoDocking; }

	void SetViewportPos(const ImVec2& pos, ImGuiCond cond = ImGuiCond_Always)
	{
		ImGuiViewport* view = ImGui::GetMainViewport();
		ImVec2 screenPos = view->Pos;
		ImVec2 screenSize = view->Size;
		ImGui::SetWindowPos(ImVec2(screenPos.x + pos.x * screenSize.x, screenPos.y + pos.y * screenSize.y), cond);
	}

	void SetMaxSize(float x, float y) { m_maxSize = ImVec2(x, y); }
	void SetMinSize(float x, float y) { m_minSize = ImVec2(x, y); }
	void SetTitle(const char *title) { m_title = title; }

	std::string GetTitle() { return m_title; }

protected:
	ImGuiWindowFlags m_imguiFlags = 0;
	ImVec2 m_minSize = ImVec2(0, 0);
	ImVec2 m_maxSize = ImVec2(FLT_MAX, FLT_MAX);
	std::string m_title;
};

class NAUI_API Panel : public PanelImGuiImpl
{
public:
	Panel(void) = default;
	Panel(const char *title) { m_title = title; }
	const uint64_t GetUID(void) const { return (uint64_t)this; }

	void SetOpen(bool value) { m_open = value; m_calledClose = false; }
	bool IsOpen(void) const { return m_open; }
	const std::string &GetLayoutID(void) const { return m_layoutID; }

protected:
	virtual void OnRender(void) { };
	virtual void OnClose(void) { };

	void SetClosable(bool value) { m_closable = value; }
	void SetCategory(const std::string &category);
	void SetLayoutID(const std::string &id) { m_layoutID = id; }
private:

	bool m_closable = true;
	bool m_open = true;
	bool m_calledClose = false;
	std::string m_layoutID;

	friend class PanelRenderer;
};

NAUI_API std::unordered_map<uint64_t, Panel*> &GetAllPanels(void);

template<typename T>
Panel &AddPanel(void)
{
	Panel *p = new T;
	GetAllPanels()[p->GetUID()] = p;
	return *p;
}

NAUI_API void DestroyPanel(uint64_t uid);
NAUI_API void DestroyAllPanels(void);

template<typename T>
T* GetPanelOfType()
{
	for(auto& [uid, panel] : GetAllPanels())
	{
		if(auto casted = dynamic_cast<T*>(panel))
			return casted;
	}

	return nullptr;
}

template<typename T>
T* FindPanelByTitle(const std::string& title)
{
	for(auto& [uid, panel] : GetAllPanels())
	{
		if(panel && panel->GetTitle() == title)
			return static_cast<T*>(panel);
	}

	return nullptr;
}

template<typename T>
void DestroyPanelOfType(void)
{
	auto &panels = GetAllPanels();
	for (auto it = panels.begin(); it != panels.end();)
	{
		if (dynamic_cast<T*>(it->second))
		{
			delete it->second;
			it = panels.erase(it);
			return;
		}
		else
			++it;
	}
}

template<typename T>
void DestroyAllPanelsOfType(void)
{
	auto &panels = GetAllPanels();
	for (auto it = panels.begin(); it != panels.end();)
	{
		if (dynamic_cast<T*>(it->second))
		{
			delete it->second;
			it = panels.erase(it);
		}
		else
			++it;
	}
}

}