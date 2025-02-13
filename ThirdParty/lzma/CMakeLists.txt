file(GLOB_RECURSE srcs CONFIGURE_DEPENDS *.c *.h)

add_library(lzma STATIC ${srcs})


set_target_properties(lzma PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(lzma PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/ThirdParty/lzma>)

if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "-fPIC")
    set_property(TARGET lzma PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()