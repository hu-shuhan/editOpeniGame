set(MODULE_NAME iGameCore)
set(CMAKE_CXX_STANDARD 20)
#=====添加路径======
set(Qt5_DIR "D:/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5")
#=================
set(IGAMEVIS_CORE_ROOT_DIR ${IGAMEVIS_ROOT_DIR}/iGameCore)

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

# ENABLE_IGV_MODULE=OFF（以及 Emscripten）时，把 IGV 的实现文件从源列表里剔除。
#
# 为什么必须在这里剔，而不能只跳过 HDF5 链接：GLOB_RECURSE 不认识 option，
# iGameIGVWriter.cpp / iGameIGVReader.cpp 会被照常编译，而它们 `#include <hdf5.h>`
# —— OFF 构建会直接编译失败，门控就成了装饰（对照 kia/igv分阶段实现方案.md
# §11.6 第 5 条「ENABLE_IGV_MODULE=OFF 全量构建成功」与 P2-7）。
#
# 只排除 IO/IGV/ 下的实现文件；iGameIGVFormat.h 是纯常量头（零 HDF5 依赖），
# 与既有 CGNS 的处置一致：模块关闭时头文件仍在树上，但不参与编译。
if (NOT ENABLE_IGV_MODULE OR EMSCRIPTEN)
    list(FILTER IGAME_CORE_FILE EXCLUDE REGEX "/IO/IGV/")
endif ()

# Currently, only shared libraries for the Linux system are supported
if (IGAME_BUILD_SHARED AND UNIX AND NOT APPLE)
    add_library(${MODULE_NAME} SHARED ${IGAME_CORE_FILE})
else ()
    add_library(${MODULE_NAME} STATIC ${IGAME_CORE_FILE} "IO/Fluent/iGameCASReader.h" "Filters/StreamView/iGameStreamlineSimplifier.h" "Filters/StreamView/iGameStreamlineSimplifier.cpp")
endif ()

if (UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    find_package(OpenMP REQUIRED)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(${MODULE_NAME} PUBLIC OpenMP::OpenMP_CXX)
    endif ()
endif ()

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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/IO/IGV>
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
        $<INSTALL_INTERFACE:include/IO/IGV>
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
        freetype
        eigen
        meshoptimizer
        tinyXML2
        vtkkwiml
        libzstd_static
        spdlog_header_only
        zlibstatic
        cnpy
        nanoflann
)

if (NOT EMSCRIPTEN)
    list(APPEND ThirdParty_lib_dependency glad)
    list(APPEND ThirdParty_lib_dependency glfw)
endif ()

#--------------------------------------------------------------------
# Find or Enable ThirdParty Components' Stuff.
#--------------------------------------------------------------------
if (ENABLE_CGNS_MODULE)
    target_compile_definitions(${MODULE_NAME} PUBLIC CGNS_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC cgns_static)
    # On Windows we still allow user to specify HDF5_DIR; on Linux CGNS already set up HDF5
    #if (WIN32)
       # set(HDF5_DIR "C:/Program Files/HDF_Group/HDF5/1.13.0/share/cmake")
        #find_package(HDF5 REQUIRED) # Optional: additional HDF5 linkage for Windows deployment
    #endif ()
endif ()

