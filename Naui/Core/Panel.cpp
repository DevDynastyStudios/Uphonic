#include "Panel.h"

namespace Naui
{

static std::unordered_map<uint64_t, Panel*> s_panels;

std::unordered_map<uint64_t, Panel*> &GetAllPanels(void)
{
    return s_panels;
}

void Panel::SetCategory(const std::string &category)
{
    
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

}