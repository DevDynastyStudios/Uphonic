#pragma once
#include "../types.h"
#include <filesystem>

void uph_project_serializer_save_json(const std::filesystem::path& path, const char* file_name);
void uph_project_serializer_load_json(const std::filesystem::path& root);
void uph_project_clear();