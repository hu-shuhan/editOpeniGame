cmake_minimum_required(VERSION 3.19)

#--------------------------------------------------------------------
# CMAKE Global Settings.
#--------------------------------------------------------------------

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(IGAME_BUILD_SHARED OFF CACHE BOOL "Build a shared version of the library")

if (IGAME_BUILD_SHARED AND UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "-fPIC")
endif ()

#--------------------------------------------------------------------
# Set Cmake Option to On/OFF
#--------------------------------------------------------------------

# Add options that the user can set with the -D parameter
# These options must NOT be forced so that command-line -D values work.
option(ENABLE_CGNS_MODULE "Enable CGNS ThirdParty Module" OFF)
option(ENABLE_NASTRAN_MODULE "Enable Nastran Lib ThirdParty Module" OFF)
option(ENABLE_LIBTORCH_MODULE "Enable Libtorch ThirdParty Module" OFF)
option(ENABLE_GPSCUDA_MODULE "Enable GPS CUDA Module." OFF)
option(ENABLE_QT_MODULE "Build GUI/Qt Module." ON)

#--------------------------------------------------------------------
# If external application want to use Core Module lib.
# Set Core Module install Option to ON
#--------------------------------------------------------------------
set(CORE_MODULE_INSTALL ON CACHE BOOL "Enable OpeniGame Core Module to install as lib or not." FORCE)
set(EXAMPLE_COMPILE ON CACHE BOOL "Enable OpeniGame Core Module's Example Compilation." FORCE)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install path prefix" FORCE)

# 如果检测到 Anaconda 环境，尝试忽略其路径
if (DEFINED ENV{CONDA_PREFIX}) # 忽略 Anaconda bin 目录，防止其编译器或工具的干扰
    list(APPEND CMAKE_IGNORE_PATH "$ENV{CONDA_PREFIX}/bin") # 忽略 Anaconda 安装的库和配置文件的路径
    list(APPEND CMAKE_IGNORE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
endif ()

# If the cuda module is disable. Enable the compile option to suppress C4819 Warning.
if (NOT ENABLE_GPSCUDA_MODULE)
    project(iGameVis LANGUAGES C CXX)
    if (WIN32)
        add_definitions(-D_WINDOWS_SOURCE)
    endif ()
else ()
    project(iGameVis LANGUAGES C CXX CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_ARCHITECTURES "86")
    set(CMAKE_CUDA_STANDARD_REQUIRED TRUE)
endif ()

add_compile_options(
        "$<$<AND:$<C_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:C>>:/utf-8>"
        "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>"
)
add_definitions(/D_USE_MATH_DEFINES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# insert our Custom CMake macro path to CMake Module Path
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(IGAMEVIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

#--------------------------------------------------------------------
# Add subdirectories
#--------------------------------------------------------------------
# 导入第三方组件
if (ENABLE_GPSCUDA_MODULE)
    add_subdirectory(CMakeGPS)
endif ()
add_subdirectory(ThirdParty)

# 导入核心库
add_subdirectory(iGameCore)

# 导入Qt模块
if (ENABLE_QT_MODULE)
    add_subdirectory(Qt)

    add_executable(${PROJECT_NAME} main.cpp)
    target_link_libraries(${PROJECT_NAME} PUBLIC
            iGameCore
            Qt_module
    )

    # 由于Windows不会搜索其他目录下的dll文件因此要使用cmake指令将dll移至exe目录
    if (WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${DEPLOYQT_EXECUTABLE} "$<TARGET_FILE:${PROJECT_NAME}>")
        if (ENABLE_LIBTORCH_MODULE)
            if (CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(LIBTORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/libtorch/Windows/Debug")
            else ()
                set(LIBTORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/libtorch/Windows/Release_GPU")
            endif ()

            include_directories(
                    "${LIBTORCH_DIR}/include"
                    "${LIBTORCH_DIR}/include/torch/csrc/api/include"
            )
            file(GLOB TORCH_DLLS
                    "${LIBTORCH_DIR}/lib/*.dll"
                    "${LIBTORCH_DIR}/bin/*.dll")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${TORCH_DLLS}
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>
                    COMMENT "Copying libtorch DLLs to output folder"
            )
        elseif (UNIX AND NOT APPLE AND ENABLE_LIBTORCH_MODULE)
            if (CMAKE_BUILD_TYPE STREQUAL "Release")
                set(LIBTORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/libtorch/Linux/Release_GPU")
            endif ()
            include_directories(
                    "${LIBTORCH_DIR}/include"
                    "${LIBTORCH_DIR}/include/torch/csrc/api/include"
            )
            set_target_properties(${PROJECT_NAME} PROPERTIES
                    BUILD_RPATH "${LIBTORCH_DIR}/lib"
                    INSTALL_RPATH "${LIBTORCH_DIR}/lib"
            )

        elseif (ENABLE_LIBTORCH_MODULE)
            message(FATAL_ERROR "Only Windows and Linux platform is supported for this project!")
        endif ()
    endif ()
endif ()

# 导入example
if (EXAMPLE_COMPILE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_subdirectory(Examples)
endif ()
