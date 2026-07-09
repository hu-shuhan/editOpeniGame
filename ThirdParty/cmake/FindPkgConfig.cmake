find_program(IGAME_PKG_CONFIG_EXECUTABLE NAMES pkg-config pkgconf)

if (IGAME_PKG_CONFIG_EXECUTABLE)
    set(PKG_CONFIG_EXECUTABLE "${IGAME_PKG_CONFIG_EXECUTABLE}"
            CACHE FILEPATH "Path to a program" FORCE)
    include("${CMAKE_ROOT}/Modules/FindPkgConfig.cmake")
elseif (TARGET PkgConfig::ZSTD)
    # 已有 CMake 目标时无需调用宿主 pkg-config
    set(PkgConfig_FOUND TRUE)
    set(PKG_CONFIG_FOUND TRUE)

    macro(pkg_search_module prefix)
        if ("${prefix}" STREQUAL "ZSTD" AND TARGET PkgConfig::ZSTD)
            set(${prefix}_FOUND TRUE)
        else ()
            message(FATAL_ERROR
                    "pkg-config is unavailable and no CMake target can satisfy ${prefix}")
        endif ()
    endmacro()
else ()
    include("${CMAKE_ROOT}/Modules/FindPkgConfig.cmake")
endif ()
