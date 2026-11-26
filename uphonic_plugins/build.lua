#!/usr/bin/lua

local IS_WINDOWS = package.config:sub(1, 1) == "\\"

local PLATFORM
if IS_WINDOWS then
    PLATFORM = "windows"
else
    local uname = io.popen("uname -s")
    local sysname = uname and uname:read("*l") or ""
    if uname then uname:close() end
    PLATFORM = (sysname == "Darwin") and "macos" or "linux"
end

local SEP = IS_WINDOWS and "\\" or "/"
local LIB_DIR = "uphonic_plugin_lib"
local GEN_DIR = "bin" .. SEP .. "gen"
local DEFAULT_FONT = LIB_DIR .. SEP .. "fonts" .. SEP .. "MYRIADPRO-REGULAR.ttf"

local LINKS = {
    windows = { "-luser32", "-lgdi32", "-lcomdlg32", "-ld2d1" },
    linux = { "-lX11", "-lGL", "-lm" },
    macos = { "-framework Cocoa", "-framework CoreFoundation", "-framework CoreGraphics" }
}

local COLOR = {
    reset = "\27[0m",
    bold = "\27[1m",
    red = "\27[31m",
    green = "\27[32m",
    cyan = "\27[36m",
}

local function log(level, color, fmt, ...)
    io.stderr:write(("%s%s[%s]%s " .. fmt .. "\n"):format(COLOR.bold, color, level, COLOR.reset, ...))
end

local function info(fmt, ...) log("INFO", COLOR.cyan, fmt, ...) end
local function success(fmt, ...) log("SUCCESS", COLOR.green, fmt, ...) end
local function fail(fmt, ...) log("ERROR", COLOR.red, fmt, ...) end

local function ran_ok(status)
    return status == true or status == 0
end

local function exists(path)
    local f = io.open(path, "rb")
    if f then
        f:close()
        return true
    end
    return false
end

local function mkdir(path)
    if IS_WINDOWS then
        os.execute(('mkdir "%s" >nul 2>nul'):format(path))
    else
        os.execute(('mkdir -p "%s"'):format(path))
    end
end

local function rmdir(path)
    if IS_WINDOWS then
        os.execute(('rmdir /S /Q "%s"'):format(path))
    else
        os.execute(('rm -rf "%s"'):format(path))
    end
end

local function write_file(path, content)
    local f = assert(io.open(path, "wb"))
    f:write(content)
    f:close()
end

local function generate_font_header()
    local out_path = GEN_DIR .. SEP .. "upl_default_font.h"
    if exists(out_path) then
        return true
    end

    local f = io.open(DEFAULT_FONT, "rb")
    if not f then
        fail("Missing default font '%s'", DEFAULT_FONT)
        return false
    end

    local data = f:read("*a")
    f:close()

    local lines = { "static const unsigned char upl__default_font_data[] = {" }
    for i = 1, #data, 24 do
        local bytes = {}
        for j = i, math.min(i + 23, #data) do
            bytes[#bytes + 1] = ("0x%02x"):format(data:byte(j))
        end
        lines[#lines + 1] = "\t" .. table.concat(bytes, ",") .. ","
    end
    lines[#lines + 1] = "};"

    mkdir(GEN_DIR)
    write_file(out_path, table.concat(lines, "\n") .. "\n")
    return true
end

local function info_plist(name, id)
    return ([[<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>%s</string>
    <key>CFBundleIdentifier</key>
    <string>%s</string>
    <key>CFBundleName</key>
    <string>%s</string>
    <key>CFBundlePackageType</key>
    <string>BNDL</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
</dict>
</plist>
]]):format(name, id, name)
end

local function compile(plugin, release)
    local source = plugin .. SEP .. plugin .. ".c"
    if not exists(source) then
        fail("Plugin source '%s' does not exist", source)
        return 1
    end

    if not generate_font_header() then
        return 1
    end

    local build_dir = "bin" .. SEP .. (release and "Release" or "Debug")
    mkdir(build_dir)

    local out = build_dir .. SEP .. plugin .. ".clap"
    if PLATFORM == "macos" then
        local macos_dir = out .. "/Contents/MacOS"
        mkdir(macos_dir)
        write_file(out .. "/Contents/Info.plist", info_plist(plugin, "com.uphonic." .. plugin))
        out = macos_dir .. "/" .. plugin
    end

    local parts = {
        "clang",
        "-std=gnu11",
        "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-missing-field-initializers",
        "-fvisibility=hidden",
        release and "-O2" or "-O0 -g",
        release and "-DNDEBUG" or "",
        "-I" .. LIB_DIR,
        "-Ivendor",
        "-I" .. GEN_DIR,
    }

    if PLATFORM == "linux" then
        table.insert(parts, "-fPIC -shared")
    elseif PLATFORM == "windows" then
        table.insert(parts, "-shared")
    else
        table.insert(parts, "-bundle")
    end

    table.insert(parts, source)
    if PLATFORM == "macos" then
        table.insert(parts, "-x objective-c")
    end
    table.insert(parts, LIB_DIR .. SEP .. "upl.c")
    table.insert(parts, '-o "' .. out .. '"')

    for _, link in ipairs(LINKS[PLATFORM]) do
        table.insert(parts, link)
    end

    local cmd = table.concat(parts, " ")
    info("%s", cmd)

    if ran_ok(os.execute(cmd)) then
        success("Compiled '%s'", build_dir .. SEP .. plugin .. ".clap")
        return 0
    end

    fail("Compilation failed")
    return 1
end

local function help()
    print(([[
%sbuild.lua%s (uphonic plugins)

Usage:
  lua build.lua build <plugin> <debug|release>
  lua build.lua clean
  lua build.lua help

<plugin> is a folder in uphonic_plugins containing <plugin>/<plugin>.c

Examples:
  lua build.lua build example_gain debug
  lua build.lua build example_gain release
]]):format(COLOR.bold, COLOR.reset))
end

local TARGETS = { debug = false, release = true }

local function main(args)
    local action = args[1]
    if action == "clean" then
        rmdir("bin")
        success("Clean complete")
        return 0
    end

    if action ~= "build" then
        help()
        return action == "help" and 0 or 1
    end

    local plugin = args[2]
    local release = TARGETS[args[3] or ""]
    if not plugin or release == nil then
        help()
        return 1
    end

    return compile(plugin, release)
end

os.exit(main({...}))
