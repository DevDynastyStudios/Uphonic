#pragma once

#define UPHONIC_LAYOUTS_FOLDER naui_path_join(UPHONIC_FOLDER, NAUI_PATH("layouts"))
#define UPHONIC_BUILTIN_LAYOUTS_FOLDER naui_path_join(naui_directory_get(NAUI_DIR_ASSETS), NAUI_PATH("Layouts"))
#define UPH_LAYOUT_EXTENSION ".json"
#define UPH_LAYOUT_BUILTIN_PREFIX ":"
#define UPH_LAYOUT_BUILTIN_DEFAULT "layout.default"

void uph_layouts_apply_saved(void);
void uph_layouts_render_menu(void);
