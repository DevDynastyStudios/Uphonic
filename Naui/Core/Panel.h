#pragma warning(push)
#pragma warning(disable : 4251)

#pragma once

#include "Base.h"
#include "Shortcut/ShortcutTable.h"

#include <imgui.h>

#include <cstdint>
#include <string>
#include <cfloat>
#include <functional>
#include <unordered_map>
#include <typeinfo>

#if defined(__GNUC__) || defined(__clang__)
	#include <cxxabi.h>
#endif

namespace Naui
{

class NAUI_API PanelImGuiImpl
{
public:
	static constexpr int PRIORITY_GLOBAL = 10;
	static constexpr int PRIORITY_PANEL = 20;

	void SetResizable(bool value)	{ value ? m_imguiFlags &= ~ImGuiWindowFlags_NoResize		  	: m_imguiFlags |= ImGuiWindowFlags_NoResize; }
	void SetMovable(bool value)	  	{ value ? m_imguiFlags &= ~ImGuiWindowFlags_NoMove				: m_imguiFlags |= ImGuiWindowFlags_NoMove; }
	void SetMinimizable(bool value)  { value ? m_imguiFlags &= ~ImGuiWindowFlags_NoCollapse			: m_imguiFlags |= ImGuiWindowFlags_NoCollapse; }
	void SetAutoResize(bool value)   { value ? m_imguiFlags |= ImGuiWindowFlags_AlwaysAutoResize   	: m_imguiFlags &= ~ImGuiWindowFlags_AlwaysAutoResize; }
	void SetDockable(bool value)	 { value ? m_imguiFlags &= ~ImGuiWindowFlags_NoDocking		 	: m_imguiFlags |= ImGuiWindowFlags_NoDocking; }
	void SetSerializable(bool value) { value ? m_imguiFlags &= ~ImGuiWindowFlags_NoSavedSettings   	: m_imguiFlags |= ImGuiWindowFlags_NoSavedSettings; }

	void SetViewportPos(const ImVec2& pos, ImGuiCond cond = ImGuiCond_Always)
	{
		ImGuiViewport* view = ImGui::GetMainViewport();
		ImVec2 screenPos  = view->Pos;
		ImVec2 screenSize = view->Size;
		ImGui::SetWindowPos(ImVec2(screenPos.x + pos.x * screenSize.x, screenPos.y + pos.y * screenSize.y), cond);
	}

	void SetMaxSize(float x, float y) { m_maxSize = ImVec2(x, y); }
	void SetMinSize(float x, float y) { m_minSize = ImVec2(x, y); }

	// Sets only the visible portion of the title, preserving the ###layoutID suffix if present
	void SetDisplayTitle(const std::string& title)
	{
		m_title = m_layoutID.empty() ? title : title + "###" + m_layoutID;
	}

	// Sets the stable layout ID used by ImGui for window identity (###ID) and updates the full title
	void SetLayoutID(const std::string& id)
	{
		m_layoutID = id;
		m_title	= GetDisplayTitle() + "###" + id;
	}

	// Returns only the human-readable portion of the title (before ###)
	std::string GetDisplayTitle() const
	{
		auto pos = m_title.find("###");
		return (pos != std::string::npos) ? m_title.substr(0, pos) : m_title;
	}
	
	bool IsFocused() const;
	std::string	GetTitle() const { return m_title; }
	const std::string& GetLayoutID() const { return m_layoutID; }
	ImGuiWindowFlags GetWindowFlags() const { return m_imguiFlags; }
	ShortcutTable& GetShortcuts() { return m_shortcuts; }
	ImVec2 m_minSize = ImVec2(0, 0);
	ImVec2 m_maxSize = ImVec2(FLT_MAX, FLT_MAX);

protected:
	virtual void OnRegisterShortcuts(Naui::ShortcutTable& table) {}
	virtual void OnFocus()   {}
	virtual void OnUnfocus() {}

