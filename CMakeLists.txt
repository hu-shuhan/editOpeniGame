cmake_minimum_required(VERSION 3.19)
project(OpeniGame LANGUAGES C CXX)
add_definitions(/D_USE_MATH_DEFINES)
set(CMAKE_CXX_STANDARD 20)
if (WIN32)
    # Used to Clear C4819 Warning.
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_definitions(-D_WINDOWS_SOURCE)
    # Or for a specific target:
    # target_compile_definitions(MyExecutable PRIVATE -D_WINDOWS_SOURCE)
endif ()

# insert our Custom CMake macro path to CMake Module Path
list(INSERT CMAKE_MODULE_PATH 0
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


add_executable(${PROJECT_NAME}
        main.cpp
)

set(OPENIGAME_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

#--------------------------------------------------------------------
## Set Cmake Option to On/OFF.
##--------------------------------------------------------------------
#
## Add an option that the user can set with the -D parameter

# if need to Enable remote ThirdParty module. Follow this step:
# 1. use git command1(git submodule init) and command2(git submodule update) to clone remote Submodule which are defined in .gitmodules file
# 2. Set OFF to On.
#option(ENABLE_CGNS_MODULE "Enable CGNS ThirdParty Module" ON)
set(ENABLE_CGNS_MODULE OFF CACHE BOOL "Enable CGNS ThirdParty Module" FORCE)
set(ENABLE_ABAQUS_MODULE OFF CACHE BOOL "Enable ABAQUS SDK ThirdParty Module" FORCE)

#--------------------------------------------------------------------
# Add subdirectories
#--------------------------------------------------------------------
#导入第三方组件
add_subdirectory(ThirdParty)
#导入iGame核心后处理组件
add_subdirectory(iGameCore)
#导入Qt模块
add_subdirectory(Qt)

##   由于Windows不会搜索其他目录下的dll文件因此要使用cmake指令将dll移至exe目录
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${DEPLOYQT_EXECUTABLE} "$<TARGET_FILE:${PROJECT_NAME}>")
endif ()