workspace "Uphonic"
    configurations { "Release" }
    startproject "Uphonic"

project "Uphonic"
    kind "ConsoleApp"
    language "C++"
	cppdialect "C++20"
    files {
        "main/**.h",
        "main/**.cpp",
        "vendor/imgui/**.h",
        "vendor/imgui/**.cpp",
        "vendor/nlohmann/**.hpp"
    }

    includedirs {
        "main",
        "vendor/imgui",
        "vendor/mINI",
        "vendor/nlohmann"
    }

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "On"