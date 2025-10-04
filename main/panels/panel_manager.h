#pragma once
#include <imgui.h>
#include <vector>

typedef void (*UphPanelCallback)(struct UphPanel* panel);

enum UphPanelFlags
{
	UphPanelFlags_None 			    = 0,
	UphPanelFlags_Panel			    = 1 << 1,
	UphPanelFlags_MenuBar           = 1 << 2,
	UphPanelFlags_Popup			    = 1 << 3,
	UphPanelFlags_Modal             = 1 << 4,
	UphPanelFlags_HiddenFromMenu    = 1 << 5
};

struct UphPanel
{
    const char* title;
	const char* category;
    bool is_visible;
	UphPanelFlags panel_flags;
    ImGuiWindowFlags window_flags;
    ImGuiDockNodeFlags dock_flags;
    UphPanelCallback render_callback;
	UphPanelCallback init_callback;
};

std::vector<UphPanel>& panels();

void uph_panel_register(const char* title, UphPanelFlags panel_flags, UphPanelCallback render_callback, UphPanelCallback init_callback);
void uph_panel_init_all();
void uph_panel_render_all();
void uph_panel_show(const char* title, bool visible = true);
UphPanel* uph_panel_get(const char* title);

// Registers a panel at static initialization time.
// Usage: UPH_REGISTER_PANEL("Title", &panel_data, panel_render_function);
#define UPH_REGISTER_PANEL(TITLE, PANEL_FLAGS, RENDER_FUNC, INIT_FUNC)            \
    static void __uph_register_##RENDER_FUNC(void) {                   	           \
        uph_panel_register(TITLE, PANEL_FLAGS, RENDER_FUNC, INIT_FUNC);	           \
    }                                                                               \
    struct __UphPanelRegistrar_##RENDER_FUNC {                                      \
        __UphPanelRegistrar_##RENDER_FUNC() { __uph_register_##RENDER_FUNC(); }     \
    };                                                                              \
    static __UphPanelRegistrar_##RENDER_FUNC __uph_panel_registrar_##RENDER_FUNC;