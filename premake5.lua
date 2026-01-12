workspace "Naui"
    configurations { "Release" }
    startproject "Uphonic"

project "Naui"
    kind "SharedLib"
    language "C++"
	cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"

    architecture "x64"
    staticruntime "Off"
    conformancemode "On"

    files {
        "Naui/**.h",
        "Naui/**.c",
        "Naui/**.cpp",
        "Naui/**.hpp",
    }

    includedirs {
        "Naui",
        "Naui/Vendor",
        "Naui/Vendor/imgui",
        "Naui/Vendor/miniz",
        "Naui/Vendor/nlohmann",
        "Naui/Vendor/stb"
    }

    defines { "NDEBUG", "NAUI_EXPORT" }
    optimize "On"

project "Uphonic"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"

    architecture "x64"
    staticruntime "Off"
    conformancemode "On"

    files {
        "Uphonic/**.h",
        "Uphonic/**.c",
        "Uphonic/**.cpp",
        "UVI/**.cpp"
    }

    includedirs {
        ".",
        "Naui",
        "Naui/Vendor",
        "Naui/Vendor/imgui",
        "Uphonic",
        "Uphonic/Vendor/miniaudio",
        "Uphonic/Vendor/imgui-knobs",
        "UVI"
    }

    links { "Naui" }

    optimize "On"

    filter { "configurations:Release" }
        defines { "NDEBUG" }