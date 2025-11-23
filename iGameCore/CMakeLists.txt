set(MODULE_NAME iGameCore)
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

if (UNIX AND NOT APPLE)
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
        libzstd_static
        glfw
        spdlog_header_only
        zlibstatic
        nanoflann
)

#--------------------------------------------------------------------
# Find or Enable ThirdParty Components' Stuff.
#--------------------------------------------------------------------
if (ENABLE_CGNS_MODULE)
    target_compile_definitions(${MODULE_NAME} PUBLIC CGNS_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC cgns_static)
    # On Windows we still allow user to specify HDF5_DIR; on Linux CGNS already set up HDF5
    if (WIN32)
        set(HDF5_DIR "C:/Program Files/HDF_Group/HDF5/1.13.0/share/cmake")
        find_package(HDF5) # Optional: additional HDF5 linkage for Windows deployment
    endif ()
endif ()

if (ENABLE_LIBTORCH_MODULE)
    list(APPEND CMAKE_PREFIX_PATH "${LIBTORCH_DIR}/share/cmake/Torch")
    find_package(Torch REQUIRED)
    target_compile_definitions(${MODULE_NAME} PUBLIC LibTorch_ENABLE)
    target_include_directories(${MODULE_NAME} PRIVATE ${TORCH_INCLUDE_DIRS})
    target_link_libraries(${MODULE_NAME} PRIVATE ${TORCH_LIBRARIES})
    if (TARGET torch_cuda)
        message(STATUS "Found torch_cuda target (CUDA-enabled LibTorch).")
    else ()
        message(WARNING "This LibTorch package looks CPU-only (no torch_cuda target).")
    endif ()
    #        get_target_property(_torch_imps torch IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE)
    #        message(STATUS "Torch IMPORTED_LINK_INTERFACE (Release): ${_torch_imps}")

    if (TARGET torch_cuda)
        message(STATUS "Found torch_cuda target (CUDA-enabled LibTorch).")
    else ()
        message(WARNING "This LibTorch package looks CPU-only (no torch_cuda target).")
    endif ()
endif ()


find_package(AbqSDK)
if (${AbqSDK_FOUND})
    message("Find abqsdk")
    target_compile_definitions(${MODULE_NAME} PUBLIC AbqSDK_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC ${ABQ_LIB_TARGETS})
endif ()

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
include(Cmake/iGameCoreInstall.cmake)
#--------------------------------------------------------------------

#if (ENABLE_LIBTORCH_MODULE)
#    set(RESOURCES_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources)
#    set(RESOURCES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Resources)
#    file(MAKE_DIRECTORY ${RESOURCES_OUTPUT_DIR})
#    file(COPY ${RESOURCES_SOURCE_DIR}/ DESTINATION ${RESOURCES_OUTPUT_DIR})
#endif ()

if (ENABLE_LIBTORCH_MODULE)
    set(RESOURCES_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources)
    set(RESOURCES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Resources)
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
