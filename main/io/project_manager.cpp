#include "project_manager.h"
#include "project_serializer.h"
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

std::wstring uph_project_name() 
{
    if (g_project_context.root.empty())
        return L""; // scratch project, no name yet

    return g_project_context.root.filename().wstring();
}

void uph_project_new()
{
	uph_project_clear();
}

void uph_project_init() 
{
    auto scratch_base = uph_temp_file_path();
    std::filesystem::create_directories(scratch_base);

    auto unique = uph_generate_unique_name();
    g_project_context.root = scratch_base / unique;
	std::filesystem::path work_dir = g_project_context.root / "workspace";
    std::filesystem::create_directories(g_project_context.root);
    std::filesystem::create_directories(work_dir);
	std::filesystem::create_directories(work_dir / "samples");
	std::filesystem::create_directories(work_dir / "mixers");
	
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

// (Chimpchi) Fix the duplication later, and refactor whole file.
void uph_project_save_as(const std::filesystem::path& dest) 
{
    if (dest.empty())
        return;

    auto parent = dest.parent_path();
    auto name   = dest.stem();
    if (name.empty())
        return;

    auto project_root = parent / name;

    std::filesystem::create_directories(project_root / "workspace" / "samples");
    std::filesystem::create_directories(project_root / "workspace" / "mixers");

    auto manifest = project_root / "project-data.json";
    uph_project_serializer_save_json(project_root, manifest.filename().string().c_str());

    g_project_context.root = project_root;
    g_project_context.is_scratch = false;

}

void uph_project_load(const std::filesystem::path& path) 
{
	if (path.empty()) 
		return;

	uph_project_serializer_load_json(path);
    g_project_context.root = path;
    g_project_context.is_scratch = false;
	g_project_context.is_loaded_project = true;
}

void uph_project_save_as_via_dialog() {
	const wchar_t* default_name = uph_project_name().c_str();
    auto filePath = uph_save_file_dialog(L"Uphonic Project\0*.uphproj\0All Files\0*.*\0", L"Save Project As", default_name);
    if (filePath.empty())
        return;

    auto parent = filePath.parent_path();
    auto name   = filePath.stem();
    if (name.empty())
        return;

    auto project_root = uph_create_project_folders(parent / name);

    auto manifest = project_root / "project-data.json";
    uph_project_serializer_save_json(project_root, manifest.filename().string().c_str());

    g_project_context.root = project_root;
    g_project_context.is_scratch = false;

}

std::filesystem::path uph_create_project_folders(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path / "workspace" / "samples");
    std::filesystem::create_directories(path / "workspace" / "mixers");
    return path;
}

void uph_project_set_dirty(bool dirty)
{
	if(g_project_context.is_dirty == dirty)
		return;

	auto marker = g_project_context.root / UPH_DIRTY_FOLDER;
	g_project_context.is_dirty = dirty;

    if (dirty)
		std::ofstream(marker).close();
    else 
	{
		std::error_code error_code;
        std::filesystem::remove(marker, error_code);	// Possibly set the is dirty flag to false if we can't write the marker folder
    }
	
}

bool uph_project_is_dirty()
{
	return g_project_context.is_dirty;
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