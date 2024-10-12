# - Find FFMPEG SDK library
# Find the FFMPEG SDK includes and library
find_path(FFMPEG_PRIVATE_INCLUDE_DIR
        NAMES
        libavcodec
        PATHS
        /usr/local/include/x86_64-linux-gnu
        /usr/local/include
        /usr/include
        "D:/dev/FFMPEG/include"
)

if(WIN32)
    find_path(FFMPEG_PRIVATE_LIB
            NAMES
            avcodec.lib
            PATHS
            "D:/dev/FFMPEG/lib"
    )
elseif(UNIX)
    if(APPLE)
        message(WARNING "FFMPEG Cmake is not support in this framework currently.")
    else()
        find_path(FFMPEG_PRIVATE_LIB
                NAMES
                libavcodec.a
                PATHS
                /usr/lib/x86_64-linux-gnu
        )
    endif()
endif ()


mark_as_advanced(FFMPEG_PRIVATE_INCLUDE_DIR)
mark_as_advanced(FFMPEG_PRIVATE_LIB)

set(FFMPEG_LIB_LIST
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale
)


set(FOUND_FFMPEG_LIB_FULL_PATH)
set(FFMPEG_LIB_TARGETS)
foreach(LIB ${FFMPEG_LIB_LIST})
    set(temp${LIB})
    find_library(temp${LIB}
            NAMES
            "${LIB}.lib"
            PATHS
            ${FFMPEG_PRIVATE_LIB}
    )
    mark_as_advanced(temp${LIB})
    list(APPEND FOUND_FFMPEG_LIB_FULL_PATH ${temp${LIB}})

    if (NOT TARGET FFMPEG::${LIB})
        add_library(FFMPEG::${LIB} UNKNOWN IMPORTED)
        set_target_properties(FFMPEG::${LIB}
                PROPERTIES
                IMPORTED_LOCATION ${temp${LIB}}
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_PRIVATE_INCLUDE_DIR}")
        list(APPEND FFMPEG_LIB_TARGETS FFMPEG::${LIB})
    endif ()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFMPEG
        REQUIRED_VARS
        FOUND_FFMPEG_LIB_FULL_PATH
        FFMPEG_PRIVATE_INCLUDE_DIR)