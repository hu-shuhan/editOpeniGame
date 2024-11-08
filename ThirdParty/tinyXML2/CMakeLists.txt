file(GLOB_RECURSE srcs CONFIGURE_DEPENDS ./*.cpp ./*.h)
add_library(tinyXML2 STATIC ${srcs})
set_target_properties(tinyXML2 PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(tinyXML2 PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/tinyXML2>

)

if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "-fPIC")
    set_property(TARGET tinyXML2 PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

