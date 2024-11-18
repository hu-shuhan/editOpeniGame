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

# If the cuda module is disable. Enable the compile option to suppress C4819 Warning.
if (NOT ENABLE_GPSCUDA_MODULE)
    project(OpeniGame LANGUAGES C CXX)
    if (WIN32)
        add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
        add_definitions(-D_WINDOWS_SOURCE)
        # Or for a specific target:
        # target_compile_definitions(MyExecutable PRIVATE -D_WINDOWS_SOURCE)
    endif ()
else ()
    project(OpeniGame LANGUAGES C CXX CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_ARCHITECTURES "86")
    set(CMAKE_CUDA_STANDARD_REQUIRED TRUE)
endif ()

add_definitions(/D_USE_MATH_DEFINES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


# insert our Custom CMake macro path to CMake Module Path
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


add_executable(${PROJECT_NAME}
        main.cpp
        "ThirdParty/lzma/7z.h" "ThirdParty/lzma/7zAlloc.c" "ThirdParty/lzma/7zAlloc.h" "ThirdParty/lzma/7zArcIn.c" "ThirdParty/lzma/7zBuf.c" "ThirdParty/lzma/7zBuf.h" "ThirdParty/lzma/7zBuf2.c" "ThirdParty/lzma/7zCrc.c" "ThirdParty/lzma/7zCrc.h" "ThirdParty/lzma/7zCrcOpt.c" "ThirdParty/lzma/7zDec.c" "ThirdParty/lzma/7zFile.c" "ThirdParty/lzma/7zFile.h" "ThirdParty/lzma/7zStream.c" "ThirdParty/lzma/7zTypes.h" "ThirdParty/lzma/7zVersion.h" "ThirdParty/lzma/7zWindows.h" "ThirdParty/lzma/Aes.c" "ThirdParty/lzma/Aes.h" "ThirdParty/lzma/AesOpt.c" "ThirdParty/lzma/Alloc.c" "ThirdParty/lzma/Alloc.h" "ThirdParty/lzma/Bcj2.c" "ThirdParty/lzma/Bcj2.h" "ThirdParty/lzma/Bcj2Enc.c" "ThirdParty/lzma/Bra.c" "ThirdParty/lzma/Bra.h" "ThirdParty/lzma/Bra86.c" "ThirdParty/lzma/BraIA64.c" "ThirdParty/lzma/Compiler.h" "ThirdParty/lzma/CpuArch.c" "ThirdParty/lzma/CpuArch.h" "ThirdParty/lzma/Delta.c" "ThirdParty/lzma/Delta.h" "ThirdParty/lzma/DllSecur.c" "ThirdParty/lzma/DllSecur.h" "ThirdParty/lzma/LzFind.c" "ThirdParty/lzma/LzFind.h" "ThirdParty/lzma/LzFindMt.c" "ThirdParty/lzma/LzFindMt.h" "ThirdParty/lzma/LzFindOpt.c" "ThirdParty/lzma/LzHash.h" "ThirdParty/lzma/Lzma2Dec.c" "ThirdParty/lzma/Lzma2Dec.h" "ThirdParty/lzma/Lzma2DecMt.c" "ThirdParty/lzma/Lzma2DecMt.h" "ThirdParty/lzma/Lzma2Enc.c" "ThirdParty/lzma/Lzma2Enc.h" "ThirdParty/lzma/Lzma86.h" "ThirdParty/lzma/Lzma86Dec.c" "ThirdParty/lzma/Lzma86Enc.c" "ThirdParty/lzma/LzmaDec.c" "ThirdParty/lzma/LzmaDec.h" "ThirdParty/lzma/LzmaEnc.c" "ThirdParty/lzma/LzmaEnc.h" "ThirdParty/lzma/LzmaLib.c" "ThirdParty/lzma/LzmaLib.h" "ThirdParty/lzma/MtCoder.c" "ThirdParty/lzma/MtCoder.h" "ThirdParty/lzma/MtDec.c" "ThirdParty/lzma/MtDec.h" "ThirdParty/lzma/Ppmd.h" "ThirdParty/lzma/Ppmd7.c" "ThirdParty/lzma/Ppmd7.h" "ThirdParty/lzma/Ppmd7Dec.c" "ThirdParty/lzma/Ppmd7Enc.c" "ThirdParty/lzma/Precomp.h" "ThirdParty/lzma/RotateDefs.h" "ThirdParty/lzma/Sha256.c" "ThirdParty/lzma/Sha256.h" "ThirdParty/lzma/Sha256Opt.c" "ThirdParty/lzma/Sort.c" "ThirdParty/lzma/Sort.h" "ThirdParty/lzma/SwapBytes.c" "ThirdParty/lzma/SwapBytes.h" "ThirdParty/lzma/Threads.c" "ThirdParty/lzma/Threads.h" "ThirdParty/lzma/Xz.c" "ThirdParty/lzma/Xz.h" "ThirdParty/lzma/XzCrc64.c" "ThirdParty/lzma/XzCrc64.h" "ThirdParty/lzma/XzCrc64Opt.c" "ThirdParty/lzma/XzDec.c" "ThirdParty/lzma/XzEnc.c" "ThirdParty/lzma/XzEnc.h" "ThirdParty/lzma/XzIn.c" "ThirdParty/lzma/Util/7z/7zMain.c" "ThirdParty/lzma/Util/7z/Precomp.c" "ThirdParty/lzma/Util/7z/Precomp.h" "ThirdParty/lzma/Util/Lzma/LzmaUtil.c" "ThirdParty/lzma/Util/Lzma/Precomp.h" "ThirdParty/lzma/Util/LzmaLib/LzmaLibExports.c" "ThirdParty/lzma/Util/LzmaLib/Precomp.c" "ThirdParty/lzma/Util/LzmaLib/Precomp.h" "ThirdParty/lzma/Util/SfxSetup/Precomp.c" "ThirdParty/lzma/Util/SfxSetup/Precomp.h" "ThirdParty/lzma/Util/SfxSetup/SfxSetup.c")

set(OPENIGAME_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")


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

#   由于Windows不会搜索其他目录下的dll文件因此要使用cmake指令将dll移至exe目录
#if (WIN32)
#    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#            COMMAND ${DEPLOYQT_EXECUTABLE} "$<TARGET_FILE:${PROJECT_NAME}>")
#endif ()