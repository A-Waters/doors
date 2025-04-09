-- Makes a path relative to the folder containing this script file.
ROOT_PATH = function(path)
    return string.format("%s/%s", _MAIN_SCRIPT_DIR, path)
end

PROJ_DIR = ROOT_PATH "projects"
DEP_DIR  = ROOT_PATH "dependencies"
BIN_DIR  = ROOT_PATH "_out/bin/%{cfg.buildcfg}-%{cfg.platform}/%{prj.name}"
OBJ_DIR  = ROOT_PATH "_out/obj/%{cfg.buildcfg}-%{cfg.platform}/%{prj.name}"

WORKSPACE_NAME = "doors"
START_PROJECT  = "frontend"

include "workspace.lua"
include "projects.lua"

for _, path in ipairs(PROJECTS) do
    include(path .. "/premake5.lua")
end

newaction {
    trigger = "bb",  
    description = "Build and run the project",
    execute = function ()
        -- Generate Visual Studio project files using Premake
        -- os.execute("premake5.exe vs2022")
        
        local build_command = string.format(
            '"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe" doors.sln /p:Configuration=Debug /p:Platform=x64'
        )
        local handle = io.popen(build_command)
        local result = handle:read("*a")
        print(result)
        handle:close()
        -- Run the compiled executable (make sure to update the path if needed)
        os.execute(ROOT_PATH("_out/bin/debug-x64/frontend/frontend.exe"))
    end
}