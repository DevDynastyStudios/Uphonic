#pragma once
#include <filesystem>
#include <stdbool.h>

struct UphProjectContext 
{
    std::filesystem::path root;
    bool is_scratch;
	bool is_dirty;
};

extern UphProjectContext g_project_context;

void uph_project_init();
void uph_project_shutdown();

void uph_project_save_as(const std::filesystem::path& dest);
void uph_project_load(const std::filesystem::path& path);
void uph_project_set_dirty(bool dirty);
bool uph_project_is_dirty();
std::vector<std::filesystem::path> uph_project_check_recovery();
void uph_project_load_recovery();
void uph_project_clear_recovery();

inline const std::filesystem::path& uph_project_root() { return g_project_context.root; }