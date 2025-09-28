#pragma once
#include <imgui.h>
#include <vector>

typedef void (*UphPanelRenderCallback)(struct UphPanel* panel);

struct UphPanel
{
    const char* title;
    bool is_visible;
    ImGuiWindowFlags window_flags;
    ImGuiDockNodeFlags dock_flags;
    UphPanelRenderCallback render_callback;
};

std::vector<UphPanel>& panels();

void uph_panel_register(const char* title, ImGuiWindowFlags window_flags, ImGuiDockNodeFlags dock_flags, UphPanelRenderCallback render_callback);
void uph_panel_render_all();

// Registers a panel at static initialization time.
// Usage: UPH_REGISTER_PANEL("Title", &panel_data, panel_render_function);
#define UPH_REGISTER_PANEL(TITLE, WINDOW_FLAGS, DOCK_FLAGS, RENDER_FUNC)            \
    static void __uph_register_##RENDER_FUNC(void) {                                \
        uph_panel_register(TITLE, WINDOW_FLAGS, DOCK_FLAGS, RENDER_FUNC);           \
    }                                                                               \
    struct __UphPanelRegistrar_##RENDER_FUNC {                                      \
        __UphPanelRegistrar_##RENDER_FUNC() { __uph_register_##RENDER_FUNC(); }     \
    };                                                                              \
    static __UphPanelRegistrar_##RENDER_FUNC __uph_panel_registrar_##RENDER_FUNC;