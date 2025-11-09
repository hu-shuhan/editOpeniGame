add_subdirectory(tinyXML2)
add_subdirectory(glad)
add_subdirectory(freetype-2.13.3)
add_subdirectory(eigen-3.4.0)
add_subdirectory(meshoptimizer-0.22)
add_subdirectory(vtkkwiml)
add_subdirectory(glfw-3.4)
add_subdirectory(spdlog)
add_subdirectory(zlib)
add_subdirectory(Python)
add_subdirectory(nanoflann)
# CGNS
# Configure the compilation definition according to the value of MODULE_FLAG

if (ENABLE_LIBTORCH_MODULE)
    add_subdirectory(libtorch)
endif ()

if (ENABLE_CGNS_MODULE)

    #    find_package(Git QUIET)
    #    if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/../.git")
    #        # Update submodules as needed
    #        option(GIT_SUBMODULE "Check submodules during build" ON)
    #        if(GIT_SUBMODULE)
    #            message(STATUS "Submodule update")
    #            execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    #                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    #                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    #            if(NOT GIT_SUBMOD_RESULT EQUAL "0")
    #                message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    #            endif()
    #        endif()
    #    endif()

    # if you want to set cgns static build , please configure in CGNS directory's CMakeLists.txt
    # Set CGNS_BUILD_SHARED to "OFF" which is on line 112 of the CMakeLists.txt
    #    set(CGNS_ENABLE_HDF5 OFF)
    # set(HDF5_DIR "C:/Program Files/HDF_Group/HDF5/1.13.0/share/cmake")

    # 在系统环境变量里设置HDF5_HOME，值为HDF5路径
    if (DEFINED ENV{HDF5_HOME})
        set(HDF5_DIR "$ENV{HDF5_HOME}/cmake")
    else ()
        #        set(HDF5_DIR "D:/dev/HDF5/1.13.0/share/cmake")
        set(HDF5_DIR "C:/Program Files/HDF_Group/HDF5/1.13.0/share/cmake")
        #set(HDF5_DIR "D:/OpeniGame/1.13.0/share/cmake")

    endif ()
    set(HDF5_BUILD_SHARED_LIBS OFF)
    set(HDF5_USE_STATIC_LIBRARIES ON)
    set(HDF5_PREFER_PARALLEL OFF)
    add_subdirectory(cgns)

    message(" HDF5 PATH: " ${HDF5_DIR})
endif ()

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(zstd/build/cmake EXCLUDE_FROM_ALL)
