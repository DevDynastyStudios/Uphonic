#include "project_manager.h"
#include <chrono>
#include <string>
#include <fstream>
#include <iostream>

#define UPH_DIRTY_FOLDER ".dirty"

UphProjectContext g_project_context = {};

inline std::filesystem::path uph_temp_file_path()
{
	return std::filesystem::temp_directory_path() / "uphonic";
}

static std::string uph_generate_unique_name() 
{
    auto ts = std::chrono::system_clock::now().time_since_epoch().count();
    return "untitled-" + std::to_string(ts);
}

void uph_project_init() 
{
    auto scratch_base = uph_temp_file_path();
    std::filesystem::create_directories(scratch_base);

    auto unique = uph_generate_unique_name();
    g_project_context.root = scratch_base / unique;
    std::filesystem::create_directories(g_project_context.root);
    std::filesystem::create_directories(g_project_context.root / "samples");
	std::filesystem::create_directories(g_project_context.root / "shrek porn");
	
    g_project_context.is_scratch = true;
	g_project_context.is_dirty = false;
}

void uph_project_shutdown() 
{
    if (!g_project_context.is_scratch)
		return;
        
    std::error_code error_code;
    std::filesystem::remove_all(g_project_context.root, error_code);
}

void uph_project_save_as(const std::filesystem::path& dest) 
{
    std::filesystem::create_directories(dest.parent_path());
    std::filesystem::rename(g_project_context.root, dest);

    g_project_context.root = dest;
    g_project_context.is_scratch = false;
}

void uph_project_load(const std::filesystem::path& path) 
{
    g_project_context.root = path;
    g_project_context.is_scratch = false;
}

void uph_project_set_dirty(bool dirty)
{
	g_project_context.is_dirty = dirty;
    auto marker = g_project_context.root / UPH_DIRTY_FOLDER;

    if (dirty)
        std::ofstream(marker).close();
    else 
	{
        std::error_code error_code;
        std::filesystem::remove(marker, error_code);
    }

}

std::vector<std::filesystem::path> uph_project_check_recovery() {
    std::vector<std::filesystem::path> candidates;
    auto scratch_base = uph_temp_file_path();

	if(!std::filesystem::exists(scratch_base))
		return candidates;

	for(auto& entry : std::filesystem::directory_iterator(scratch_base))
	{
		if(!entry.is_directory())
			continue;

		if(std::filesystem::exists(entry.path() / UPH_DIRTY_FOLDER))
			candidates.push_back(entry.path());
	}

	return candidates;
}

void uph_project_load_recovery()
{
	auto candidates = uph_project_check_recovery();
	if(candidates.size() <= 0)
		return;

	uph_project_load(candidates[0]);
}

void uph_project_clear_recovery()
{
    try 
	{
        auto candidates = uph_project_check_recovery();
        for (auto& path : candidates) 
		{
            if (std::filesystem::exists(path))
                std::filesystem::remove_all(path);

        }
    } catch (const std::exception& e) 
	{
        std::cerr << "Failed to clear recovery folders: " << e.what() << "\n";
    }
}