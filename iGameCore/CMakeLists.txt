set(MODULE_NAME iGameCore_module)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DPLATFORM_WINDOWS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DPLATFORM_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(-DPLATFORM_MAC)
endif ()

#set(SHADER_COMPILE_TARGET OpeniGameShaderCompile)
#add_subdirectory(Rendering/Shaders)

file(GLOB_RECURSE IGAME_CORE_FILE CONFIGURE_DEPENDS ./*.h ./*.cpp)
add_library(${MODULE_NAME} STATIC ${IGAME_CORE_FILE})

target_include_directories(${MODULE_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/CellModel
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Common
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/DataModel
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/FilterModel
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Mapper
        ${CMAKE_CURRENT_SOURCE_DIR}/IO
        ${CMAKE_CURRENT_SOURCE_DIR}/Filters
        # 删除下面
        ${CMAKE_CURRENT_SOURCE_DIR}/Filters/*

        ${CMAKE_CURRENT_SOURCE_DIR}/Rendering
        ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Common
        ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Core/Interactor
        ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Core/Painter
        ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/OpenGL
)
target_link_libraries(${MODULE_NAME} PUBLIC
        glad
        freetype
        eigen
        meshoptimizer
        tinyXML2
)

if (ENABLE_CGNS_MODULE)
    target_compile_definitions(${MODULE_NAME} PRIVATE CGNS_ENABLE)
    target_link_libraries(${MODULE_NAME} PUBLIC cgns_static)
ENDIF ()

if(ENABLE_ABAQUS_MODULE)
    find_package(AbqSDK)
    if (${AbqSDK_FOUND})
        message("Find abqsdk")
        target_compile_definitions(${MODULE_NAME} PRIVATE AbqSDK_ENABLE)
        target_link_libraries(${MODULE_NAME} PUBLIC ${ABQ_LIB_TARGETS})
    endif ()
endif ()

target_compile_definitions(${MODULE_NAME} PRIVATE ASSESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Assests")
target_compile_definitions(${MODULE_NAME} PRIVATE SHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Shaders")

IF (WIN32)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_460)
ELSEIF (APPLE)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_330)
ELSEIF (UNIX)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_330)
ENDIF ()


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

target_link_libraries(${PROJECT_NAME} PUBLIC ${MODULE_NAME})
