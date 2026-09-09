cmake_minimum_required(VERSION 3.20)
project(iGameVisRemoteCatalogClientTests LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT Qt5_DIR AND DEFINED ENV{QT_HOME})
    set(Qt5_DIR "$ENV{QT_HOME}/5.15.2/msvc2019_64/lib/cmake/Qt5")
endif()
find_package(Qt5 REQUIRED COMPONENTS Core)

get_filename_component(IGAMEVIS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_executable(iGameVisRemoteCatalogClientTests
    RemoteCatalogClientTests.cpp
    "${IGAMEVIS_ROOT}/Qt/src/IQCore/igQtRemoteCatalogClient.cpp"
    "${IGAMEVIS_ROOT}/Qt/include/IQCore/igQtRemoteCatalogClient.h"
    "${IGAMEVIS_ROOT}/Qt/src/IQCore/igQtRemotePackageValidation.cpp"
    "${IGAMEVIS_ROOT}/Qt/include/IQCore/igQtRemotePackageValidation.h"
    "${IGAMEVIS_ROOT}/iGameCore/Core/Common/DataTransfer/iGamePackageProtocol.cpp"
    "${IGAMEVIS_ROOT}/iGameCore/Core/Common/DataTransfer/iGamePackageProtocol.h"
)

add_executable(iGameVisRemotePackageDownloaderTests
    RemotePackageDownloaderTests.cpp
    "${IGAMEVIS_ROOT}/Qt/src/IQCore/igQtPackageDownloader.cpp"
    "${IGAMEVIS_ROOT}/Qt/include/IQCore/igQtPackageDownloader.h"
    "${IGAMEVIS_ROOT}/iGameCore/Core/Common/DataTransfer/iGamePackageProtocol.cpp"
    "${IGAMEVIS_ROOT}/iGameCore/Core/Common/DataTransfer/iGamePackageProtocol.h"
)
target_include_directories(iGameVisRemotePackageDownloaderTests PRIVATE
    "${IGAMEVIS_ROOT}/Qt/include"
    "${IGAMEVIS_ROOT}/iGameCore/Core/Common"
)
target_compile_definitions(iGameVisRemotePackageDownloaderTests PRIVATE IG_QT_COMPILE_EXPORT)
target_link_libraries(iGameVisRemotePackageDownloaderTests PRIVATE Qt5::Core)
if(WIN32)
    target_compile_definitions(iGameVisRemotePackageDownloaderTests PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601)
    target_link_libraries(iGameVisRemotePackageDownloaderTests PRIVATE ws2_32)
endif()
target_include_directories(iGameVisRemoteCatalogClientTests PRIVATE
    "${IGAMEVIS_ROOT}/Qt/include"
    "${IGAMEVIS_ROOT}/iGameCore/Core/Common"
)
target_compile_definitions(iGameVisRemoteCatalogClientTests PRIVATE IG_QT_COMPILE_EXPORT)
target_link_libraries(iGameVisRemoteCatalogClientTests PRIVATE Qt5::Core)
if(WIN32)
    target_compile_definitions(iGameVisRemoteCatalogClientTests PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601)
    target_link_libraries(iGameVisRemoteCatalogClientTests PRIVATE ws2_32)
endif()

enable_testing()
add_test(NAME iGameVisRemoteCatalogClientTests
         COMMAND iGameVisRemoteCatalogClientTests)
set_tests_properties(iGameVisRemoteCatalogClientTests PROPERTIES TIMEOUT 30)
add_test(NAME iGameVisRemotePackageDownloaderTests
         COMMAND iGameVisRemotePackageDownloaderTests)
set_tests_properties(iGameVisRemotePackageDownloaderTests PROPERTIES TIMEOUT 30)