#--------------------------------------------------------------------
# IGV (HDF5-based iGameVis universal format) —— 见 kia/igv分阶段实现方案.md §4.3
#
# 与 ENABLE_CGNS_MODULE 同构：编译宏 IGV_ENABLE + 链接 HDF5。
# 差异在于 IGV 的 HDF5 是**直接依赖**（CGNS 只是恰好也用到 HDF5）。
#
# 不能依赖 CGNS 带入的那份 HDF5：ENABLE_CGNS_MODULE=OFF 是本仓库所有 preset 的
# 默认，而 ENABLE_IGV_MODULE 默认 ON，所以 IGV 必须能独立定位 HDF5。
#
# ══ HDF5 查找：跨平台通用（Windows / Linux / macOS）══
#
# 只依赖 CMake 自带的标准入口，不针对任何发行版或安装包做特殊推断：
#
#   1. `find_package(HDF5 NAMES hdf5 COMPONENTS C)` —— **config 模式**。
#      (`NAMES` 一旦出现，FindHDF5 就改走 config 模式去找 `hdf5-config.cmake`。)
#      覆盖 HDF_Group 官方 Windows 包、Homebrew/macOS、Spack，以及任何用
#      `cmake --install` 装过 HDF5 的环境。位置由标准变量指定：
#      `HDF5_ROOT` / `HDF5_DIR` / `CMAKE_PREFIX_PATH`，或 `HDF5_HOME` 环境变量 ——
#      这些都是 CMake 与 HDF5 自己的约定，我们不必发明新变量。
#
#   2. 回落到 CMake 自带的 `FindHDF5` **Module 模式**。它是为 Linux 发行版
#      (libhdf5-dev) 与 macOS (Homebrew / MacPorts) 写的，是那些平台最可靠的路径。
#
#   3. 都失败 → `FATAL_ERROR`，并在信息里给出三大平台的安装提示。
#
# ══ 链接目标：按优先级取第一个"能提供 C API"的 ══
#
#   不同来源暴露的目标名不同，所以按优先级探测，而不是假设某一个：
#     HDF5::HDF5          ← Module 模式（各平台统一，优先）
#     hdf5::hdf5-shared   ← config 模式的共享变体（默认优先）
#     hdf5::hdf5-static   ← config 模式的静态变体
#     ${HDF5_LIBRARIES}   ← 老式 Module 模式的裸变量回退
#
#   ⚠ 实测踩坑（见 kia/P1实施记录.md §5.3）：config 包导出的静态/共享 target
#     名是**带命名空间**的（`hdf5::hdf5-static` / `hdf5::hdf5-shared`）。
#     先前写成不带命名空间的 `TARGET hdf5-static`，判断不命中 →
#     iGameCore "看起来配置成功但根本没链上 HDF5"，行为随后漂移到别处解析出的库。
#     这类"静默没链上"极难发现，所以下面**对最终是否拿到目标做了兜底报错**。
#
# ══ 静态 / 共享 ══
#
#   默认优先**共享**，理由是避开"某个预编译静态库带本机没有的工具链依赖"这类环境
#   问题（史上 HDF_Group 的 Windows 静态库曾由 Intel 编译器构建，MSVC 链接会报
#   LNK1104 'libircmt.lib'）。要静态可加 `-DIGV_HDF5_PREFER_STATIC=ON`。
#
#   另有一条纯 CMake 注意事项：**不要在 find_package 之前设
#   `HDF5_USE_STATIC_LIBRARIES`** —— 它在 config 模式下不生效（那是 HDF5 自带包
#   文件决定的），却会在 Module 模式下改变选型，从而造成"iGameCore 静态、
#   Examples 共享"这类混链。要控制变体，请用本文件自己的
#   `IGV_HDF5_PREFER_STATIC`。
#--------------------------------------------------------------------
if (ENABLE_IGV_MODULE AND NOT EMSCRIPTEN)
    # 不假定任何安装布局：只把 CMake / HDF5 的标准约定摆好。
    set(HDF5_PREFER_PARALLEL OFF CACHE BOOL "Prefer serial HDF5 (avoid mpi.h)" FORCE)
    option(IGV_HDF5_PREFER_STATIC
           "IGV: prefer the HDF5 static library over the shared one" OFF)

    # ---- 1) config 模式：HDF_Group / Homebrew / Spack / cmake --install ----
    find_package(HDF5 NAMES hdf5 COMPONENTS C QUIET)

    # ---- 2) Module 模式回退：Linux 发行版 / macOS Homebrew / MacPorts ----
    if (NOT HDF5_FOUND)
        find_package(HDF5 COMPONENTS C QUIET)
    endif ()

    # ---- 按优先级挑一个能提供 C API 的目标 ----
    set(_IGV_HDF5_TARGET "")
    set(_IGV_HDF5_VIA_VARS OFF)
    if (TARGET HDF5::HDF5)
        set(_IGV_HDF5_TARGET HDF5::HDF5)
    elseif (IGV_HDF5_PREFER_STATIC AND TARGET hdf5::hdf5-static)
        set(_IGV_HDF5_TARGET hdf5::hdf5-static)
    elseif (TARGET hdf5::hdf5-shared)
        set(_IGV_HDF5_TARGET hdf5::hdf5-shared)
    elseif (TARGET hdf5::hdf5-static)
        set(_IGV_HDF5_TARGET hdf5::hdf5-static)
    elseif (HDF5_LIBRARIES)
        set(_IGV_HDF5_VIA_VARS ON)
    endif ()

    if (NOT _IGV_HDF5_TARGET AND NOT _IGV_HDF5_VIA_VARS)
        message(FATAL_ERROR
            "IGV requires HDF5 (ENABLE_IGV_MODULE=ON) but CMake could not find it.\n"
            "  Windows : install the official HDF_Group HDF5 package, then pass\n"
            "            -DHDF5_ROOT=\"<install dir>\"   (or set HDF5_ROOT in the preset)\n"
            "  Linux   : sudo apt install libhdf5-dev        (Debian/Ubuntu)\n"
            "            sudo dnf install hdf5-devel          (Fedora/RHEL)\n"
            "  macOS   : brew install hdf5                  (or: sudo port install hdf5)\n"
            "  any     : set HDF5_ROOT / HDF5_DIR / CMAKE_PREFIX_PATH to your HDF5 install.\n"
            "  To skip IGV entirely, configure with -DENABLE_IGV_MODULE=OFF.")
    endif ()

    target_compile_definitions(${MODULE_NAME} PUBLIC IGV_ENABLE)

    if (_IGV_HDF5_TARGET)
        target_link_libraries(${MODULE_NAME} PUBLIC ${_IGV_HDF5_TARGET})
        message(STATUS "IGV: linking HDF5 target '${_IGV_HDF5_TARGET}' (HDF5 ${HDF5_VERSION})")

        # 静态 HDF5 通常需要 zlib；本工程已有 zlibstatic 目标。
        # 共享变体自带运行时依赖，不需要这一步。
        get_target_property(_IGV_HDF5_TYPE ${_IGV_HDF5_TARGET} TYPE)
        if (_IGV_HDF5_TYPE STREQUAL "STATIC_LIBRARY" AND TARGET zlibstatic)
            target_link_libraries(${MODULE_NAME} PUBLIC zlibstatic)
        endif ()
    else ()
        # 裸变量回退（老式 Module 模式只给变量）
        target_link_libraries(${MODULE_NAME} PUBLIC ${HDF5_LIBRARIES})
        if (HDF5_INCLUDE_DIRS)
            target_include_directories(${MODULE_NAME} PUBLIC ${HDF5_INCLUDE_DIRS})
        endif ()
        message(STATUS "IGV: linking HDF5 via variables ${HDF5_LIBRARIES} (HDF5 ${HDF5_VERSION})")
    endif ()
