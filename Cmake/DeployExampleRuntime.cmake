if (NOT IGAME_RUNTIME_DESTINATION)
    message(FATAL_ERROR "IGAME_RUNTIME_DESTINATION is required")
endif ()

# ---------------------------------------------------------------------------
# 目标目录
#
# 为什么可能不止一个：Examples 的目标是 add_executable（默认输出目录），
# 在 MSBuild / Xcode 这类多配置生成器下，RUNTIME_OUTPUT_DIRECTORY 会**自动**
# 追加 <Config>，于是 exe 落在 Examples/Release/ 而二进制根目录是 Examples/。
# 只往根目录拷会让 exe 附近依旧没有 DLL（P1 实测：build/igv_msvc_release 与
# build/pvr_release 的 Examples/Release/ 下一个 DLL 都没有，
# testIGVRoundTrip.exe 直接 0xC0000135「找不到 DLL」，连 stdout 都来不及写）。
#
# 因此：既有配置子目录时以它为唯一目标（exe 真正所在处），否则回落到根目录。
# IGAME_CONFIG 由 Examples/CMakeLists.txt 以 $<CONFIG> 传入；单配置生成器
# （Ninja / Makefile）下它为空或等于 CMAKE_BUILD_TYPE，此时通常不存在
# <dest>/<Config>，自动回落到根目录。
# ---------------------------------------------------------------------------
set(_IGAME_RUNTIME_DESTINATIONS "")
if (IGAME_RUNTIME_DESTINATION)
    list(APPEND _IGAME_RUNTIME_DESTINATIONS "${IGAME_RUNTIME_DESTINATION}")
    if (IGAME_CONFIG)
        set(_IGAME_CONFIG_DESTINATION "${IGAME_RUNTIME_DESTINATION}/${IGAME_CONFIG}")
        if (EXISTS "${_IGAME_CONFIG_DESTINATION}")
            set(_IGAME_RUNTIME_DESTINATIONS "${_IGAME_CONFIG_DESTINATION}")
        endif ()
    endif ()
endif ()

foreach(_IGAME_RUNTIME_DESTINATION IN LISTS _IGAME_RUNTIME_DESTINATIONS)
    file(MAKE_DIRECTORY "${_IGAME_RUNTIME_DESTINATION}")
endforeach()

# ---------------------------------------------------------------------------
# 资源目录同步到 exe 旁边
#
# 与 DLL 同样的理由：一些 Reader 会在**当前工作目录**下按相对路径找资源
# （例如 `AnsysReader::Parsing()` 找 `Resources\pyAnsysLib\ansys_to_pvd_converter.exe`，
# 现有测试 `Examples/IO/AnsysRSTReader.cpp:33` 也是用 `./Models/hemisphere.rst`），
# 而多配置生成器把 exe 放在 `Examples/<Config>/`。只在 `Examples/` 放 Resources
# 会让 `.rst` / `.ccm` 这类需要外部转换器的格式在运行期静默失败。
#
# 整目录复制（`Examples/CMakeLists.txt` 本来就把它拷到 `Examples/`，这里只是让
# exe 所在的 `<Config>/` 也有同一份）。
# ---------------------------------------------------------------------------
if (IGAME_RESOURCES_SOURCE AND EXISTS "${IGAME_RESOURCES_SOURCE}")
    foreach(_IGAME_RUNTIME_DESTINATION IN LISTS _IGAME_RUNTIME_DESTINATIONS)
        file(COPY "${IGAME_RESOURCES_SOURCE}"
                DESTINATION "${_IGAME_RUNTIME_DESTINATION}")
        message(STATUS "Synced Resources/ to ${_IGAME_RUNTIME_DESTINATION}")
    endforeach()
endif ()

set(_IGAME_RUNTIME_FILES "")

