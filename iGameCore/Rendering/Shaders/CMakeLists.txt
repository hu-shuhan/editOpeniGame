set(TARGET_NAME ${SHADER_COMPILE_TARGET})

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.comp"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.geom"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.tesc"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.tese"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.mesh"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.task"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.rgen"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.rchit"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.rmiss"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.rcall")

SOURCE_GROUP("shaders" FILES ${SHADER_FILES})

set(SHADER_INCLUDE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/GLSL/include)

set(GENERATED_SHADER_FOLDER "generated")
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER})
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER})
endif ()

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}/spv)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}/spv)
endif ()

include(${IGAMEVIS_ROOT_DIR}/Cmake/ShaderCompile.cmake)

if (WIN32)
    set(glslangValidator_executable ${CMAKE_CURRENT_SOURCE_DIR}/GLSLangTool/Win32/glslangValidator.exe)
elseif (UNIX)
    if (APPLE)
        set(glslangValidator_executable ${CMAKE_CURRENT_SOURCE_DIR}/GLSLangTool/MacOS/glslangValidator)
    else ()
        set(glslangValidator_executable ${CMAKE_CURRENT_SOURCE_DIR}/GLSLangTool/Linux/glslangValidator)
    endif ()
else ()
    message(FATAL_ERROR "Unknown Platform")
endif ()

compile_shader(
        "${SHADER_FILES}"
        "${TARGET_NAME}"
        "${SHADER_INCLUDE_FOLDER}"
        "${GENERATED_SHADER_FOLDER}"
        "${glslangValidator_executable}")

set_target_properties("${TARGET_NAME}" PROPERTIES FOLDER "iGameVis")