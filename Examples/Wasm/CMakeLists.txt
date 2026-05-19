cmake_minimum_required(VERSION 3.19)

project(iGameVisWasmDemo LANGUAGES C CXX)

if (NOT EMSCRIPTEN)
    message(FATAL_ERROR "Examples/Wasm must be built with Emscripten")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(iGameWasmDemo main_wasm.cpp)

target_link_libraries(iGameWasmDemo PUBLIC iGameCore)

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
    message(STATUS "IGAME_WASM_MEMORY_PROFILE=memory64-16g is experimental and runtime-dependent")
endif ()

target_link_options(iGameWasmDemo PRIVATE
    "SHELL:-s WASM=1"
    "SHELL:-s USE_GLFW=3"
    "SHELL:-s FULL_ES2=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    ${IGAME_WASM_MEMORY_LINK_OPTIONS}
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','HEAPU8']"
    "SHELL:-s EXPORTED_FUNCTIONS=['_malloc','_free','_igw_init','_igw_load_vtk_from_mem','_igw_load_vtk_from_mem_ex','_igw_load_vtu_from_mem','_igw_load_vtu_from_mem_ex','_igw_get_model_list_json','_igw_set_active_model','_igw_set_model_visibility','_igw_remove_model','_igw_remove_all_models','_igw_render_frame','_igw_reset_camera','_igw_send_mouse_event','_igw_set_view_style','_igw_set_scalar_field','_igw_set_surface_shading_mode','_igw_set_background_color','_igw_get_attribute_list_json']"
    "SHELL:-s FORCE_FILESYSTEM=1"
    "SHELL:-s ASSERTIONS=1"
)

set_target_properties(iGameWasmDemo PROPERTIES
    OUTPUT_NAME "iGameWasmDemo"
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/index.html ${CMAKE_CURRENT_BINARY_DIR}/index.html COPYONLY)

add_custom_command(TARGET iGameWasmDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/index.html
            ${CMAKE_CURRENT_BINARY_DIR}/index.html
)
