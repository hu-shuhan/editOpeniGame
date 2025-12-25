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

# 排除 test_pybind11.cpp，避免被编译进 iGameCore 库
list(FILTER IGAME_CORE_FILE EXCLUDE REGEX ".*test_pybind11\\.cpp$")

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
        metis
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
        find_package(HDF5 REQUIRED) # Optional: additional HDF5 linkage for Windows deployment
    endif ()
endif()

if(TARGET pybind11::module)
    message(STATUS "pybind11 integrated into iGameCore")
    
    # 从 iGameCore 的源文件列表（如果使用了通配符）中排除 test_pybind11.cpp
    # 注意：我们应该在定义 iGameCore 目标之前或在 GLOB 中排除它
    # 但由于 iGameCore 目标已经定义，我们这里只需要确保 pybind11_add_module 能正确编译它
    
    # 示例：构建一个名为 test_pybind11 的 Python 扩展模块
    # 注意：这里我们假设 test_pybind11.cpp 位于 Core 目录下
    pybind11_add_module(test_pybind11 Core/test_pybind11.cpp)
    
    # 显式取消 HEADER_FILE_ONLY 属性，以防万一
    set_source_files_properties(Core/test_pybind11.cpp PROPERTIES HEADER_FILE_ONLY FALSE)
    
    # 链接 iGameCore 库，以便在绑定中使用 iGame 的类和函数
    target_link_libraries(test_pybind11 PRIVATE iGameCore)
    
    # 可选：设置输出目录，方便测试
    set_target_properties(test_pybind11 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python_modules)
endif()

if (ENABLE_LIBTORCH_MODULE)
    target_compile_definitions(${MODULE_NAME} PUBLIC LibTorch_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC ${TORCH_LIBRARIES})
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
