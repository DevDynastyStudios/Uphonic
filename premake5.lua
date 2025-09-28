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
        "vendor/imgui-knobs/**.h",
        "vendor/imgui-knobs/**.cpp"
    }

    includedirs {
        "main",
        "vendor",
        "vendor/imgui",
        "vendor/mINI",
        "vendor/nlohmann",
        "vendor/vst2"
    }

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "On"