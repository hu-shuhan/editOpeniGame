cmake_minimum_required(VERSION 3.19)

foreach (_required_variable
        IGAME_RUNTIME_SEARCH_DIR
        IGAME_RUNTIME_DESTINATION
        IGAME_RUNTIME_LIBRARY_NAMES)
    if (NOT DEFINED ${_required_variable} OR "${${_required_variable}}" STREQUAL "")
        message(FATAL_ERROR "DeployAbqRuntime: ${_required_variable} is required")
    endif ()
endforeach ()

if (NOT IS_DIRECTORY "${IGAME_RUNTIME_SEARCH_DIR}")
    message(FATAL_ERROR
            "DeployAbqRuntime: Abaqus runtime directory does not exist: "
            "${IGAME_RUNTIME_SEARCH_DIR}")
endif ()

string(REPLACE "," ";" _runtime_library_names
        "${IGAME_RUNTIME_LIBRARY_NAMES}")

set(_runtime_roots)
foreach (_library_name IN LISTS _runtime_library_names)
    set(_runtime_dll "${IGAME_RUNTIME_SEARCH_DIR}/${_library_name}.dll")
    if (EXISTS "${_runtime_dll}")
        list(APPEND _runtime_roots "${_runtime_dll}")
    else ()
        message(WARNING "DeployAbqRuntime: missing Abaqus DLL: ${_runtime_dll}")
    endif ()
endforeach ()

if (NOT _runtime_roots)
    message(FATAL_ERROR "DeployAbqRuntime: no Abaqus runtime DLLs were found")
endif ()

file(GET_RUNTIME_DEPENDENCIES
        RESOLVED_DEPENDENCIES_VAR _resolved_dependencies
        UNRESOLVED_DEPENDENCIES_VAR _unresolved_dependencies
        LIBRARIES ${_runtime_roots}
        DIRECTORIES "${IGAME_RUNTIME_SEARCH_DIR}"
        PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
        POST_EXCLUDE_REGEXES
        ".*[\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\/]([Ss]ystem32|[Ss]ys[Ww][Oo][Ww]64)[\\/].*")

# Only deploy files supplied by this Abaqus installation. System and unrelated
# runtimes resolved through PATH must remain owned by their respective products.
file(REAL_PATH "${IGAME_RUNTIME_SEARCH_DIR}" _runtime_search_dir_real)
set(_runtime_files ${_runtime_roots})
foreach (_dependency IN LISTS _resolved_dependencies)
    get_filename_component(_dependency_dir "${_dependency}" DIRECTORY)
    file(REAL_PATH "${_dependency_dir}" _dependency_dir_real)
    if (_dependency_dir_real STREQUAL _runtime_search_dir_real)
        list(APPEND _runtime_files "${_dependency}")
    endif ()
endforeach ()
list(REMOVE_DUPLICATES _runtime_files)

file(MAKE_DIRECTORY "${IGAME_RUNTIME_DESTINATION}")
execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        ${_runtime_files}
        "${IGAME_RUNTIME_DESTINATION}"
        RESULT_VARIABLE _copy_result)
if (NOT _copy_result EQUAL 0)
    message(FATAL_ERROR "DeployAbqRuntime: copying DLLs failed (${_copy_result})")
endif ()

list(LENGTH _runtime_files _runtime_file_count)
message(STATUS
        "Deployed ${_runtime_file_count} Abaqus runtime DLLs to "
        "${IGAME_RUNTIME_DESTINATION}")

if (_unresolved_dependencies)
    list(JOIN _unresolved_dependencies ", " _unresolved_text)
    message(WARNING
            "DeployAbqRuntime: unresolved non-system dependencies: "
            "${_unresolved_text}")
endif ()
