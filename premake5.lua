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
        "vendor",
        "vendor/imgui",
        "vendor/mINI"
    }

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "On"