elseif (ENABLE_IGV_MODULE AND EMSCRIPTEN)
    message(STATUS "IGV module disabled on Emscripten (see kia/future-tasks.md)")
endif ()

if (ENABLE_LIBTORCH_MODULE)
    target_compile_definitions(${MODULE_NAME} PUBLIC LibTorch_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC ${TORCH_LIBRARIES})
endif ()

find_package(AbqSDK)
if (${AbqSDK_FOUND})
    message("Find abqsdk")
    target_compile_definitions(${MODULE_NAME} PUBLIC AbqSDK_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC ${ABQ_LIB_TARGETS})

    get_filename_component(_ABQ_SDK_VERSION
            "${AbqSDK_PRIVATE_PUBLIC_DIR}" NAME)
    set_property(SOURCE
            "${CMAKE_CURRENT_SOURCE_DIR}/IO/Abaqus/iGameODBReader.cpp"
            APPEND PROPERTY COMPILE_DEFINITIONS
            IGAME_ABAQUS_VERSION="${_ABQ_SDK_VERSION}")

    # Abaqus is an optional runtime integration.  A release built with the
    # SDK must still start on machines where Abaqus is not installed.  Delay
    # the Windows imports until an ODB file is actually opened; ODBReader then
    # locates and preloads the user's local Abaqus runtime.
    if (MSVC AND WIN32)
        target_link_libraries(${MODULE_NAME} PUBLIC delayimp)
        foreach (_ABQ_DELAYLOAD_LIBRARY IN LISTS ABQ_LIB_LIST)
            target_link_options(${MODULE_NAME} PUBLIC
                    "/DELAYLOAD:${_ABQ_DELAYLOAD_LIBRARY}.dll")
        endforeach ()
    endif ()

    # Abaqus 2024 has legacy diamond inheritance in odb_Repository. MSVC's
    # C++20 conformance mode diagnoses its generated copy constructor as
    # C2594, which is a compiler error and cannot be disabled with /wd2594.
    # Keep the compatibility mode local to the Abaqus translation unit.
    if (MSVC)
        set_property(SOURCE
                "${CMAKE_CURRENT_SOURCE_DIR}/IO/Abaqus/iGameODBReader.cpp"
                APPEND PROPERTY COMPILE_OPTIONS /permissive)
    endif ()

    if (WIN32 AND AbqSDK_DLL_DIR)
        if (ABQ_RUNTIME_LIB_LIST)
            string(JOIN "," IGAME_ABQ_RUNTIME_LIBRARY_NAMES
                    ${ABQ_RUNTIME_LIB_LIST})
        else ()
            string(JOIN "," IGAME_ABQ_RUNTIME_LIBRARY_NAMES ${ABQ_LIB_LIST})
        endif ()
        set(IGAME_ABQ_RUNTIME_DIR "${AbqSDK_DLL_DIR}" PARENT_SCOPE)
        set(IGAME_ABQ_RUNTIME_LIBRARY_NAMES
                "${IGAME_ABQ_RUNTIME_LIBRARY_NAMES}" PARENT_SCOPE)
    endif ()
