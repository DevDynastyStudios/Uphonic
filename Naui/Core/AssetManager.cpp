#include "AssetManager.h"
#include <stb/stb_image.h>

#include <unordered_map>
#include <filesystem>
#include <string>
#include <algorithm>

namespace Naui
{

static std::unordered_map<std::string, Image*> s_images;

void AssetManager::Initialize(Renderer &renderer, const char *assets_directory)
{
    namespace fs = std::filesystem;

    if (!fs::exists(assets_directory))
        return;
    
    for (auto &entry : fs::recursive_directory_iterator(assets_directory))
    {
        if (!entry.is_regular_file())
            continue;

        auto path = entry.path();
        if (path.extension() == ".png")
        {
            auto relative_path = fs::relative(path, assets_directory);
            
            std::string name = relative_path.string();
            std::replace(name.begin(), name.end(), '\\', '/');
            
            name = name.substr(0, name.length() - 4);
            
            int width, height, channels;
            stbi_uc *data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
            auto image = renderer.CreateImage(width, height, data);
            s_images[name] = image;
        }
    }
}

void AssetManager::Shutdown(Renderer &renderer)
{
    for (auto& image : s_images)
        renderer.DestroyImage(image.second);
}

Image &AssetManager::GetImage(const char *path)
{
    return *s_images[path];
}

}
