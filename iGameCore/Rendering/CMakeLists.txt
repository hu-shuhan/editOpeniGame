set(TARGET_NAME ${SHADER_COMPILE_TARGET})

# compile shaders
#set(SHADER_COMPILE_TARGET iGameVisShaderCompile)
# add_subdirectory(Shaders)
#add_dependencies(${MODULE_NAME} ${SHADER_COMPILE_TARGET})
#target_include_directories(
#        ${MODULE_NAME}
#        PUBLIC $<BUILD_INTERFACE:${IGAMEVIS_CORE_ROOT_DIR}/Rendering/Shaders/generated/cpp>)

# copy resources to the build directory
set(RENDERING_ASSESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Assests)
if (EMSCRIPTEN)
    set(RENDERING_SHADERS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/GLSL_Web)
else ()
    set(RENDERING_SHADERS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/GLSL)
endif ()
set(RENDERING_RESOURCES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Resources)
file(MAKE_DIRECTORY ${RENDERING_RESOURCES_OUTPUT_DIR})

file(COPY ${RENDERING_ASSESTS_SOURCE_DIR}/ DESTINATION ${RENDERING_RESOURCES_OUTPUT_DIR}/Assests)
file(COPY ${RENDERING_SHADERS_SOURCE_DIR}/ DESTINATION ${RENDERING_RESOURCES_OUTPUT_DIR}/Shaders)

# add definition
if (IGAME_RENDER_BACKEND STREQUAL "GLES2")
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_GLES2)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_330)
    if (EMSCRIPTEN)
        target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_PLATFORM_WEB)
    endif ()
elseif (WIN32)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_460)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_PLATFORM_WINDOWS)
elseif (UNIX)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_460)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_PLATFORM_LINUX)
else ()
    message(FATAL_ERROR "Unknown Platform")
endif ()
