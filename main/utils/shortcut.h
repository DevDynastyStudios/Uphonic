#pragma once
#include <vector>
#include <imgui.h>

typedef void (*UphShortcutEvent)();

struct UphShortcut
{
    std::vector<ImGuiKey> keys;
    UphShortcutEvent callback;
};
