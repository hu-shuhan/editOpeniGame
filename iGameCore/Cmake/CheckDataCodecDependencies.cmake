function(igame_datacodec_collect_includes file_path output_variable)
    file(READ "${file_path}" file_content)
    string(REGEX MATCHALL "#[ \t]*include[ \t]*[<\"][^>\"]+[>\"]" include_lines "${file_content}")
    set(${output_variable} "${include_lines}" PARENT_SCOPE)
endfunction()

function(igame_datacodec_reject_includes root_path forbidden_pattern rule_name)
    if (NOT EXISTS "${root_path}")
        return()
    endif ()
    file(GLOB_RECURSE source_files CONFIGURE_DEPENDS
        "${root_path}/*.h"
        "${root_path}/*.hpp"
        "${root_path}/*.cpp")
    foreach (source_file IN LISTS source_files)
        if (ARGC GREATER 3)
            file(RELATIVE_PATH relative_source "${root_path}" "${source_file}")
            if (relative_source MATCHES "${ARGV3}")
                continue()
            endif ()
        endif ()
        igame_datacodec_collect_includes("${source_file}" include_lines)
        foreach (include_line IN LISTS include_lines)
            if (include_line MATCHES "${forbidden_pattern}")
                file(RELATIVE_PATH relative_file "${CMAKE_SOURCE_DIR}" "${source_file}")
                message(FATAL_ERROR
                    "DataCodec dependency rule '${rule_name}' failed: ${relative_file}: ${include_line}")
            endif ()
        endforeach ()
    endforeach ()
endfunction()

function(igame_check_datacodec_dependencies datacodec_root)
    igame_datacodec_reject_includes(
        "${datacodec_root}/Common"
        "DataCodec/(Workflow|Filter|Platform)/|(^|[/<\"])iGame[^/]*\\.h"
        "Common must remain host, platform and workflow independent")
    igame_datacodec_reject_includes(
        "${datacodec_root}/API"
        "DataCodec/Filter/|Examples/"
        "API must not depend on Filter or Examples")
    igame_datacodec_reject_includes(
        "${datacodec_root}/Platform/Wasm"
        "(^|[/<\"])iGame[^/]*\\.h|ModelSurface/|Rendering/|WebGL|GLES"
        "Platform/Wasm must not depend on iGame or rendering types")

    foreach (generic_module IN ITEMS Common API Codec Log Runtime Storage Validation Workflow)
        igame_datacodec_reject_includes(
            "${datacodec_root}/${generic_module}"
            "DataCodec/Filter/Wasm/|emscripten/|WebGL|GLES"
            "generic DataCodec modules must not depend on WASM host or browser types")
        igame_datacodec_reject_includes(
            "${datacodec_root}/${generic_module}"
            "DataCodec/Test/"
            "production DataCodec modules must not depend on Test")
    endforeach ()

    igame_datacodec_reject_includes(
        "${datacodec_root}/Filter"
        "DataCodec/Test/"
        "production Filter modules must not depend on Test"
        "^Test/")

    if (EXISTS "${CMAKE_SOURCE_DIR}/Examples/CMakeLists.txt")
        file(READ "${CMAKE_SOURCE_DIR}/Examples/CMakeLists.txt" examples_cmake)
        if (examples_cmake MATCHES "DataCodec/(Test|Filter/Test)/")
            message(FATAL_ERROR
                "DataCodec tests must not be enumerated by Examples/CMakeLists.txt")
        endif ()
    endif ()
endfunction()
