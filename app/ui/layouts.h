#pragma once

#define UPHONIC_LAYOUTS_FOLDER naui_path_join(UPHONIC_FOLDER, NAUI_PATH("layouts"))
#define UPH_LAYOUT_EXTENSION ".json"

void uph_layouts_apply_default(void);
void uph_layouts_apply_saved(void);
bool uph_layouts_load(const char *name);
void uph_layouts_render_menu(void);
