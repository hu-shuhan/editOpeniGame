cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(iGameVisDataServer VERSION 0.1.0 LANGUAGES C CXX)
endif()

get_filename_component(IGAME_DATA_SERVER_REPOSITORY_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# In a top-level iGameVis build ThirdParty has already provided this target.
# For a standalone DataServer build, configure only the static zstd library.
if(NOT TARGET libzstd_static)
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_DICTBUILDER OFF CACHE BOOL "" FORCE)
    set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)
    add_subdirectory(
        "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/ThirdParty/zstd/build/cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/zstd"
        EXCLUDE_FROM_ALL)
endif()

add_executable(iGameVisDataServer
    DataServer.cpp
    PackageCatalog.cpp
    PackageCatalog.h
    PackageProtocol.cpp
    PackageProtocol.h
    Sha256.cpp
    Sha256.h
    "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/iGameCore/Core/Common/DataTransfer/iGamePackageProtocol.cpp"
    "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/iGameCore/Core/Common/DataTransfer/iGamePackageProtocol.h"
    "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/iGameCore/Core/Common/DataTransfer/iGameTarZstdArchive.cpp"
    "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/iGameCore/Core/Common/DataTransfer/iGameTarZstdArchive.h"
)

target_compile_features(iGameVisDataServer PRIVATE cxx_std_17)
target_include_directories(iGameVisDataServer PRIVATE
    "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/iGameCore/Core/Common")
target_link_libraries(iGameVisDataServer PRIVATE libzstd_static)

if(MSVC)
    target_compile_options(iGameVisDataServer PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(iGameVisDataServer PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0601
    )
    target_link_libraries(iGameVisDataServer PRIVATE ws2_32)
else()
    target_compile_options(iGameVisDataServer PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS iGameVisDataServer RUNTIME DESTINATION bin)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_igame_data_server_tests_default ON)
else()
    set(_igame_data_server_tests_default OFF)
endif()
option(IGAME_DATA_SERVER_BUILD_TESTS "Build protocol and integration tests"
       ${_igame_data_server_tests_default})
if(IGAME_DATA_SERVER_BUILD_TESTS)
    enable_testing()
    add_executable(iGameVisPackageProtocolTests
        ProtocolTests.cpp
        PackageProtocol.cpp
        PackageProtocol.h
        Sha256.cpp
        Sha256.h
        "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/iGameCore/Core/Common/DataTransfer/iGamePackageProtocol.cpp"
        "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/iGameCore/Core/Common/DataTransfer/iGamePackageProtocol.h"
    )
    target_compile_features(iGameVisPackageProtocolTests PRIVATE cxx_std_17)
    target_include_directories(iGameVisPackageProtocolTests PRIVATE
        "${IGAME_DATA_SERVER_REPOSITORY_ROOT}/iGameCore/Core/Common")
    if(MSVC)
        target_compile_options(iGameVisPackageProtocolTests PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(iGameVisPackageProtocolTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME iGameVisPackageProtocolTests COMMAND iGameVisPackageProtocolTests)

    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME iGameVisDataServerCatalogIntegrationTests
            COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                    "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_LIST_DIR}/CatalogIntegrationTests.py"
                    --server "$<TARGET_FILE:iGameVisDataServer>"
        )
        set_tests_properties(iGameVisDataServerCatalogIntegrationTests PROPERTIES
            TIMEOUT 120
            LABELS "integration;catalog"
        )
    else()
        message(WARNING
            "Python3 was not found; catalog black-box integration tests are disabled")
    endif()
endif()
