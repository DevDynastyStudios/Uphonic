workspace "Uphonic"
    configurations { "Release" }
    startproject "Uphonic"

project "UVI"
    kind "StaticLib"
    architecture "x64"
    language "C++"
    cppdialect "C++20"
    files {
        "uvi/**.h",
        "uvi/**.cpp"
    }

project "Uphonic"
    kind "ConsoleApp"
    architecture "x64"
    language "C++"
	cppdialect "C++20"
    files {
        "main/**.h",
        "main/**.cpp",
        "vendor/imgui/**.h",
        "vendor/imgui/**.cpp",
        "vendor/imgui-knobs/**.h",
        "vendor/imgui-knobs/**.cpp",
        "vendor/miniaudio/**.h",
        "vendor/miniaudio/**.c"
    }

    includedirs {
        "main",
        "uvi",
        "vendor",
        "vendor/imgui",
        "vendor/mINI",
        "vendor/nlohmann",
        "vendor/imgui-knobs",
        "vendor/miniaudio",
        "vendor/FontAwesome",
        "vendor/vst2"
    }

    links { "UVI" }

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "On"

    filter "system:windows"
        removefiles {
            "vendor/imgui/imgui_impl_sdl2.cpp",
            "vendor/imgui/imgui_impl_sdl2.h",
            "vendor/imgui/imgui_impl_opengl3_loader.cpp",
            "vendor/imgui/imgui_impl_opengl3.h",
            "vendor/imgui/imgui_impl_opengl3.cpp"
        }

    filter "system:linux"
        removefiles {
            "vendor/imgui/imgui_impl_win32.cpp",
            "vendor/imgui/imgui_impl_win32.h",
            "vendor/imgui/imgui_impl_dx11.cpp",
            "vendor/imgui/imgui_impl_dx11.h"
        }
        links { "SDL2", "GL", "dl", "m" }