#pragma once
#include <imgui.h>
#include <vector>

typedef void (*UphPanelRenderCallback)(struct UphPanel* panel);

struct UphPanel
{
    const char* title;
    bool is_visible;
    ImGuiWindowFlags flags;
    UphPanelRenderCallback render_callback;
};

std::vector<UphPanel>& panels();

void uph_panel_register(const char* title, ImGuiWindowFlags window_flags, UphPanelRenderCallback render_callback, bool hidden_on_boot);
void uph_panel_render_all();
UphPanel* uph_panel_get(int id);

// Registers a panel at static initialization time.
// Usage: UPH_REGISTER_PANEL("Title", &panel_data, panel_render_function);
#define UPH_REGISTER_PANEL(TITLE, WINDOW_FLAGS, RENDER_FUNC, HIDDEN_ON_BOOT)        \
    static void __uph_register_##RENDER_FUNC(void) {                                \
        uph_panel_register(TITLE, WINDOW_FLAGS, RENDER_FUNC, HIDDEN_ON_BOOT);       \
    }                                                                               \
    struct __UphPanelRegistrar_##RENDER_FUNC {                                      \
        __UphPanelRegistrar_##RENDER_FUNC() { __uph_register_##RENDER_FUNC(); }     \
    };                                                                              \
    static __UphPanelRegistrar_##RENDER_FUNC __uph_panel_registrar_##RENDER_FUNC;