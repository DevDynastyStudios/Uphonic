#pragma once
#include "../panels/panel_manager.h"
#include <vector>
#include <imgui.h>
#include <string>
#include <unordered_map>

typedef void (*UphShortcutEvent)(UphShortcut shortcut);

struct UphShortcut
{
    std::string action;
    std::vector<ImGuiKey> keys;
    UphShortcutEvent callback;
};

void shortcut_register_global(const char* action, const std::vector<ImGuiKey>& keys);
void shortcut_register_local(UphPanel* panel, const char* action, const std::vector<ImGuiKey>& keys);

bool shortcut_combo_triggered(const std::vector<int>& keys);

extern std::vector<UphShortcut> g_shortcuts;