endif ()

find_package(FFMPEG)
if (${FFMPEG_FOUND})
    message("Find FFMPEG")
    target_compile_definitions(${MODULE_NAME} PUBLIC FFMPEG_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC ${FFMPEG_LIB_TARGETS})
    if (WIN32 AND FFMPEG_DLL_DIR)
        if (ENABLE_QT_MODULE)
            set(_IGAME_DLL_OUTPUT_TARGET ${PROJECT_NAME})
        else ()
            set(_IGAME_DLL_OUTPUT_TARGET ${MODULE_NAME})
        endif ()
        file(GLOB FFMPEG_DLLS "${FFMPEG_DLL_DIR}/*.dll")
        add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${_IGAME_DLL_OUTPUT_TARGET}>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${FFMPEG_DLLS}
                "$<TARGET_FILE_DIR:${_IGAME_DLL_OUTPUT_TARGET}>"
                COMMENT "Copying FFMPEG DLLs to output folder")
    endif ()
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
    )

    target_link_libraries(${MODULE_NAME} PRIVATE
            CommonLib
            Geom
            GPHelperIO
            GPSplineLib
    )

    file(GLOB_RECURSE EXDLL ${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/ReferenceDLL/*.dll)
    if (ENABLE_QT_MODULE)
        set(_IGAME_DLL_OUTPUT_TARGET ${PROJECT_NAME})
    else ()
        set(_IGAME_DLL_OUTPUT_TARGET ${MODULE_NAME})
    endif ()
    add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${_IGAME_DLL_OUTPUT_TARGET}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${EXDLL}
            "$<TARGET_FILE_DIR:${_IGAME_DLL_OUTPUT_TARGET}>"
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
include(Cmake/iGameCoreInstall.cmake)
#--------------------------------------------------------------------

if (ENABLE_LIBTORCH_MODULE)
    set(RESOURCES_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources/AI)
    set(RESOURCES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Resources/AI)
    file(MAKE_DIRECTORY ${RESOURCES_OUTPUT_DIR})
    file(COPY ${RESOURCES_SOURCE_DIR}/ DESTINATION ${RESOURCES_OUTPUT_DIR})
    install(
            DIRECTORY ${RESOURCES_SOURCE_DIR}/
            DESTINATION Resources
            COMPONENT runtime
            FILES_MATCHING
            PATTERN "*"
            PATTERN ".git*" EXCLUDE
            PATTERN "__pycache__" EXCLUDE
            PATTERN "*.psd" EXCLUDE
    )
endif ()
