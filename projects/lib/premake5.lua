project "lib"
    kind          "StaticLib"
    language      "C++"
    cppdialect    "C++17"
    staticruntime "on"
    systemversion "latest"
    warnings      "extra"
    targetdir     (BIN_DIR)
    objdir        (OBJ_DIR)
    files {
        "src/**.h",
        "src/**.hpp",
        "src/**.cpp",
    }
    includedirs {
        "include/lib",
        "src/"
    }
    links {
        
    }
    defines {

    }
