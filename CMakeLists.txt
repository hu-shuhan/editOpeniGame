cmake_minimum_required(VERSION 3.19)
#--------------------------------------------------------------------
# Set Cmake Option to On/OFF.
#--------------------------------------------------------------------

# Add an option that the user can set with the -D parameter

#--------------------------------------------------------------------
# if need to Enable remote ThirdParty module. Follow this step:
# 1. use git command1(git submodule init) and command2(git submodule update)
# to clone remote Submodule which are defined in .gitmodules file.
# 2. Switch OFF to On.
#--------------------------------------------------------------------

#option(ENABLE_CGNS_MODULE "Enable CGNS ThirdParty Module" ON)
set(ENABLE_CGNS_MODULE OFF CACHE BOOL "Enable CGNS ThirdParty Module" FORCE)

# This module is temporarily only available on Windows Platform.
set(ENABLE_GPSCUDA_MODULE OFF CACHE BOOL "Enable GPS CUDA Module. " FORCE)

#--------------------------------------------------------------------
# If external application want to use Core Module lib.
# Set Core Module install Option to ON
#--------------------------------------------------------------------
set(CORE_MODULE_INSTALL OFF CACHE BOOL "Enable OpeniGame Core Module to install as lib or not." FORCE)
set(EXAMPLE_COMPILE OFF CACHE BOOL "Enable OpeniGame Core Module's Example Compilation." FORCE)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install path prefix" FORCE)


# If the cuda module is disable. Enable the compile option to suppress C4819 Warning.
if (NOT ENABLE_GPSCUDA_MODULE)
    project(iGameVis LANGUAGES C CXX)
    if (WIN32)
        add_definitions(-D_WINDOWS_SOURCE)
        # Or for a specific target:
        # target_compile_definitions(MyExecutable PRIVATE -CUDA_NVCCD_WINDOWS_SOURCE)
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

#if(EXAMPLE_COMPILE )
if (EXAMPLE_COMPILE)
    add_subdirectory(Examples)
endif ()

# 由于Windows不会搜索其他目录下的dll文件因此要使用cmake指令将dll移至exe目录
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${DEPLOYQT_EXECUTABLE} "$<TARGET_FILE:${PROJECT_NAME}>")
endif ()
#   编译时将默认的MSVC运行时库静态链接，用于exe的打包
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()