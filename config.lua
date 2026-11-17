return {
    app_name = "Uphonic",
    src = {
        windows = "uphonic/build.c",
        linux = "uphonic/build.c",
        macos = "uphonic/build.m"
    },

    include_dirs = {
        ".",
        "naui/vendor",
        "uphonic"
    },

    links = {
        windows = { "shell32", "user32", "gdi32", "dxgi", "d3d11", "d3dcompiler", "dxguid", "ole32", "advapi32", "ws2_32", "imm32" },
        linux = { "X11", "Xext", "Xcomposite", "EGL", "m", "asound" },
        macos = { "m" }
    },

    frameworks = {
        macos = { "Cocoa", "Metal", "QuartzCore", "CoreMIDI", "CoreFoundation" }
    },

    defines = {
        windows = { "MGFX_D3D11" },
        linux = { "MGFX_OPENGL" },
        macos = { "MGFX_METAL" }
    }
}
