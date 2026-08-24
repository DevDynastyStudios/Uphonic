return {
    app_name = "Uphonic",
    src = "app/build.c",

    include_dirs = {
        ".",
        "naui/vendor",
        "app"
    },

    links = {
        windows = { "shell32", "user32", "dxgi", "d3d11", "d3dcompiler", "dxguid" },
        linux = { "X11", "vulkan", "m" },
        macos = { "m" }
    },

    frameworks = {
        macos = { "Cocoa", "Metal", "QuartzCore" }
    },

    defines = {
        windows = { "MGFX_D3D11" },
        linux = { "MGFX_VULKAN" },
        macos = { "MGFX_METAL" }
    }
}
