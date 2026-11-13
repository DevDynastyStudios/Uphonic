return {
    app_name = "Uphonic",
    src = "app/build.c",

    include_dirs = {
        ".",
        "naui/vendor",
        "app"
    },

    links = {
        windows = { "shell32", "user32", "dxgi", "d3d11", "d3dcompiler", "dxguid", "ole32", "advapi32" },
        linux = { "X11", "EGL", "m", "asound" },
        macos = { "m" }
    },

    frameworks = {
        macos = { "Cocoa", "Metal", "QuartzCore", "CoreMIDI.framework", "CoreFoundation.framework" }
    },

    defines = {
        windows = { "MGFX_D3D11" },
        linux = { "MGFX_OPENGL" },
        macos = { "MGFX_METAL" }
    }
}
