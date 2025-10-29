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

# Add an option that the user can set with the -D parameter

#--------------------------------------------------------------------
# if need to Enable remote ThirdParty module. Follow this step:
# 1. use git command1(git submodule init) and command2(git submodule update)
# to clone remote Submodule which are defined in .gitmodules file.
# 2. Switch OFF to On.
#--------------------------------------------------------------------

#option(ENABLE_CGNS_MODULE "Enable CGNS ThirdParty Module" ON)
set(ENABLE_CGNS_MODULE ON CACHE BOOL "Enable CGNS ThirdParty Module" FORCE)

set(ENABLE_NASTRAN_MODULE OFF CACHE BOOL "Enable Nastran Lib ThirdParty Module" FORCE)

#option(ENABLE_CGNS_MODULE "Enable Libtorch ThirdParty Module" ON)
set(ENABLE_LIBTORCH_MODULE OFF CACHE BOOL "Enable Libtorch ThirdParty Module" FORCE)

# This module is temporarily only available on Windows Platform.
set(ENABLE_GPSCUDA_MODULE OFF CACHE BOOL "Enable GPS CUDA Module. " FORCE)

#--------------------------------------------------------------------
# If external application want to use Core Module lib.
# Set Core Module install Option to ON
#--------------------------------------------------------------------
set(CORE_MODULE_INSTALL ON CACHE BOOL "Enable OpeniGame Core Module to install as lib or not." FORCE)
set(EXAMPLE_COMPILE OFF CACHE BOOL "Enable OpeniGame Core Module's Example Compilation." FORCE)
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
        # Or for a specific target:
        # target_compile_definitions(MyExecutable PRIVATE -D_WINDOWS_SOURCE)
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

add_executable(${PROJECT_NAME} main.cpp)

set(IGAMEVIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

### ---- 运行时 DLL 拷贝（Windows）----------------------------------------
if (WIN32 AND ENABLE_LIBTORCH_MODULE)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LIBTORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/libtorch/Debug")
    else ()
        set(LIBTORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/libtorch/Release")
    endif ()

    include_directories(
            "${LIBTORCH_DIR}/include"
            "${LIBTORCH_DIR}/include/torch/csrc/api/include"
    )

    file(GLOB TORCH_DLLS "${LIBTORCH_DIR}/lib/*.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TORCH_DLLS}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying libtorch DLLs to output folder"
    )
elseif (ENABLE_LIBTORCH_MODULE)
    message(FATAL_ERROR "Only Windows platform is supported for this project! LibTorch DLL copy is defined for Windows only.")
endif ()

#--------------------------------------------------------------------
# Add subdirectories
#--------------------------------------------------------------------
#导入第三方组件
if (ENABLE_GPSCUDA_MODULE)
    add_subdirectory(CMakeGPS)
endif ()
add_subdirectory(ThirdParty)
#导入iGame核心后处理组件
add_subdirectory(iGameCore)
target_link_libraries(${PROJECT_NAME} PUBLIC iGameCore_module)
#导入Qt模块
add_subdirectory(Qt)
target_link_libraries(${PROJECT_NAME} PUBLIC Qt_module)


if (EXAMPLE_COMPILE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_subdirectory(Examples)
endif ()

# 由于Windows不会搜索其他目录下的dll文件因此要使用cmake指令将dll移至exe目录
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${DEPLOYQT_EXECUTABLE} "$<TARGET_FILE:${PROJECT_NAME}>")
endif ()

# 编译时将默认的MSVC运行时库静态链接，用于exe的打包
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

# If user requests GPSCUDA, check nvcc version. For Release builds where nvcc is missing
# or too old, automatically disable GPSCUDA so that Release configure/build can proceed.
if (ENABLE_GPSCUDA_MODULE)
    find_program(NVCC_EXECUTABLE nvcc)
    if (NOT NVCC_EXECUTABLE)
        set(NVCC_FOUND FALSE)
    else ()
        set(NVCC_FOUND TRUE)
        execute_process(COMMAND ${NVCC_EXECUTABLE} --version
                OUTPUT_VARIABLE NVCC_OUTPUT
                ERROR_VARIABLE NVCC_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(REGEX MATCH "release ([0-9]+)\\.([0-9]+)" _match "${NVCC_OUTPUT}")
        if (_match)
            string(REGEX REPLACE ".*release ([0-9]+)\\.([0-9]+).*" "\\1" NVCC_MAJOR "${NVCC_OUTPUT}")
            string(REGEX REPLACE ".*release ([0-9]+)\\.([0-9]+).*" "\\2" NVCC_MINOR "${NVCC_OUTPUT}")
            math(EXPR NVCC_VER_INT "${NVCC_MAJOR} * 100 + ${NVCC_MINOR}")
        else ()
            set(NVCC_VER_INT 0)
        endif ()
    endif ()

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        if (NOT NVCC_FOUND)
            message(WARNING "ENABLE_GPSCUDA_MODULE=ON but nvcc not found. Disabling GPSCUDA for Release configure to allow build to continue. Install CUDA >=12.4 to re-enable GPU support.")
            set(ENABLE_GPSCUDA_MODULE OFF CACHE BOOL "Enable GPS CUDA Module. " FORCE)
        elseif (NVCC_VER_INT LESS 1204)
            message(WARNING "Detected nvcc version ${NVCC_MAJOR}.${NVCC_MINOR} which is older than required 12.4. Disabling GPSCUDA for Release configure. Install CUDA >=12.4 to enable GPU support.")
            set(ENABLE_GPSCUDA_MODULE OFF CACHE BOOL "Enable GPS CUDA Module. " FORCE)
        endif ()
    else ()
        # For non-Release builds, just print detection info; keep the user's choice
        if (NOT NVCC_FOUND)
            message(STATUS "GPSCUDA requested but nvcc not found. Debug builds may still succeed without GPU.")
        elseif (NVCC_VER_INT LESS 1204)
            message(STATUS "GPSCUDA requested but detected nvcc ${NVCC_MAJOR}.${NVCC_MINOR}; CUDA >=12.4 is recommended.")
        else ()
            message(STATUS "Found nvcc ${NVCC_MAJOR}.${NVCC_MINOR} - GPU support enabled")
        endif ()
    endif ()
endif ()