# ---------------------------------------------------------------------------
# 按"目录"收集运行时 DLL
#
# HDF5 之所以按目录（而不是只拷 hdf5.dll 一个文件）：官方 HDF5 2.x 的 Windows 包
# 自带一份 MSVC 运行时（concrt140.dll / msvcp140*.dll / vcruntime140*.dll）。
# 在干净的机器上只拷 hdf5.dll 不够，而逐个列出又会随 HDF5 版本变化。
# 同目录下的 hdf5_tools.dll / hdf5_cpp.dll 等一并带上，无副作用。
# ---------------------------------------------------------------------------
set(_IGAME_HDF5_RUNTIME_DIR "")
if (IGAME_HDF5_RUNTIME_FILE AND EXISTS "${IGAME_HDF5_RUNTIME_FILE}")
    get_filename_component(_IGAME_HDF5_RUNTIME_DIR "${IGAME_HDF5_RUNTIME_FILE}" DIRECTORY)
endif ()

foreach(_IGAME_RUNTIME_DIR IN ITEMS
        "${IGAME_TORCH_RUNTIME_DIR}"
        "${IGAME_TORCH_BIN_DIR}"
        "${IGAME_FFMPEG_RUNTIME_DIR}"
        "${_IGAME_HDF5_RUNTIME_DIR}")
    if (_IGAME_RUNTIME_DIR AND EXISTS "${_IGAME_RUNTIME_DIR}")
        file(GLOB _IGAME_DIRECTORY_DLLS LIST_DIRECTORIES FALSE
                "${_IGAME_RUNTIME_DIR}/*.dll")
        list(APPEND _IGAME_RUNTIME_FILES ${_IGAME_DIRECTORY_DLLS})
    endif ()
endforeach()

if (IGAME_GPS_RUNTIME_DIR AND EXISTS "${IGAME_GPS_RUNTIME_DIR}")
    file(GLOB_RECURSE _IGAME_GPS_DLLS LIST_DIRECTORIES FALSE
            "${IGAME_GPS_RUNTIME_DIR}/*.dll")
    list(APPEND _IGAME_RUNTIME_FILES ${_IGAME_GPS_DLLS})
endif ()

# HDF5 自己的传递依赖：zlib / szip（不同发行版命名不同，逐个探测）。
# MSVC 运行时由上面的目录 GLOB 覆盖。
if (_IGAME_HDF5_RUNTIME_DIR)
    foreach(_IGAME_HDF5_DEP zlibd1.dll zlib1.dll szip_D.dll szip.dll)
        if (EXISTS "${_IGAME_HDF5_RUNTIME_DIR}/${_IGAME_HDF5_DEP}")
            list(APPEND _IGAME_RUNTIME_FILES "${_IGAME_HDF5_RUNTIME_DIR}/${_IGAME_HDF5_DEP}")
        endif ()
    endforeach()
endif ()

if (IGAME_ABQ_RUNTIME_DIR AND IGAME_ABQ_RUNTIME_LIBRARY_NAMES)
    string(REPLACE "," ";" _IGAME_ABQ_RUNTIME_LIBRARY_NAMES
            "${IGAME_ABQ_RUNTIME_LIBRARY_NAMES}")
    foreach(_IGAME_ABQ_LIBRARY_NAME IN LISTS _IGAME_ABQ_RUNTIME_LIBRARY_NAMES)
        set(_IGAME_ABQ_RUNTIME_FILE
                "${IGAME_ABQ_RUNTIME_DIR}/${_IGAME_ABQ_LIBRARY_NAME}.dll")
        if (EXISTS "${_IGAME_ABQ_RUNTIME_FILE}")
            list(APPEND _IGAME_RUNTIME_FILES "${_IGAME_ABQ_RUNTIME_FILE}")
        else ()
            message(WARNING "Example runtime dependency not found: ${_IGAME_ABQ_RUNTIME_FILE}")
        endif ()
    endforeach()
endif ()

list(REMOVE_DUPLICATES _IGAME_RUNTIME_FILES)
if (_IGAME_RUNTIME_FILES)
    list(LENGTH _IGAME_RUNTIME_FILES _IGAME_RUNTIME_FILE_COUNT)
    foreach(_IGAME_RUNTIME_DESTINATION IN LISTS _IGAME_RUNTIME_DESTINATIONS)
        file(COPY ${_IGAME_RUNTIME_FILES}
                DESTINATION "${_IGAME_RUNTIME_DESTINATION}")
        message(STATUS
                "Deployed ${_IGAME_RUNTIME_FILE_COUNT} runtime DLLs to ${_IGAME_RUNTIME_DESTINATION}")
    endforeach()
endif ()
