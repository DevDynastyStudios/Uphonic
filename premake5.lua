local host = os.host()
VST_SDK = os.getenv("VST_SDK")

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
    filter "system:windows"
        removefiles {
            "Naui/Vendor/imgui/imgui_impl_opengl3.*",
            "Naui/Vendor/imgui/imgui_impl_opengl3_loader.h",
            "Naui/Vendor/imgui/imgui_impl_xlib.*",
        }
    filter "system:linux"
        removefiles {
            "Naui/Vendor/imgui/imgui_impl_dx11.*",
            "Naui/Vendor/imgui/imgui_impl_win32.*"
        }
        links { "X11", "EGL" }

project "UVI"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    targetdir "bin/%{cfg.buildcfg}"
    architecture "x64"
    staticruntime "Off"
    conformancemode "On"
    files {
        "UVI/**.h",
        "UVI/**.cpp",
        VST_SDK.."/public.sdk/source/common/memorystream.cpp"
    }
    includedirs {
        ".",
        "UVI",
        VST_SDK
    }
    filter "system:windows"
        files { VST_SDK.."/public.sdk/source/vst/hosting/module_win32.cpp" }
        buildoptions { "/Zc:char8_t-" }
        defines { "NOMINMAX", "WIN32" }
    filter "system:linux"
        files { VST_SDK.."/public.sdk/source/vst/hosting/module_linux.cpp" }
        buildoptions { "-fno-char8_t" }
        defines { "LINUX" }
    filter "system:macosx"
        files { VST_SDK.."/public.sdk/source/vst/hosting/module_mac.cpp" }
    filter {}
    filter "configurations:Release"
        defines { "NDEBUG" }
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
    }
    includedirs {
        ".",
        "Naui",
        "Naui/Vendor",
        "Naui/Vendor/imgui",
        "Naui/Vendor/miniz",
        "Uphonic",
        "Uphonic/Vendor/stb",
        "Uphonic/Vendor/miniaudio",
        "Uphonic/Vendor/imgui-knobs",
        "UVI",
        VST_SDK
    }
    filter "configurations:Release"
        links {
            "UVI",
            "sdk_hosting",
            "sdk_common",
            "sdk",
            "base",
            "pluginterfaces"
        }
    filter "system:windows"
        libdirs { VST_SDK.."/build/lib/Release" }
        defines { "NOMINMAX", "WIN32" }
    filter "system:linux"
        libdirs { VST_SDK.."/build/lib" }
        defines { "LINUX" }
        links { "pthread", "dl", "X11" }
    filter "system:macosx"
        links {
            "CoreFoundation.framework",
            "Cocoa.framework"
        }
    filter {}
    links { "Naui" }
    optimize "On"
    filter "configurations:Release"
        defines { "NDEBUG" }