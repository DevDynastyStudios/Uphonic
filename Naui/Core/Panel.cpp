#include "Panel.h"

namespace Naui
{

static std::unordered_map<uint64_t, Panel*> s_panels;
static std::unordered_map<std::string, int> s_panelTypeCounters;
static std::unordered_map<std::string, PanelFactory> s_panelFactories;

std::unordered_map<uint64_t, Panel*>& GetAllPanels(void)
{
	return s_panels;
}

std::unordered_map<std::string, int>& GetPanelTypeCounters(void)
{
	return s_panelTypeCounters;
}

std::unordered_map<std::string, PanelFactory>& GetPanelFactories(void)
{
	return s_panelFactories;
}

void ResetPanelTypeCounters(void)
{
	s_panelTypeCounters.clear();
}

void RegisterPanelFactory(const std::string& typeName, PanelFactory factory)
{
	s_panelFactories[NormalizeTypeName(typeName.c_str())] = std::move(factory);
}

Panel* CreatePanelByType(const std::string& typeName, const std::string& layoutID)
{
	auto it = s_panelFactories.find(typeName);
	if (it == s_panelFactories.end())
		return nullptr;

	Panel* p = it->second();
	p->SetLayoutID(layoutID);
	s_panels[p->GetUID()] = p;
	return p;
}

void DestroyPanel(uint64_t uid)
{
	delete s_panels[uid];
	s_panels.erase(uid);
}

void DestroyAllPanels(void)
{
	for (auto it = s_panels.begin(); it != s_panels.end();)
	{
		delete it->second;
		it = s_panels.erase(it);
	}
}

void Panel::SetCategory(const std::string& category)
{

}

}