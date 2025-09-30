#pragma once
#include <string>
#include <vector>

// Save the current ImGui layout to disk under a name
void uph_save_layout(const char* name);

// Load a layout by name from disk and apply it
bool uph_load_layout(const char* name);

// Remove a layout by name from disk
bool uph_remove_layout(const char* name);

bool uph_layout_has_header(const char* name, const char* header);

bool uph_layout_is_immutable(const char* name);

// List all layout names stored on disk
std::vector<std::string> uph_list_layouts(const char* filename);