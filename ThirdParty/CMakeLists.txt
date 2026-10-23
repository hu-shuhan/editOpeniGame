add_subdirectory(tinyXML2)
add_subdirectory(glad)
add_subdirectory(freetype-2.13.3)
add_subdirectory(eigen-3.4.0)
add_subdirectory(meshoptimizer-0.22)
add_subdirectory(vtkkwiml)
if (NOT EMSCRIPTEN)
    add_subdirectory(glfw-3.4)
endif ()
add_subdirectory(spdlog)
add_subdirectory(zlib)
add_subdirectory(cnpy)
if (NOT EMSCRIPTEN)
    add_subdirectory(Python)
endif ()
add_subdirectory(zstd/build/cmake)

if (ENABLE_CGNS_MODULE)
    # if you want to set cgns static build , please configure in CGNS directory's CMakeLists.txt
    # On Linux prefer static build to simplify deployment (avoid missing shared artifacts)
    if (UNIX AND NOT APPLE)
        set(CGNS_BUILD_SHARED OFF CACHE BOOL "Build shared CGNS library" FORCE)
    endif ()

    # Respect an explicit HDF5_DIR; otherwise locate an installed config file.
    # HDF5_HOME may name either the installation root or the config directory.
    if (NOT HDF5_DIR)
        find_path(HDF5_DIR
            NAMES hdf5-config.cmake HDF5Config.cmake
            HINTS "${HDF5_ROOT}" ENV HDF5_ROOT ENV HDF5_HOME ENV HDF5_DIR
            PATHS "C:/Program Files/HDF_Group/HDF5/1.13.0"
                  "D:/anaconda3/Library"
            PATH_SUFFIXES cmake share/cmake cmake/hdf5 lib/cmake/hdf5
            DOC "HDF5 CMake configuration directory")
    endif ()
    set(HDF5_BUILD_SHARED_LIBS OFF)
    set(HDF5_USE_STATIC_LIBRARIES ON)
    set(HDF5_PREFER_PARALLEL OFF)

    # Prefer serial HDF5 on Linux to avoid mpi.h dependency
    if (UNIX AND NOT APPLE)
        if (EXISTS "/usr/bin/h5cc")
            set(HDF5_C_COMPILER_EXECUTABLE "/usr/bin/h5cc" CACHE FILEPATH "Use serial HDF5 wrapper" FORCE)
        endif ()
        if (EXISTS "/usr/include/hdf5/serial")
            set(HDF5_INCLUDE_DIR "/usr/include/hdf5/serial" CACHE PATH "Serial HDF5 include dir" FORCE)
        endif ()
        set(HDF5_PREFER_PARALLEL OFF CACHE BOOL "Prefer parallel HDF5" FORCE)
    endif ()

    add_subdirectory(cgns)

    message(" HDF5 PATH: " ${HDF5_DIR})
endif ()

# CGNS creates uninstall unconditionally. Load it first, then let nanoflann
# attach its namespaced uninstall target to the existing target. If CGNS is
# disabled, nanoflann creates uninstall itself.
set(MASTER_PROJECT_HAS_TARGET_UNINSTALL ON CACHE BOOL
    "Share the uninstall target with the parent project" FORCE)
add_subdirectory(nanoflann)
