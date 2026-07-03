#file(GLOB_RECURSE srcs CONFIGURE_DEPENDS ./*.cpp ./*.h)
file(GLOB_RECURSE srcs CONFIGURE_DEPENDS ./tinyxml2.cpp ./tinyxml2.h)

# Currently, only shared libraries for the Linux system are supported
if(IGAME_BUILD_SHARED AND UNIX AND NOT APPLE)
    add_library(tinyXML2 SHARED ${srcs})
else ()
    add_library(tinyXML2 STATIC ${srcs})
endif ()


set_target_properties(tinyXML2 PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(tinyXML2 PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/tinyXML2>

)

if(UNIX AND NOT APPLE)
    set_property(TARGET tinyXML2 PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

