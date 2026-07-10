# - Find FFMPEG SDK library
# Find the FFMPEG SDK includes and library
set(_FFMPEG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/FFMPEG")

find_path(FFMPEG_PRIVATE_INCLUDE_DIR
        NAMES
        libavcodec/avcodec.h
        PATHS
        "${_FFMPEG_ROOT}/include"
        /usr/include/x86_64-linux-gnu
        /usr/local/include/x86_64-linux-gnu
        /usr/local/include
        /usr/include
        "D:/dev/FFMPEG/include"
        "D:/iGameVis/FFMPEG/include"
)

if (WIN32)
    find_path(FFMPEG_PRIVATE_LIB
            NAMES
            avcodec.lib
            PATHS
            "${_FFMPEG_ROOT}/lib"
            "D:/dev/FFMPEG/lib"
            "D:/iGameVis/FFMPEG/lib"
    )
    find_path(FFMPEG_DLL_DIR
            NAMES
            avcodec-61.dll
            PATHS
            "${_FFMPEG_ROOT}/bin"
            "D:/dev/FFMPEG/bin"
            "D:/iGameVis/FFMPEG/bin"
    )
elseif (APPLE)
    message(WARNING "FFMPEG Cmake is not support in this framework currently.")
else ()
    find_library(_FFMPEG_AVCODEC_LIBRARY
            NAMES
            avcodec
            libavcodec
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib
            /usr/local/lib
    )
    if (_FFMPEG_AVCODEC_LIBRARY)
        get_filename_component(FFMPEG_PRIVATE_LIB "${_FFMPEG_AVCODEC_LIBRARY}" DIRECTORY)
    endif ()
endif ()

mark_as_advanced(FFMPEG_PRIVATE_INCLUDE_DIR)
mark_as_advanced(FFMPEG_PRIVATE_LIB)
mark_as_advanced(FFMPEG_DLL_DIR)

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
mark_as_advanced(FFMPEG_LIB_LIST)

set(FOUND_FFMPEG_LIB_FULL_PATH)
set(FFMPEG_LIB_TARGETS)
foreach (LIB ${FFMPEG_LIB_LIST})
    if (WIN32)
        find_library(temp${LIB}
                NAMES
                "${LIB}.lib"
                PATHS
                ${FFMPEG_PRIVATE_LIB}
                NO_DEFAULT_PATH
        )
    else ()
        find_library(temp${LIB}
                NAMES
                ${LIB}
                lib${LIB}
                PATHS
                ${FFMPEG_PRIVATE_LIB}
                /usr/lib/x86_64-linux-gnu
                /usr/lib
                /usr/local/lib
        )
    endif ()
    mark_as_advanced(temp${LIB})

    if (temp${LIB})
        list(APPEND FOUND_FFMPEG_LIB_FULL_PATH ${temp${LIB}})

        if (NOT TARGET FFMPEG::${LIB})
            add_library(FFMPEG::${LIB} UNKNOWN IMPORTED)
            set_target_properties(FFMPEG::${LIB}
                    PROPERTIES
                    IMPORTED_LOCATION ${temp${LIB}}
                    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_PRIVATE_INCLUDE_DIR}")
            list(APPEND FFMPEG_LIB_TARGETS FFMPEG::${LIB})
        endif ()
    endif ()
endforeach ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFMPEG
        REQUIRED_VARS
        FFMPEG_PRIVATE_INCLUDE_DIR
        tempavcodec
        tempavdevice
        tempavfilter
        tempavformat
        tempavutil
        temppostproc
        tempswresample
        tempswscale)
