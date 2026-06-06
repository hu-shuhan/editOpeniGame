cmake_minimum_required(VERSION 3.19)

project(iGameVisWasmDemo LANGUAGES C CXX)

if (NOT EMSCRIPTEN)
    message(FATAL_ERROR "Examples/Wasm must be built with Emscripten")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(iGameWeb main_wasm.cpp)

target_link_libraries(iGameWeb PUBLIC iGameCore)

set(IGAME_WASM_OUTPUT_NAME "iGameWeb")
set(IGAME_WASM_JS_NAME "iGameWeb.js")
set(IGAME_WASM_WASM_NAME "iGameWeb.wasm")

set(IGAME_WASM_MEMORY_PROFILE "stable" CACHE STRING "Wasm memory profile: stable, aggressive, max4g, or memory64-16g")
set_property(CACHE IGAME_WASM_MEMORY_PROFILE PROPERTY STRINGS stable aggressive max4g memory64-16g)

set(IGAME_WASM_MEMORY_LINK_OPTIONS
    "SHELL:-s INITIAL_MEMORY=805306368"
    "SHELL:-s MAXIMUM_MEMORY=2147483648"
    "SHELL:-s STACK_SIZE=1048576"
    "SHELL:-s MALLOC=dlmalloc"
)

if (IGAME_WASM_MEMORY_PROFILE STREQUAL "aggressive")
    set(IGAME_WASM_MEMORY_LINK_OPTIONS
        "SHELL:-s INITIAL_MEMORY=1073741824"
        "SHELL:-s MAXIMUM_MEMORY=4294967296"
        "SHELL:-s STACK_SIZE=1048576"
        "SHELL:-s MALLOC=dlmalloc"
    )
elseif (IGAME_WASM_MEMORY_PROFILE STREQUAL "max4g")
    set(IGAME_WASM_MEMORY_LINK_OPTIONS
        "SHELL:-s INITIAL_MEMORY=1073741824"
        "SHELL:-s MAXIMUM_MEMORY=4294967296"
        "SHELL:-s STACK_SIZE=1048576"
        "SHELL:-s MALLOC=dlmalloc"
    )
elseif (IGAME_WASM_MEMORY_PROFILE STREQUAL "memory64-16g")
    set(IGAME_WASM_MEMORY_LINK_OPTIONS
        "SHELL:-s INITIAL_MEMORY=2147483648"
        "SHELL:-s MAXIMUM_MEMORY=17179869184"
        "SHELL:-s STACK_SIZE=1048576"
        "SHELL:-s MALLOC=dlmalloc"
        "SHELL:-s MEMORY64=1"
    )
    set(IGAME_WASM_OUTPUT_NAME "iGameWeb_64")
    set(IGAME_WASM_JS_NAME "iGameWeb_64.js")
    set(IGAME_WASM_WASM_NAME "iGameWeb_64.wasm")
    message(STATUS "IGAME_WASM_MEMORY_PROFILE=memory64-16g is experimental and runtime-dependent")
endif ()

target_link_options(iGameWeb PRIVATE
    "SHELL:-s WASM=1"
    "SHELL:-s USE_GLFW=3"
    "SHELL:-s FULL_ES2=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-lembind"
    ${IGAME_WASM_MEMORY_LINK_OPTIONS}
    "SHELL:-s FORCE_FILESYSTEM=1"
    "SHELL:-s ASSERTIONS=1"
    "SHELL:--embed-file ${CMAKE_SOURCE_DIR}/iGameCore/Rendering/Shaders/GLSL_Web@/Resources/Shaders"
    "SHELL:--embed-file ${CMAKE_SOURCE_DIR}/iGameCore/Rendering/Assests/Fonts/SourceHanSansCN-Normal.otf@/Resources/Assests/Fonts/SourceHanSansCN-Normal.otf"
)

set_target_properties(iGameWeb PROPERTIES
    OUTPUT_NAME "${IGAME_WASM_OUTPUT_NAME}"
)

if (IGAME_WASM_MEMORY_PROFILE STREQUAL "memory64-16g")
    add_custom_command(TARGET iGameWeb POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -f
                $<TARGET_FILE_DIR:iGameWeb>/iGameWeb.js
        COMMAND ${CMAKE_COMMAND} -E rm -f
                $<TARGET_FILE_DIR:iGameWeb>/iGameWeb.wasm
    )
endif ()

set(IGAME_WASM_HTML_ASSETS
    index.html
)

foreach (asset IN LISTS IGAME_WASM_HTML_ASSETS)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${asset}
                   ${CMAKE_CURRENT_BINARY_DIR}/${asset}
                   @ONLY)
endforeach ()
