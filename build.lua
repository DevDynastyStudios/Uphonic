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
  lua build.lua debug         Build Debug
  lua build.lua release       Build Release
  lua build.lua run_debug     Run Debug build
  lua build.lua run_release   Run Release build
  lua build.lua clean         Delete bin/
  lua build.lua help          Show this help
]]):format(COLOR.bold, COLOR.reset))
end

local COMMANDS = {
    debug       = function(release) compile(release) end,
    release     = function(release) compile(release) end,
    run_debug   = function(_)       run(false) end,
    run_release = function(_)       run(true) end,
    clean       = function(_)       clean() end,
    help        = function(_)       help() end,
}

local function main(args)
    if #args == 0 then
        help()
        return 0
    end

    for _, arg in ipairs(args) do
        if not COMMANDS[arg] then
            fail("Unknown command '%s'", arg)
            return 1
        end
    end

    local release = false
    for _, arg in ipairs(args) do
        if arg == "release" then
            release = true
        end
    end

    for _, arg in ipairs(args) do
        COMMANDS[arg](release)
    end

    return 0
end

os.exit(main({...}))
