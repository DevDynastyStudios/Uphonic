#pragma once
#include <string>
#include <vector>

void uph_save_layout(const char* name);
bool uph_load_layout(const char* name);
bool uph_remove_layout(const char* name);
bool uph_layout_has_header(const char* name, const char* header);
bool uph_layout_is_immutable(const char* name);
std::vector<std::string> uph_list_layouts(const char* filename);

void uph_layout_request_save(const char* name);
void uph_layout_request_load(const char* name);
void uph_layout_clear_requests();
void uph_layout_process_requests();