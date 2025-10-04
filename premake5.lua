workspace "Uphonic"
    configurations { "Release" }
    startproject "Uphonic"

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
        "vendor",
        "vendor/imgui",
        "vendor/mINI",
        "vendor/nlohmann",
        "vendor/imgui-knobs",
        "vendor/miniaudio",
        "vendor/FontAwesome",
        "vendor/vst2"
    }

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "On"