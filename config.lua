return {
    app_name = "Uphonic",
    src = "uphonic/build.c",

    include_dirs = {
        ".",
        "naui/vendor",
        "uphonic"
    },

    links = {
        windows = { "shell32", "user32", "gdi32", "dxgi", "d3d11", "d3dcompiler", "dxguid", "ole32", "advapi32", "ws2_32", "imm32" },
        linux = { "X11", "Xext", "Xcomposite", "Xdamage", "Xcursor", "vulkan", "m", "asound" },
        macos = { "m" }
    },

    frameworks = {
        macos = { "Cocoa", "Metal", "QuartzCore", "CoreMIDI", "CoreFoundation" }
    },

    defines = {
        windows = { "MGFX_D3D11" },
        linux = { "MGFX_VULKAN", "_GNU_SOURCE" },
        macos = { "MGFX_METAL" }
    }
}
