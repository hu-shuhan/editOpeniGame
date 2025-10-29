set(MODULE_NAME iGameCore_module)
set(CMAKE_CXX_STANDARD 20)
#=====添加路径======
set(Qt5_DIR "D:/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5")
#=================
set(IGAMEVIS_CORE_ROOT_DIR ${IGAMEVIS_ROOT_DIR}/iGameCore)
#set(LIBTORCH_DIR "${CMAKE_SOURCE_DIR}/ThirdParty/libtorch")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DPLATFORM_WINDOWS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DPLATFORM_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(-DPLATFORM_MAC)
endif ()

#--------------------------------------------------------------------
# Core Module's main Cmake
#--------------------------------------------------------------------
file(GLOB_RECURSE IGAME_CORE_FILE CONFIGURE_DEPENDS ./*.h ./*.cpp)

# Currently, only shared libraries for the Linux system are supported
if (IGAME_BUILD_SHARED AND UNIX AND NOT APPLE)
    add_library(${MODULE_NAME} SHARED ${IGAME_CORE_FILE})
else ()
    add_library(${MODULE_NAME} STATIC ${IGAME_CORE_FILE} "IO/Fluent/iGameCASReader.h")
endif ()

if(UNIX AND NOT APPLE)
    find_package(OpenMP REQUIRED)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(${MODULE_NAME} PUBLIC OpenMP::OpenMP_CXX)
    endif()
endif()

# add rendering core
add_subdirectory(Rendering)

target_include_directories(${MODULE_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CSTest>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Core/CellModel>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Core/Common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Core/DataModel>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Core/FilterModel>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Core/Mapper>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/IO>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/IO/XML>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Filters>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Rendering>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Core/Interactor>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Core/Painter>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Core/RenderWindow>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Rendering/OpenGL>

        $<INSTALL_INTERFACE:include/Core/CellModel>
        $<INSTALL_INTERFACE:include/Core/Common>
        $<INSTALL_INTERFACE:include/Core/DataModel>
        $<INSTALL_INTERFACE:include/Core/FilterModel>
        $<INSTALL_INTERFACE:include/Core/Mapper>
        $<INSTALL_INTERFACE:include/IO>
        $<INSTALL_INTERFACE:include/IO/XML>
        $<INSTALL_INTERFACE:include/IO/TestIO>
        $<INSTALL_INTERFACE:include/Filters>
        $<INSTALL_INTERFACE:include/Rendering>
        $<INSTALL_INTERFACE:include/Rendering/Common>
        $<INSTALL_INTERFACE:include/Rendering/Core>
        $<INSTALL_INTERFACE:include/Rendering/Core/Interactor>
        $<INSTALL_INTERFACE:include/Rendering/Core/Painter>
        $<INSTALL_INTERFACE:include/Rendering/Core/RenderWindow>
        $<INSTALL_INTERFACE:include/Rendering/OpenGL>

)
set(ThirdParty_lib_dependency)
list(APPEND ThirdParty_lib_dependency
        glad
        freetype
        eigen
        meshoptimizer
        tinyXML2
        vtkkwiml
        lzma
        glfw
        spdlog_header_only
        zlibstatic
)

#--------------------------------------------------------------------
# Find or Enable ThirdParty Components' Stuff.
#--------------------------------------------------------------------
if (ENABLE_CGNS_MODULE)
    target_compile_definitions(${MODULE_NAME} PUBLIC CGNS_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC cgns_static)
    #        find_package(HDF5)
    #        target_link_libraries(
    #         ${MODULE_NAME} PUBLIC
    #         ${HDF5_LIBRARIES}/../libzlib.lib
    #         ${HDF5_LIBRARIES}/../libsz.lib
    #         ${HDF5_LIBRARIES}/../libaec.lib)
ENDIF ()

if (ENABLE_LIBTORCH_MODULE)
    target_compile_definitions(${MODULE_NAME} PUBLIC LibTorch_ENABLE)

    target_include_directories(${MODULE_NAME} PRIVATE
            ${LIBTORCH_DIR}/include
            ${LIBTORCH_DIR}/include/torch/csrc/api/include
    )

    target_link_libraries(${MODULE_NAME} PRIVATE
            ${LIBTORCH_DIR}/lib/torch.lib
            ${LIBTORCH_DIR}/lib/c10.lib
            ${LIBTORCH_DIR}/lib/torch_cpu.lib
    )
endif ()

if (ENABLE_LIBTORCH_MODULE)
    # Include
    #    target_include_directories(${MODULE_NAME} PUBLIC
    #            ${LIBTORCH_DIR}/include
    #            ${LIBTORCH_DIR}/include/torch/csrc/api/include
    #    )
    target_include_directories(${MODULE_NAME} PRIVATE
            $<BUILD_INTERFACE:${LIBTORCH_DIR}/include>
            $<BUILD_INTERFACE:${LIBTORCH_DIR}/include/torch/csrc/api/include>
    )

    message(STATUS "LIBTORCH_DIR: ${LIBTORCH_DIR}")
    message(STATUS "Include directories: ${LIBTORCH_DIR}/include ${LIBTORCH_DIR}/include/torch/csrc/api/include")


    # Link
    target_link_libraries(${MODULE_NAME} PRIVATE
            ${LIBTORCH_DIR}/lib/torch.lib
            ${LIBTORCH_DIR}/lib/c10.lib
            ${LIBTORCH_DIR}/lib/torch_cpu.lib
    )


    message(STATUS "LibTorch include directories: ${LIBTORCH_DIR}/include")
    message(STATUS "LibTorch libraries: ${LIBTORCH_DIR}/lib/torch.lib ${LIBTORCH_DIR}/lib/c10.lib")
endif ()

#find_package(AbqSDK)
#if (${AbqSDK_FOUND})
#    message("Find abqsdk")
#    target_compile_definitions(${MODULE_NAME} PUBLIC AbqSDK_ENABLE)
#    target_link_libraries(${MODULE_NAME} PUBLIC ${ABQ_LIB_TARGETS})
#endif ()

find_package(FFMPEG)
if (${FFMPEG_FOUND})
    message("Find FFMPEG")
    target_compile_definitions(${MODULE_NAME} PUBLIC FFMPEG_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC ${FFMPEG_LIB_TARGETS})
endif ()

if (${ENABLE_NASTRAN_MODULE})
    target_compile_definitions(${MODULE_NAME} PUBLIC NASTRAN_ENABLE)

endif ()

#if(Python3_FOUND)
#    target_include_directories(${MODULE_NAME} PUBLIC
#        ${Python3_INCLUDE_DIRS}
#        ${pybind11_INCLUDE_DIRS}
#    )
#    # 添加编译定义
#    add_compile_definitions(PYNASTRAN_SCRIPT_PATH="${PYNASTRAN_SCRIPT_DIR}")
#
#    message(STATUS "Python集成已启用")
#    message(STATUS "  - Python Include: ${Python3_INCLUDE_DIRS}")
#    message(STATUS "  - pybind11 Include: ${pybind11_INCLUDE_DIRS}")
#    message(STATUS "  - Python Scripts: ${PYNASTRAN_SCRIPT_DIR}")
#endif()

if (ENABLE_GPSCUDA_MODULE)
    target_compile_definitions(${MODULE_NAME} PUBLIC GPSCUDA_ENABLE)
    message("Enable GPSCuda Module.")
    target_include_directories(${MODULE_NAME} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/extern>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/extern/wmma_extension/include>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/extern/cutlass/include>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/extern/cutlass/tools/util/include>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/extern/oneTBB-2021.10.0/include>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/extern/glm>
            $<INSTALL_INTERFACE:include/ThirdParty/CMakeGPS>
    )

    target_link_libraries(${MODULE_NAME} PRIVATE
            CommonLib
            Geom
            GPHelperIO
            GPSplineLib
    )

    file(GLOB_RECURSE EXDLL ${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/ReferenceDLL/*.dll)
    add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${EXDLL}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif ()

#--------------------------------------------------------------------
# Target link Module needed ThirdParty's lib
#--------------------------------------------------------------------
target_link_libraries(${MODULE_NAME} PUBLIC ${ThirdParty_lib_dependency})

if (WIN32)
    #  内部库在cmake构建时定义IGAME_COMPILE_EXPORT，使得模式切换为dllexport, 否则外部库在使用编译这个动态库代码时将其模式切换为dllimport
    target_compile_definitions(${MODULE_NAME} PRIVATE IGAME_COMPILE_EXPORT)
    #  由于Windows不会搜索其他目录下的dll文件因此要使用cmake指令将dll移至exe目录
    #  将第三方模块编译成的可执行文件(.exe)、静态库(.lib)、动态库(.dll)的输出移至放入主程序源码目录
    set_property(TARGET ${MODULE_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
endif ()

#--------------------------------------------------------------------
# Exec Install iGameVis Core Module's CMake
#--------------------------------------------------------------------
include(Cmake/CoreModuleInstall.cmake)
#--------------------------------------------------------------------

if (ENABLE_LIBTORCH_MODULE)
    set(RESOURCES_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources)
    set(RESOURCES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Resources)
    file(MAKE_DIRECTORY ${RESOURCES_OUTPUT_DIR})
    file(COPY ${RESOURCES_SOURCE_DIR}/ DESTINATION ${RESOURCES_OUTPUT_DIR})
endif ()
