#!/usr/bin/lua

local config = require("config")

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

local COLOR = {
    reset  = "\27[0m",
    bold   = "\27[1m",
    red    = "\27[31m",
    green  = "\27[32m",
    yellow = "\27[33m",
    cyan   = "\27[36m",
}

local function log(level, color, fmt, ...)
    io.stderr:write(("%s%s[%s]%s " .. fmt .. "\n"):format(
        COLOR.bold, color, level, COLOR.reset, ..., COLOR.reset
    ))
end

local function info(fmt, ...)    log("INFO", COLOR.cyan, fmt, ...) end
local function success(fmt, ...) log("SUCCESS", COLOR.green, fmt, ...) end
local function warn(fmt, ...)    log("WARN", COLOR.yellow, fmt, ...) end
local function fail(fmt, ...)    log("ERROR", COLOR.red, fmt, ...) end

local function exists(path)
    local f = io.open(path, "rb")
    if f then
        f:close()
        return true
    end

    return os.rename(path, path) ~= nil
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

local function ran_ok(status)
    return status == true or status == 0
end

local function compiler()
    return "clang"
end

local function defines(release)
    local list = config.defines[PLATFORM] or {}
    local out = {}

    for _, define in ipairs(list) do
        table.insert(out, "-D" .. define)
    end

    if release then
        table.insert(out, "-DNDEBUG")
    end

    return out
end

local function subsystem_flags(release)
    -- Only meaningful on Windows; makes a windowed (GUI) app instead of a console app
    if not IS_WINDOWS or not release then
        return {}
    end

    return { "-Wl,/subsystem:windows", "-Wl,/entry:WinMainCRTStartup" }
end

local function include_flags()
    local out = {}

    for _, dir in ipairs(config.include_dirs) do
        table.insert(out, "-I" .. dir)
    end

    return out
end

local function link_flags()
    local libs = config.links[PLATFORM] or {}
    local out = {}

    for _, lib in ipairs(libs) do
        table.insert(out, "-l" .. lib)
    end

    local frameworks = config.frameworks and config.frameworks[PLATFORM] or {}
    for _, framework in ipairs(frameworks) do
        table.insert(out, "-framework")
        table.insert(out, framework)
    end

    return out
end

local function output_path(release)
    local build_type = release and "Release" or "Debug"
    local exe_name = IS_WINDOWS and (config.app_name .. ".exe") or config.app_name

    if IS_WINDOWS then
        return ("bin\\%s\\%s"):format(build_type, exe_name)
    else
        return ("bin/%s/%s"):format(build_type, exe_name)
    end
end

local function opt_flags(release)
    return release and { "-O2 -s" } or { "-O0 -g" }
end

local function compile(release)
    local build_dir = release and "bin/Release" or "bin/Debug"
    mkdir("bin")
    mkdir(build_dir)

    local out = output_path(release)

    local parts = {}
    table.insert(parts, compiler())

    for _, part in ipairs(opt_flags(release))     do table.insert(parts, part) end
    for _, part in ipairs(defines(release))       do table.insert(parts, part) end
    for _, part in ipairs(include_flags())        do table.insert(parts, part) end

    table.insert(parts, config.src)
    table.insert(parts, "-o")
    table.insert(parts, '"' .. out .. '"')

    for _, part in ipairs(link_flags())           do table.insert(parts, part) end
    for _, part in ipairs(subsystem_flags(release)) do table.insert(parts, part) end

    local cmd = table.concat(parts, " ")
    info("%s", cmd)

    if ran_ok(os.execute(cmd)) then
        success("Compiled '%s'", out)
        return 0
    else
        fail("Compilation failed")
        return 1
    end
end

local function run(release)
    local out = output_path(release)

    if not exists(out) then
        fail("Target '%s' does not exist", out)
        return 1
    end

    info("Running '%s'...\n", out)

    local ok
    if IS_WINDOWS then
        ok = os.execute('"' .. out .. '"')
    else
        ok = os.execute("./" .. out)
    end

    io.stderr:write("\n")
    if ran_ok(ok) then
        success("Program exited successfully")
    else
        fail("Program exited with errors")
    end

    return ok
end

local function clean()
    if not exists("bin") then
        warn("Nothing to clean")
        return
    end

    info("Deleting 'bin'...")
    rmdir("bin")
    success("Clean complete")
end

local function help()
    print(([[
%sbuild.lua%s

Usage:
  lua build.lua <action> <target>
  lua build.lua clean
  lua build.lua help

Actions:
  build       Compile the target
  run         Run an already-built target
  build_run   Compile the target, then run it if the build succeeds

Targets:
  debug       Debug build
  release     Release build

Examples:
  lua build.lua build debug
  lua build.lua build_run release
  lua build.lua run debug
  lua build.lua clean
]]):format(COLOR.bold, COLOR.reset))
end

local ACTIONS = {
    build     = function(release) return compile(release) end,
    run       = function(release) return run(release) end,
    build_run = function(release)
        local status = compile(release)
        if not ran_ok(status) then
            return status
        end
        return run(release)
    end,
    clean     = function(_) clean() return 0 end,
    help      = function(_) help() return 0 end,
}

local TARGETS = { debug = false, release = true }

local function main(args)
    if #args == 0 then
        help()
        return 0
    end

    local action_name = args[1]
    local action = ACTIONS[action_name]
    if not action then
        fail("Unknown action '%s'", action_name)
        return 1
    end

    local release = false
    if action_name == "build" or action_name == "run" or action_name == "build_run" then
        local target_name = args[2]
        if not target_name then
            fail("Action '%s' requires a target: debug or release", action_name)
            return 1
        end
        if TARGETS[target_name] == nil then
            fail("Unknown target '%s' (expected debug or release)", target_name)
            return 1
        end
        release = TARGETS[target_name]
    end

    return action(release)
end

os.exit(main({...}))