	ImGuiWindowFlags m_imguiFlags = 0;
	std::string m_title;
	std::string m_layoutID;
	ShortcutTable m_shortcuts;
	bool m_shortcutsRegistered = false;
};

class NAUI_API Panel : public PanelImGuiImpl
{
public:
	Panel(void) = default;
	Panel(const char* title) { m_title = title; }

	static constexpr int PRIORITY_GLOBAL = 10;
	static constexpr int PRIORITY_PANEL = 20;

	const uint64_t GetUID() const { return (uint64_t)this; }
	const std::string& GetTypeName() const { return m_typeName; }

	void SetOpen(bool value) { m_open = value; m_calledClose = false; }
	bool IsOpen(void) const { return m_open; }
	void SetTypeName(const std::string& type)  { m_typeName = type; }

protected:
	virtual void OnRegisterShortcuts(ShortcutTable& table) { }
	virtual void OnRender(void) { }
	virtual void OnClose(void)  { }

	void SetClosable(bool value) { m_closable = value; }
	void SetCategory(const std::string& category);

private:
	bool m_closable	= true;
	bool m_open	= true;
	bool m_calledClose = false;
	bool m_registered = false;
	std::string m_typeName;

	friend class PanelRenderer;
};



using PanelFactory = std::function<Panel*(void)>;

NAUI_API std::unordered_map<uint64_t, Panel*>& GetAllPanels(void);
NAUI_API std::unordered_map<std::string, int>&  GetPanelTypeCounters(void);
NAUI_API std::unordered_map<std::string, PanelFactory>& GetPanelFactories(void);

NAUI_API void   RegisterPanelFactory(const std::string& typeName, PanelFactory factory);
NAUI_API Panel* CreatePanelByType(const std::string& typeName, const std::string& layoutID);
NAUI_API void   ResetPanelTypeCounters(void);

NAUI_API void DestroyPanel(uint64_t uid);
NAUI_API void DestroyAllPanels(void);

inline std::string NormalizeTypeName(const char* name)
{
	std::string s;

#if defined(__GNUC__) || defined(__clang__)
	int status = 0;
	char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	s = (status == 0 && demangled) ? demangled : name;
	std::free(demangled);
#else
	s = name;
#endif

	for (const char* prefix : { "class ", "struct " })
	{
		if (s.rfind(prefix, 0) == 0)
		{
			s.erase(0, std::strlen(prefix));
			break;
		}
	}

	return s;
}

// Call once per panel type at startup so that Layout::Load can recreate panels from a file
template<typename T>
void RegisterPanel()
{
	RegisterPanelFactory(typeid(T).name(), []() -> Panel*
	{
		Panel* p = new T;
		p->SetTypeName(typeid(T).name());
		return p;
	});
}

template<typename T>
Panel& AddPanel()
{
	Panel* p = new T;

	const std::string typeName = typeid(T).name();
	p->SetTypeName(typeName);

	const int idx = GetPanelTypeCounters()[typeName]++;
	p->SetLayoutID(typeName + "_" + std::to_string(idx));

	GetAllPanels()[p->GetUID()] = p;
	return *p;
}

template<typename T>
T* GetPanelOfType()
{
	for (auto& [uid, panel] : GetAllPanels())
	{
		if (auto casted = dynamic_cast<T*>(panel))
			return casted;
	}

	return nullptr;
}

template<typename T>
T* FindPanelByTitle(const std::string& title)
{
	for (auto& [uid, panel] : GetAllPanels())
	{
		if (panel && panel->GetDisplayTitle() == title)
			return static_cast<T*>(panel);
	}

	return nullptr;
}

template<typename T>
void DestroyPanelOfType()
{
	auto& panels = GetAllPanels();
	for (auto it = panels.begin(); it != panels.end(); ++it)
	{
		if (dynamic_cast<T*>(it->second))
		{
			delete it->second;
			panels.erase(it);
			return;
		}
	}
}

template<typename T>
void DestroyAllPanelsOfType()
{
	auto& panels = GetAllPanels();
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

#pragma warning(pop)